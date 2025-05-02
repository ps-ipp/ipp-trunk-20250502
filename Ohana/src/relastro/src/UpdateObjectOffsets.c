# include "relastro.h"

// We run through each DVO catalog, updating the measures that come from the modified images
// We need to watch for failures:
// * in UpdateMeasures, in fixImageRaw, we track the cumulative offset for each image
// * after all updates are done, we can check for any bad images and reset them to the
//   original coordinates

int UpdateObjectOffsets_parallel (SkyList *sky);

int UpdateObjectOffsets (SkyList *skylist, int hostID, char *hostpath) {

  int i;
  Catalog catalog;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    UpdateObjectOffsets_parallel (skylist);
    return TRUE;
  }

  // load data from each region file, only use bright stars
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // define the catalog file name
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = hostID ? hostfile : skylist[0].filename[i];

    // set up the basic catalog info
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

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

    // set the values in MeasureTiny needed by UpdateObjects
    populate_tiny_values(&catalog, DVO_TV_MEASURE);

    // match measurements with images
    initImageBins (&catalog, 1, FALSE);
    findImages (&catalog, 1, FALSE);

    if (REPAIR_STACKS) RepairStackMeasures (&catalog);

    // update the detection coordinates using the new image parameters
    UpdateMeasures (&catalog, 1);

    // uses MeasureTiny values, but will also update Measure values when populated
    // if Galaxy Motion Model correction is active, do not apply at this last stage 
    UpdateObjects (&catalog, 1, -1);

    if (CHECK_MEASURE_TO_IMAGE) CheckMeasureToImage (&catalog);

    free_tiny_values(&catalog);

    freeImageBins (1);

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

    // write the updated detections to disk
    save_catalogs (&catalog, 1);
  }
  printNcatTotal();
  return (TRUE);
}

int UpdateObjectOffsets_parallel_group (HostTableGroup *group, SkyList *sky);
int UpdateObjectOffsets_parallel_table (HostTable *table, SkyList *sky);

// CATDIR is supplied globally
# define DEBUG 1
int UpdateObjectOffsets_parallel (SkyList *sky) {

  // launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

# if (1)
  
  UpdateObjectOffsets_parallel_table (table, sky);

# else

  int Ngroups;
  HostTableGroup *groups = HostTableGroupsUniqueMachines (table, &Ngroups);
  // split the table into Ngroups, each with a unique set of machines (this avoids overloading the remote host machines)

  int i;
  for (i = 0; i < Ngroups; i++) {
    // update only a group of unique machines at a time
    UpdateObjectOffsets_parallel_group (&groups[i], sky);
  }
# endif

  return TRUE;
}      

// CATDIR is supplied globally
# define DEBUG 1
int UpdateObjectOffsets_parallel_group (HostTableGroup *group, SkyList *sky) {

  // launch the relastro_client jobs to the parallel hosts

  int i, j;
  for (i = 0; i < group->Nhosts; i++) {

    if (sky->Nregions < group->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], group->hosts[i][0].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) {
	// fprintf (stderr, "skip host %s\n", group->hosts[i][0].hostname);
	continue;
      }
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (group->hosts[i][0].pathname, DVO_MAX_PATH);
    free (group->hosts[i][0].pathname);
    group->hosts[i][0].pathname = tmppath;

    // options / arguments that can affect relastro_client -load:
    // VERBOSE, VERBOSE2
    // RESET (-reset)
    // TimeSelect -time
    // (note that psfQF is applied rigidly at 0.85, as is the galaxy test)
    // ImagSelect, ImagMin, ImagMax
    // MaxDensityUse, MaxDensityValue

    char *command = NULL;
    strextend (&command, "relastro_client -update-offsets");
    strextend (&command, "-hostID %d", group->hosts[i][0].hostID);
    strextend (&command, "-hostdir %s", group->hosts[i][0].pathname);

    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-minerror %f", MIN_ERROR);

    strextend (&command, "-D RELASTRO_SIGMA_LIM %f", SIGMA_LIM);
    strextend (&command, "-D RELASTRO_SRC_MEAS_TOOFEW %d", SRC_MEAS_TOOFEW);

    if (FIT_MODE == FIT_PM_ONLY)  	 strextend (&command, "-pm");
    if (FIT_MODE == FIT_PAR_ONLY) 	 strextend (&command, "-par");
    if (FIT_MODE == FIT_PM_AND_PAR)      strextend (&command, "-pmpar");

    if (VERBOSE)       strextend (&command, "-v");
    if (VERBOSE2)      strextend (&command, "-vv");
    if (RESET)         strextend (&command, "-reset");

    if (ImagSelect)    strextend (&command, "-instmag %f %f", ImagMin, ImagMax);
    if (MaxDensityUse) strextend (&command, "-max-density %f", MaxDensityValue);
    if (FlagOutlier)     strextend (&command, "-clip %d", CLIP_THRESH);
    if (ExcludeBogus)    strextend (&command, "-exclude-bogus %f", ExcludeBogusRadius);
    
    if (USE_FIXED_PIXCOORDS) strextend (&command, "-D USE_FIXED_PIXCOORDS 1"); 
    if (PHOTCODE_KEEP_LIST) strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST);
    if (PHOTCODE_SKIP_LIST) strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST);
    if (PhotFlagSelect)     strextend (&command, "+photflags"); 
    if (PhotFlagBad)        strextend (&command, "+photflagbad %d", PhotFlagBad);
    if (PhotFlagPoor)       strextend (&command, "+photflagpoor %d", PhotFlagPoor);
    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

    if (DCR_BLUE_COLOR_POS && DCR_BLUE_COLOR_NEG) {
      strextend (&command, "-dcr-blue-color %s %s", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG); 
    }
    if (DCR_RED_COLOR_POS && DCR_RED_COLOR_NEG) {
      strextend (&command, "-dcr-red-color %s %s", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG); 
    }

    if (REPAIR_STACKS)          strextend (&command, "-repair-stacks-on-update");
    if (CHECK_MEASURE_TO_IMAGE) strextend (&command, "-check-measures");

    if (UPDATE_ALL_MEASURE)       strextend (&command, "-update-all-cameras");
    if (UPDATE_PS1_STACK_MEASURE) strextend (&command, "-update-ps1-stack");
    if (UPDATE_PS1_CHIP_MEASURE)  strextend (&command, "-update-ps1-chip");
    if (UPDATE_HSC_MEASURE)    	  strextend (&command, "-update-hsc");
    if (UPDATE_CFH_MEASURE)    	  strextend (&command, "-update-cfh");

    if (UPDATE)           strextend (&command, "-update");
    if (RESET_BAD_IMAGES) strextend (&command, "-reset-bad-images");
    if (USE_BASIC_CHECK)  strextend (&command, "-basic-image-search");
    if (USE_ALL_IMAGES)   strextend (&command, "-use-all-images");

    if (MinBadQF > 0.0)        strextend (&command, "-min-bad-psfqf %f", MinBadQF); 
    if (MaxMeanOffset != 10.0) strextend (&command, "-max-mean-offset  %f", MaxMeanOffset);
    if (N_BOOTSTRAP_SAMPLES > 1) strextend (&command, "-bootstrap-samples %d", N_BOOTSTRAP_SAMPLES); 

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      strextend (&command, "-time %s %s", tstart, tstop); 
      free (tstart);
      free (tstop);
    }
    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running relastro_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", group->hosts[i][0].hostname, command, group->hosts[i][0].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", group->hosts[i][0].hostname, errorInfo);
	exit (1);
      }
      group->hosts[i][0].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableGroupWaitJobsGetIO (group, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      

// CATDIR is supplied globally
# define DEBUG 1
int UpdateObjectOffsets_parallel_table (HostTable *table, SkyList *sky) {

  // launch the relastro_client jobs to the parallel hosts

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

    // options / arguments that can affect relastro_client -load:
    // VERBOSE, VERBOSE2
    // RESET (-reset)
    // TimeSelect -time
    // (note that psfQF is applied rigidly at 0.85, as is the galaxy test)
    // ImagSelect, ImagMin, ImagMax
    // MaxDensityUse, MaxDensityValue

    char *command = NULL;
    strextend (&command, "relastro_client -update-offsets");
    strextend (&command, "-hostID %d", table->hosts[i].hostID);
    strextend (&command, "-hostdir %s", table->hosts[i].pathname);

    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-minerror %f", MIN_ERROR);

    strextend (&command, "-D RELASTRO_SIGMA_LIM %f", SIGMA_LIM);
    strextend (&command, "-D RELASTRO_SRC_MEAS_TOOFEW %d", SRC_MEAS_TOOFEW);

    if (FIT_MODE == FIT_PM_ONLY)  	 strextend (&command, "-pm");
    if (FIT_MODE == FIT_PAR_ONLY) 	 strextend (&command, "-par");
    if (FIT_MODE == FIT_PM_AND_PAR)      strextend (&command, "-pmpar");

    if (VERBOSE)       strextend (&command, "-v");
    if (VERBOSE2)      strextend (&command, "-vv");
    if (RESET)         strextend (&command, "-reset");

    if (ImagSelect)    strextend (&command, "-instmag %f %f", ImagMin, ImagMax);
    if (MaxDensityUse) strextend (&command, "-max-density %f", MaxDensityValue);
    if (FlagOutlier)     strextend (&command, "-clip %d", CLIP_THRESH);
    if (ExcludeBogus)    strextend (&command, "-exclude-bogus %f", ExcludeBogusRadius);
    
    if (USE_FIXED_PIXCOORDS) strextend (&command, "-D USE_FIXED_PIXCOORDS 1"); 
    if (PHOTCODE_KEEP_LIST) strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST);
    if (PHOTCODE_SKIP_LIST) strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST);
    if (PhotFlagSelect)     strextend (&command, "+photflags"); 
    if (PhotFlagBad)        strextend (&command, "+photflagbad %d", PhotFlagBad);
    if (PhotFlagPoor)       strextend (&command, "+photflagpoor %d", PhotFlagPoor);
    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

    if (DCR_BLUE_COLOR_POS && DCR_BLUE_COLOR_NEG) {
      strextend (&command, "-dcr-blue-color %s %s", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG); 
    }
    if (DCR_RED_COLOR_POS && DCR_RED_COLOR_NEG) {
      strextend (&command, "-dcr-red-color %s %s", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG); 
    }

    if (REPAIR_STACKS)          strextend (&command, "-repair-stacks-on-update");
    if (CHECK_MEASURE_TO_IMAGE) strextend (&command, "-check-measures");

    if (UPDATE_ALL_MEASURE)       strextend (&command, "-update-all-cameras");
    if (UPDATE_PS1_STACK_MEASURE) strextend (&command, "-update-ps1-stack");
    if (UPDATE_PS1_CHIP_MEASURE)  strextend (&command, "-update-ps1-chip");
    if (UPDATE_HSC_MEASURE)    	  strextend (&command, "-update-hsc");
    if (UPDATE_CFH_MEASURE)    	  strextend (&command, "-update-cfh");

    if (UPDATE)        strextend (&command, "-update");
    if (RESET_BAD_IMAGES) strextend (&command, "-reset-bad-images");
    if (USE_BASIC_CHECK) strextend (&command, "-basic-image-search");
    if (USE_ALL_IMAGES)      strextend (&command, "-use-all-images");

    if (N_BOOTSTRAP_SAMPLES > 1) strextend (&command, "-bootstrap-samples %d", N_BOOTSTRAP_SAMPLES); 

    if (MinBadQF > 0.0)        strextend (&command, "-min-bad-psfqf %f", MinBadQF); 
    if (MaxMeanOffset != 10.0) strextend (&command, "-max-mean-offset  %f", MaxMeanOffset);

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      strextend (&command, "-time %s %s", tstart, tstop); 
      free (tstart);
      free (tstop);
    }
    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      continue;
    }

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
