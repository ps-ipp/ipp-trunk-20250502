# include "relastro.h"

int hpm_catalogs_parallel (SkyList *sky);

int hpm_catalogs (SkyTable *sky, SkyList *skylist, int hostID, char *hostpath) {

  int i;
  Catalog catalog;

  // tell libdvo the CATDIR
  dvo_set_catdir(CATDIR);

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    hpm_catalogs_parallel (skylist);
    goto finish;
  }

  // load data from each region file, only use bright stars
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = hostID ? hostfile : skylist[0].filename[i];
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE2, "r")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    hpm_objects (skylist[0].regions[i], &catalog);

    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
  }
  
finish:

  // in relastro (NOT relastr_client), save the Images.dat, Photcodes.dat, SkyTable.fits files
  if (!hostID) {
    char photcodeFile[1024];
    snprintf (photcodeFile, 1024, "%s/Photcodes.dat", HIGH_SPEED_DIR);
    if (!SavePhotcodesFITS (photcodeFile)) {
      fprintf (stderr, "error saving photcode table %s\n", photcodeFile);
      exit (1);
    }
    
    // need to copy across the Images, SkyTable, and Photcode tables:
    char *skyfile = SkyTableFilename (HIGH_SPEED_DIR);
    SkyTableSave (sky, skyfile);
    
    char line[2048];
    snprintf (line, 2048, "cp %s/Images.dat %s/Images.dat", CATDIR, HIGH_SPEED_DIR);
    int status = system (line);
    if (status) {
      fprintf (stderr, "copy of Images.dat failed\n");
      exit (3);
    }
  }

  return (TRUE);
}

// CATDIR is supplied globally
# define DEBUG 1
int hpm_catalogs_parallel (SkyList *sky) {

  // launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  // If we want to parallelize the output, the remote client needs to load the HostTable
  // for the output catdir.  Require the output HostTable to match the input (ie, same
  // hostIDs must exist).  Preferred, but not required, that they match in the host
  // location.
  HostTable *tableOut = NULL;
  if (PARALLEL_OUTPUT) {
    // load the list of hosts
    tableOut = HostTableLoad (HIGH_SPEED_DIR, sky->hosts);
    if (!tableOut) {
      fprintf (stderr, "ERROR: failure reading Host Table %s for output database %s\n", sky->hosts, HIGH_SPEED_DIR);
      exit (1);
    }    
    if (table->Nhosts != tableOut->Nhosts) {
      fprintf (stderr, "ERROR: output HostTable must have matching hosts (%d in, %d out)\n", table->Nhosts, tableOut->Nhosts);
      exit (1);
    }
  }

  int i, j;
  for (i = 0; i < table->Nhosts; i++) {

    if (sky->Nregions < table->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], table->hosts[i].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) {
	// fprintf (stderr, "skip host %s\n", table->hosts[i].hostname);
	continue;
      }
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // use this directory to save the output catalogs (distributed if parallel)
    char *outputDir = strcreate(HIGH_SPEED_DIR);
    if (PARALLEL_OUTPUT) {
      // find the matching host entry in the output HostTable (match by ID, not index)
      int hostID = table->hosts[i].hostID;
      int index = table->index[hostID];
      if (index == -1) {
	fprintf (stderr, "ERROR: output HostTable must have matching hosts (host ID %d not found)\n", hostID);
	exit (1);
      }

      free (outputDir);
      outputDir = abspath (tableOut->hosts[index].pathname, DVO_MAX_PATH);
    }

    char *command = NULL;
    strextend (&command, "relastro_client -hpm %f %s", RADIUS, outputDir);
    strextend (&command, " -hostID %d", table->hosts[i].hostID);
    strextend (&command, " -D CATDIR %s", CATDIR);
    strextend (&command, " -hostdir %s", table->hosts[i].pathname);
    strextend (&command, " -region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    free (outputDir);

    // options / arguments that can affect relastro_client -high-speed
    if (FIT_MODE == FIT_PM_ONLY)  	 { strextend (&command, "-pm"); }
    if (FIT_MODE == FIT_PAR_ONLY) 	 { strextend (&command, "-par"); }
    if (FIT_MODE == FIT_PM_AND_PAR)      { strextend (&command, "-pmpar"); }

    if (VERBOSE)       { strextend (&command, "-v"); }
    if (VERBOSE2)      { strextend (&command, "-vv"); }
    if (RESET)         { strextend (&command, "-reset"); }
    if (ImagSelect)    { strextend (&command, "-instmag %f %f", ImagMin, ImagMax); }
    if (MaxDensityUse) { strextend (&command, "-max-density %f", MaxDensityValue); }
    
    if (USE_ALL_IMAGES)      { strextend (&command, "-use-all-images"); }
    if (USE_BASIC_CHECK)     { strextend (&command, "-basic-image-search"); }
    if (FlagOutlier)         { strextend (&command, "-clip %d", CLIP_THRESH); }
    if (USE_FIXED_PIXCOORDS) { strextend (&command, "-D USE_FIXED_PIXCOORDS 1"); }
    if (PHOTCODE_KEEP_LIST)  { strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST); }
    if (PHOTCODE_SKIP_LIST)  { strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST); }
    if (PhotFlagSelect)      { strextend (&command, "+photflags"); }
    if (PhotFlagBad)         { strextend (&command, "+photflagbad %d", PhotFlagBad); }
    if (PhotFlagPoor)        { strextend (&command, "+photflagpoor %d", PhotFlagPoor); }
    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

    if (MinBadQF > 0.0)          strextend (&command, "-min-bad-psfqf %f", MinBadQF); 
    if (MaxMeanOffset != 10.0)   strextend (&command, "-max-mean-offset  %f", MaxMeanOffset); 
    if (N_BOOTSTRAP_SAMPLES > 1) strextend (&command, "-bootstrap-samples %d", N_BOOTSTRAP_SAMPLES); 

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      strextend (&command, "-time %s %s", tstart, tstop); 
      free (tstart);
      free (tstop);
    }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running relastro_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      
