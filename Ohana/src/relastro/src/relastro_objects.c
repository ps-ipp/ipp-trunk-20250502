# include "relastro.h"

int relastro_objects_parallel (SkyList *sky);

int relastro_objects (SkyList *skylist, int hostID, char *hostpath) {

  int i, j, k, m;

  Catalog catalog;

  DVOMeasureFlags measureBits = 
    ID_MEAS_POOR_ASTROM |
    ID_MEAS_SKIP_ASTROM | 
    ID_MEAS_USED_OBJ    |
    ID_MEAS_USED_CHIP;

  DVOAverageFlags astromBits = 
    ID_OBJ_LARGE_PM        | // star with large proper motion
    ID_OBJ_RAW_AVE     	   | // simple weighted average position was used (no IRLS fitting)
    ID_OBJ_FIT_AVE         | // average position was fitted
    ID_OBJ_FIT_PM          | // proper motion model was fitted
    ID_OBJ_FIT_PAR         | // parallax model was fitted
    ID_OBJ_USE_AVE         | // average position used (not PM or PAR)
    ID_OBJ_USE_PM          | // proper motion used (not AVE or PAR)
    ID_OBJ_USE_PAR         | // parallax used (not AVE or PM)
    ID_OBJ_NO_MEAN_ASTROM  | // mean astrometry could not be measured
    ID_OBJ_STACK_FOR_MEAN  | // stack position used for mean astrometry
    ID_OBJ_MEAN_FOR_STACK  | // mean astrometry could not be measured
    ID_OBJ_BAD_PM;           // failure to measure proper-motion model

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    relastro_objects_parallel (skylist);
    return TRUE;
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

    // set up the basic catalog info
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    // loads Average, Measure, SecFilt
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE2, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    // operates on full tables (Average, Measure, Secfilt)
    if (FlagOutlier) {
      FlagOutliers(&catalog);
    }

    // reset only the astrometry-related average and measure bits
    if (RESET) {
      for (j = 0; j < catalog.Naverage; j++) {
	catalog.average[j].flags &= ~astromBits;
	m = catalog.average[j].measureOffset;
	for (k = 0; k < catalog.average[j].Nmeasure; k++) {
	  catalog.measure[m+k].dbFlags &= ~measureBits;
	}
      }
    }

    populate_tiny_values(&catalog, DVO_TV_MEASURE);

    // the 3rd argument (-1) is the loop number for re-weighting 2MASS and Tycho. -1 is a special value meaning "ignore" 
    // we do NOT want to apply special weights to 2MASS and/or Tycho when calculating object final motions.
    UpdateObjects (&catalog, 1, -1); 

    free_tiny_values(&catalog);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    gfits_modify (&catalog.header, "RELASTRO", "%s", 1, moddate);      
    free (moddate);

    save_catalogs (&catalog, 1);
  }
  
  return (TRUE);
}

// CATDIR is supplied globally
# define DEBUG 1
int relastro_objects_parallel (SkyList *sky) {

  // launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
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

    // options / arguments that can affect relastro_client -update-objects:
    // VERBOSE, VERBOSE2
    // RESET (-reset)
    // TimeSelect -time
    // (note that psfQF is applied rigidly at 0.85, as is the galaxy test)
    // ImagSelect, ImagMin, ImagMax
    // MaxDensityUse, MaxDensityValue

    // FIT_MODE
    // PM_TOOFEW
    // SRC_MEAS_TOOFEW

    char *command = NULL;
    strextend (&command, "relastro_client -update-objects");
    strextend (&command, "-hostID %d", table->hosts[i].hostID);
    strextend (&command, "-hostdir %s", table->hosts[i].pathname);

    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-minerror %f", MIN_ERROR);

    strextend (&command, "-D RELASTRO_SIGMA_LIM %f", SIGMA_LIM);
    strextend (&command, "-D RELASTRO_SRC_MEAS_TOOFEW %d", SRC_MEAS_TOOFEW);

    if (FIT_MODE == FIT_PM_ONLY)  	 { strextend (&command, "-pm"); }
    if (FIT_MODE == FIT_PAR_ONLY) 	 { strextend (&command, "-par"); }
    if (FIT_MODE == FIT_PM_AND_PAR)      { strextend (&command, "-pmpar"); }

    if (VERBOSE)       { strextend (&command, "-v"); }
    if (VERBOSE2)      { strextend (&command, "-vv"); }
    if (RESET)         { strextend (&command, "-reset"); }

    if (ImagSelect)    { strextend (&command, "-instmag %f %f", ImagMin, ImagMax); }
    if (MaxDensityUse) { strextend (&command, "-max-density %f", MaxDensityValue); }
    if (FlagOutlier)   { strextend (&command, "-clip %d", CLIP_THRESH); }
    if (ExcludeBogus)    strextend (&command, "-exclude-bogus %f", ExcludeBogusRadius);

    if (USE_FIXED_PIXCOORDS) { strextend (&command, "-D USE_FIXED_PIXCOORDS 1"); }
    if (PHOTCODE_KEEP_LIST)  { strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST); }
    if (PHOTCODE_SKIP_LIST)  { strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST); }
    if (PhotFlagSelect)      { strextend (&command, "+photflags"); }
    if (PhotFlagBad)         { strextend (&command, "+photflagbad %d", PhotFlagBad); }
    if (PhotFlagPoor)        { strextend (&command, "+photflagpoor %d", PhotFlagPoor); }
    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

    if (DCR_BLUE_COLOR_POS && DCR_BLUE_COLOR_NEG) {
      strextend (&command, "-dcr-blue-color %s %s", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG); 
    }
    if (DCR_RED_COLOR_POS && DCR_RED_COLOR_NEG) {
      strextend (&command, "-dcr-red-color %s %s", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG); 
    }

    if (UPDATE)        { strextend (&command, "-update"); }
    if (USE_ALL_IMAGES)      { strextend (&command, "-use-all-images"); }

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
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }
  FreeHostTable (table);

  return TRUE;
}      
