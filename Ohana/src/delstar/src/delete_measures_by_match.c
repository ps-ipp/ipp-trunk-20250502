# include "delstar.h"
int isGPC1chip (int photcode);
int isGPC1warp (int photcode);
int isGPC1stack (int photcode);
int dvo_catalog_subset_backup (Catalog *catalog, char *suffix);

// this function identifies detections to be deleted by matching the supplied values (time, photcode, imageID)

int delete_measures_by_match () {

  int i;
  Catalog catalog;

  int validOptions = FALSE;
  validOptions |= DELETE_MAX_CAT_ID;
  validOptions |= DELETE_MIN_CAT_ID;
  validOptions |= DELETE_MAX_DET_ID;
  validOptions |= DELETE_MIN_DET_ID;
  validOptions |= DELETE_MAX_IMAGE_ID;
  validOptions |= DELETE_MIN_IMAGE_ID;
  validOptions |= DELETE_MAX_PHOTCODE;
  validOptions |= DELETE_MIN_PHOTCODE;
  validOptions |= DELETE_MAX_TIME;
  validOptions |= DELETE_MIN_TIME;
  if (!validOptions) {
    fprintf (stderr, "ERROR : no valid delete ranges selected\n");
    exit (2);
  }

  fprintf (stderr, "deleting measurements matching the following\n");
  fprintf (stderr, "image ID range : %d to %d\n", DELETE_MIN_IMAGE_ID, DELETE_MAX_IMAGE_ID);
  fprintf (stderr, "det ID range : %d to %d\n", DELETE_MIN_DET_ID, DELETE_MAX_DET_ID);
  fprintf (stderr, "cat ID range : %d to %d\n", DELETE_MIN_CAT_ID, DELETE_MAX_CAT_ID);
  fprintf (stderr, "photcode range : %d to %d\n", DELETE_MIN_PHOTCODE, DELETE_MAX_PHOTCODE);
  fprintf (stderr, "time range (UNIX) : %d to %d\n", DELETE_MIN_TIME, DELETE_MAX_TIME);

  char *minDate = ohana_sec_to_date (DELETE_MIN_TIME);
  char *maxDate = ohana_sec_to_date (DELETE_MAX_TIME);
  fprintf (stderr, "time range (date) : %s to %s\n", minDate, maxDate);
  free (minDate); free (maxDate);

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
    exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  // determine the populated SkyRegions overlapping the requested area (default depth)
  SkyList *skylist = NULL;
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  if (!skylist) {
    fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
    exit (2);
  }

  // launch the remote jobs
  if (PARALLEL && !HOST_ID) {
    int status = delete_measures_by_match_parallel (skylist);

    SkyTableFree (sky);
    SkyListFree (skylist);
    FreePhotcodeTable ();

    return status;
  }

  // delete detections from region
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : skylist[0].filename[i];
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;

    if (VERBOSE) fprintf (stderr, "deleting from %s\n", catalog.filename);

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE2, "a")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    DeleteMeasureResult result = delete_measures_by_match_catalog (&catalog);
    
    // track number of deletions and only update if modifications are made
    int Nmods = 0;
    Nmods += result.NdelWarp;
    Nmods += result.NdelChip;
    Nmods += result.NdelStack;
    Nmods += result.NdelOther;
    Nmods += result.NdelAves;
    Nmods += result.NdelMeas;
    if (!Nmods) {
      fprintf (stderr, "no changes to %s, no output\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    char history[128];
    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    snprintf (history, 128, "delete measurements by match: %s", moddate);
    gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry
    free (moddate);

    // add metadata to define number of corrections
    char line[128];
    snprintf (line, 60, "Chip "OFF_T_FMT", Warp "OFF_T_FMT", Stack "OFF_T_FMT, result.NdelChip, result.NdelWarp, result.NdelStack);
    gfits_modify (&catalog.header, "DELETE_1", "%s", 1, line);
    snprintf (line, 60, "Other "OFF_T_FMT", Measure "OFF_T_FMT", Average "OFF_T_FMT, result.NdelOther, result.NdelMeas, result.NdelAves);
    gfits_modify (&catalog.header, "DELETE_2", "%s", 1, line);

    // save backup of original cpm file
    if (!dvo_catalog_subset_backup (&catalog, BACKUP_EXTNAME)) {
      fprintf (stderr, "ERROR: failed to make backup cpt table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_subset_backup (catalog.measure_catalog, BACKUP_EXTNAME)) {
      fprintf (stderr, "ERROR: failed to make backup cpm table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_subset_backup (catalog.secfilt_catalog, BACKUP_EXTNAME)) {
      fprintf (stderr, "ERROR: failed to make backup cps table for catalog %s\n", catalog.filename);
      exit (1);
    }

    // XXX something of a hack : I only want to save average, measure, secfilt.  
    catalog.Nmissing = catalog.Nmissing_off;
    catalog.Nlensing = catalog.Nlensing_off;
    catalog.Nlensobj = catalog.Nlensobj_off;
    catalog.Nstarpar = catalog.Nstarpar_off;
    catalog.Ngalphot = catalog.Ngalphot_off;

    catalog.Nmissing_off = 0;
    catalog.Nlensing_off = 0;
    catalog.Nlensobj_off = 0;
    catalog.Nstarpar_off = 0;
    catalog.Ngalphot_off = 0;

    SetProtect (TRUE);
    dvo_catalog_save_complete (&catalog, VERBOSE2);
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }

  SkyTableFree (sky);
  SkyListFree (skylist);
  FreePhotcodeTable ();

  return TRUE;
}

// CATDIR is supplied globally
int delete_measures_by_match_parallel (SkyList *sky) {

  // launch the delstar_client jobs to the parallel hosts

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

    char *command = NULL;
    strextend (&command, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -delete-measures-by-match", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (DELETE_MIN_DET_ID) strextend (&command, "-delete-min-detID %d", DELETE_MIN_DET_ID);
    if (DELETE_MAX_DET_ID) strextend (&command, "-delete-max-detID %d", DELETE_MAX_DET_ID);
    if (DELETE_MIN_CAT_ID) strextend (&command, "-delete-min-catID %d", DELETE_MIN_CAT_ID);
    if (DELETE_MAX_CAT_ID) strextend (&command, "-delete-max-catID %d", DELETE_MAX_CAT_ID);

    if (DELETE_MIN_IMAGE_ID) strextend (&command, "-delete-min-imageID %d", DELETE_MIN_IMAGE_ID);
    if (DELETE_MAX_IMAGE_ID) strextend (&command, "-delete-max-imageID %d", DELETE_MAX_IMAGE_ID);
    if (DELETE_MIN_PHOTCODE) strextend (&command, "-delete-min-photcode %d", DELETE_MIN_PHOTCODE);
    if (DELETE_MAX_PHOTCODE) strextend (&command, "-delete-max-photcode %d", DELETE_MAX_PHOTCODE);
    if (DELETE_MIN_TIME)     {
      char *minDate = ohana_sec_to_date (DELETE_MIN_TIME);
      strextend (&command, "-delete-min-time %s", minDate);
      free (minDate);
    }
    if (DELETE_MAX_TIME) {
      char *maxDate = ohana_sec_to_date (DELETE_MAX_TIME);
      strextend (&command, "-delete-max-time %s", maxDate);
      free (maxDate);
    }

    if (VERBOSE)    	 { strextend (&command, "-v");                  }
    if (VERBOSE2)   	 { strextend (&command, "-vv");                 }
    if (SINGLE_CPT) 	 { strextend (&command, "-cpt %s", SINGLE_CPT); }
    if (UPDATE)     	 { strextend (&command, "-update");             }
    if (SAVE_DELETES)    { strextend (&command, "-save-deletes");    }

    if (BACKUP_EXTNAME)  { strextend (&command, "-backup-extname %s", BACKUP_EXTNAME);    }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running delstar_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  FreeHostTable(table);

  return TRUE;
}

DeleteMeasureResult delete_measures_by_match_catalog (Catalog *catalog) {

  off_t i, n, m, N, D, currentAve;

  Measure *measureOut = NULL;
  Average *averageOut = NULL;
  SecFilt *secfiltOut = NULL;

  off_t *measureDrop, *measureSeqRaw, *measureSeqOut, *measureRefOut, *measureAveRaw, *averageNmeas, *averageDmeas, *averageSeqOut, *averageRefOut, *measureAveOut;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  /* internal counters */
  off_t Nmeasure = catalog[0].Nmeasure;
  off_t Naverage = catalog[0].Naverage;

  Measure *measure = catalog[0].measure;
  Average *average = catalog[0].average;
  SecFilt *secfilt = catalog[0].secfilt;
  
  if (VERBOSE) fprintf (stderr, "starting with Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT"\n",  catalog[0].Naverage,  catalog[0].Nmeasure);

  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // measure[i].averef -> average[averef]
  // measure[i].objID = average[averef].objID
  // measure[i].catID = average[averef].catID

  // we want a sorted measure array with all averef entries in sequence, skipping the entries which match our criteria

  // arrays 
  ALLOCATE (measureDrop,   off_t, Nmeasure); // mark measures to drop (0 == keep, 1 == drop)
  ALLOCATE (measureSeqRaw, off_t, Nmeasure); // original sequence of measures (0 ... Nmeasure - 1)
  ALLOCATE (measureAveRaw, off_t, Nmeasure); // averef values for original measure sequence (measure[N].averef : N = measureSeqRaw[i], averef = measureAveRaw[i]

  ALLOCATE (measureSeqOut, off_t, Nmeasure); // sequence of measures for output (resorted by averef and culled)
  ALLOCATE (measureRefOut, off_t, Nmeasure); // reference to original sequence for new output sequence
  ALLOCATE (averageNmeas,  off_t, Naverage); // output number of measures for each average (average.Nmeasure)
  ALLOCATE (averageDmeas,  off_t, Naverage); // output starting point of measures for each average (average.measureOffset)
  ALLOCATE (averageSeqOut, off_t, Naverage); // output sequence for averages
  ALLOCATE (averageRefOut, off_t, Naverage); // raw references for output sequence 
  // NOTE since we no longer drop any averages even if they have no measures, these indexes are probably not needed

  memset (measureDrop, 0, sizeof(off_t) * Nmeasure);

  for (i = 0; i < Naverage; i++) {
    averageNmeas[i] = 0;
    averageDmeas[i] = -1;
  }

# if (1)
  FILE *fsave = NULL;
  if (SAVE_DELETES) {
    char savename[DVO_MAX_PATH];
    snprintf (savename, DVO_MAX_PATH, "%s.save.0913", catalog->filename);
    struct stat filestat;
    int myStatus = stat (savename, &filestat);
    if (myStatus && (errno == ENOENT)) { 
      fsave = fopen (savename, "w");
      if (!fsave) {
	fprintf (stderr, "problem opening file %s for output\n", savename);
      }
    } else {
      if (!myStatus) {
	fprintf (stderr, "file %s exists: will not overwrite\n", savename);
      }
    }
  }    
# endif

  DeleteMeasureResult result;
  result.NdelWarp = 0;
  result.NdelChip = 0;
  result.NdelStack = 0;
  result.NdelOther = 0;
  result.NdelAves = 0;
  result.NdelMeas = 0;
  
  // mark the measures to be dropped
  for (i = 0; i < Nmeasure; i++) {

    // does this measure match our selection criteria?
    if (measure[i].detID > DELETE_MAX_DET_ID) continue;
    if (measure[i].detID < DELETE_MIN_DET_ID) continue;

    if (measure[i].catID > DELETE_MAX_CAT_ID) continue;
    if (measure[i].catID < DELETE_MIN_CAT_ID) continue;

    if (measure[i].imageID > DELETE_MAX_IMAGE_ID) continue;
    if (measure[i].imageID < DELETE_MIN_IMAGE_ID) continue;

    if (measure[i].photcode > DELETE_MAX_PHOTCODE) continue;
    if (measure[i].photcode < DELETE_MIN_PHOTCODE) continue;

    if (measure[i].t > DELETE_MAX_TIME) continue;
    if (measure[i].t < DELETE_MIN_TIME) continue;

    measureDrop[i] = TRUE;
    off_t N = measure[i].averef;
    if (VERBOSE) fprintf (stderr, "0x%08x 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[i].imageID, measure[i].detID, measure[i].catID, average[N].R, average[N].D, measure[i].photcode);
    if (fsave) {
      fprintf (fsave, "0x%08x 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[i].imageID, measure[i].detID, measure[i].catID, average[N].R, average[N].D, measure[i].photcode);
    }
    if (isGPC1chip(measure[i].photcode)) {
      result.NdelChip ++;
      continue;
    } 
    if (isGPC1warp(measure[i].photcode)) {
      result.NdelWarp ++;
      continue;
    } 
    if (isGPC1stack(measure[i].photcode)) {
      result.NdelStack ++;
      continue;
    } 
    result.NdelOther ++;
  }
  
  if (fsave) fclose (fsave);

  // set up the measure sequence lists
  for (i = 0; i < Nmeasure; i++) {
    measureSeqRaw[i] = i;
    measureAveRaw[i] = measure[i].averef;
  }

  // sort measureSeqRaw and measureAveRaw in order of measureAveRaw
  SortAveMeasMatch(measureSeqRaw, measureAveRaw, Nmeasure);

  // generate the measureSeqOut array
  off_t NmeasOut = 0;
  for (i = 0; i < Nmeasure; i++) {
    off_t j = measureSeqRaw[i];
    if (measureDrop[j]) continue;
    measureSeqOut[NmeasOut] = j;
    measureRefOut[j] = NmeasOut;
    NmeasOut ++;
  }
  // n = measureSeqOut[i] : measureOut[i] = measure[n]  (i = 0 -- NmeasOut, n = 0 -- Nmeasure)
  // n = measureRefOut[i] : measureOut[n] = measure[i]
  ALLOCATE (measureAveOut, off_t, NmeasOut);

  // count the number of measures for each averef
  N =  0;
  D = -1;
  currentAve = measureAveRaw[0];
  for (i = 0; i < Nmeasure; i++) {
    if (measureAveRaw[i] != currentAve) {
      // we have hit the next entry in the list
      averageNmeas[currentAve] = N; // number of measures
      averageDmeas[currentAve] = D; // first measure 
      N =  0;
      D = -1;
      currentAve = measureAveRaw[i];
    }
    off_t j = measureSeqRaw[i];
    if (measureDrop[j]) continue;
    N++;
    if (D == -1) { 
      // first valid measureOut for this averef
      D = measureRefOut[j];
    }
  }
  averageNmeas[currentAve] = N; // number of measures
  averageDmeas[currentAve] = D; // first measure 

  // generate the new average sequence, skipping entries with no measurements
  // update 20160524 : keep entries with no measurements 
  // NOTE: with the above update, averageSeqOut = averageRefOut = 0 ... Naverage - 1
  off_t NaveOut = 0;
  for (i = 0; i < Naverage; i++) {
    // if (averageNmeas[i] == 0) continue;
    averageSeqOut[NaveOut] = i;
    averageRefOut[i] = NaveOut;
    NaveOut ++;
  }
  // n = averageSeqOut[i] : averageOut[i] = average[n]  (i = 0 -- NaveOut, n = 0 -- Naverage)
  // n = averageRefOut[i] : averageOut[n] = average[i]

  // generate the output averefs
  for (i = 0; i < NmeasOut; i++) {
    off_t j = measureSeqOut[i]; // measure[i] = measureOut[i]
    n = measureAveRaw[j]; // average[n] : measure[i]
    N = averageRefOut[n]; // averageOut[N] = average[n];
    measureAveOut[i] = N; // measureOut[i].averef = measureAveOut[i] 
  }

  // copy the (kept) measurements in the sorted order
  ALLOCATE (measureOut, Measure, NmeasOut);
  for (i = 0; i < NmeasOut; i++) {
    off_t j = measureSeqOut[i];
    measureOut[i] = measure[j];
    measureOut[i].averef = measureAveOut[i];
  }

  // copy the (kept) average entries
  ALLOCATE (averageOut, Average, NaveOut);
  for (i = 0; i < NaveOut; i++) {
    off_t j = averageSeqOut[i];
    averageOut[i] = average[j];
    averageOut[i].Nmeasure      = averageNmeas[j]; 
    averageOut[i].measureOffset = averageDmeas[j];
  }

  // copy the secfilt entries for the (kept) average entries
  ALLOCATE (secfiltOut, SecFilt, NaveOut*Nsecfilt);
  for (i = 0; i < NaveOut; i++) {
    off_t j = averageSeqOut[i];
    for (m = 0; m < Nsecfilt; m++) {
      secfiltOut[i*Nsecfilt + m] = secfilt[j*Nsecfilt + m];
    }
  }

  // update the values of average.measureOffset and average.Nmeasure
  FREE(measure);
  FREE(average);
  FREE(secfilt);
  catalog[0].measure = measureOut;
  catalog[0].average = averageOut;
  catalog[0].secfilt = secfiltOut;
  catalog[0].Nmeasure = NmeasOut;
  catalog[0].Naverage = NaveOut;

  // update catalog.Nmeasure and catalog.Naverage, double check 
  int NmeasureTotal = 0;
  int measureOffsetOK = TRUE;
  int averefOK = TRUE;
  int objIDisOK = TRUE;
  for (i = 0; i < NaveOut; i++) {
    NmeasureTotal += catalog[0].average[i].Nmeasure;
    if (VERBOSE2 && !(NmeasureTotal <= catalog[0].Nmeasure)) {
      fprintf (stderr, "too few measurements: %d %d %d\n", (int) i, NmeasureTotal, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset < catalog[0].Nmeasure);
    if (VERBOSE2 && !(catalog[0].average[i].measureOffset < catalog[0].Nmeasure)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure);
    if (VERBOSE2 && !(catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure)) {
      fprintf (stderr, "orrset + Nmeasure too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].measureOffset, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
    m = catalog[0].average[i].measureOffset;
    for (off_t j = 0; j < catalog[0].average[i].Nmeasure; j++) {
      averefOK &= (catalog[0].measure[m+j].averef == i);
      if (VERBOSE2 && !(catalog[0].measure[m+j].averef == i)) {
	fprintf (stderr, "averef broken: %d vs %d (measure %d)\n", (int) i, catalog[0].measure[m+j].averef, (int) (m+j));
      }
      objIDisOK &= (catalog[0].measure[m+j].objID == catalog[0].average[i].objID);
      if (VERBOSE2 && !(catalog[0].measure[m+j].objID == catalog[0].average[i].objID)) {
	fprintf (stderr, "objID broken: %d vs %d (average %d, measure %d)\n", (int) i, (int) (m + j), catalog[0].average[i].objID, catalog[0].measure[m+j].objID);
      }
    }
  }

  if (!measureOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid measureOffset\n", catalog[0].filename);
  }

  if (!averefOK) {
    fprintf (stderr, "ERROR: catalog %s has invalid averefs\n", catalog[0].filename);
  }

  if (NmeasureTotal != catalog[0].Nmeasure) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Nmeasure\n", catalog[0].filename);
  }

  if (!objIDisOK) {
    fprintf (stderr, "ERROR: catalog %s has invalid objIDs\n", catalog[0].filename);
  }

  // MARKTIME("  match time %9.4f sec for %7lld measures, %6lld average\n", dtime, (long long) Nmeasure, (long long) Naverage);

  catalog[0].sorted = TRUE;

  if (VERBOSE) fprintf (stderr, "ending with Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT"\n",  catalog[0].Naverage,  catalog[0].Nmeasure);

  result.NdelAves = Naverage - catalog[0].Naverage;
  result.NdelMeas = Nmeasure - catalog[0].Nmeasure;

  fprintf (stderr, "deleting from %s : "OFF_T_FMT" meas, "OFF_T_FMT" aves : "OFF_T_FMT" chip, "OFF_T_FMT" stack, "OFF_T_FMT" warp, "OFF_T_FMT" other\n", catalog[0].filename, result.NdelMeas, result.NdelAves, result.NdelChip, result.NdelStack, result.NdelWarp, result.NdelOther);

  FREE (measureDrop);
  FREE (measureSeqRaw);
  FREE (measureSeqOut);
  FREE (measureRefOut);
  FREE (measureAveRaw);
  FREE (averageNmeas);
  FREE (averageDmeas);
  FREE (averageSeqOut);
  FREE (averageRefOut);
  FREE (measureAveOut);

  return result;
}
