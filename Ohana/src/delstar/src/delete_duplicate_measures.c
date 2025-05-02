# include "delstar.h"
int isGPC1chip (int photcode);
int isGPC1warp (int photcode);
int isGPC1stack (int photcode);
int dvo_catalog_subset_backup (Catalog *catalog, char *suffix);

// this function identifies detections to be deleted as being duplicates based on imageID + detID

int delete_duplicate_measures () {

  int i;
  Catalog catalog;

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
    int status = delete_duplicate_measures_parallel (skylist);

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

    DeleteMeasureResult result = delete_duplicate_measures_catalog (&catalog);
    
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
    snprintf (history, 128, "delete duplicate measurements: %s", moddate);
    gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry
    free (moddate);

    // add metadata to define number of corrections
    char line[128];
    snprintf (line, 60, "Chip "OFF_T_FMT", Warp "OFF_T_FMT", Stack "OFF_T_FMT, result.NdelChip, result.NdelWarp, result.NdelStack);
    gfits_modify (&catalog.header, "DELETE_1", "%s", 1, line);
    snprintf (line, 60, "Other "OFF_T_FMT", Measure "OFF_T_FMT", Average "OFF_T_FMT, result.NdelOther, result.NdelMeas, result.NdelAves);
    gfits_modify (&catalog.header, "DELETE_2", "%s", 1, line);

    // save backup of original cpm file
    if (!dvo_catalog_subset_backup (&catalog, ".dlz")) {
      fprintf (stderr, "ERROR: failed to make backup cpt table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_subset_backup (catalog.measure_catalog, ".d1z")) {
      fprintf (stderr, "ERROR: failed to make backup cpm table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_subset_backup (catalog.secfilt_catalog, ".dlz")) {
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

int dvo_catalog_subset_backup (Catalog *catalog, char *suffix) {

  int dbstate;

  char tmpfilename[DVO_MAX_PATH];
  int status = snprintf (tmpfilename, DVO_MAX_PATH, "%s%s", catalog->filename, suffix);
  if (status >= DVO_MAX_PATH) {
    fprintf (stderr, "path name too long: %s\n", catalog->filename);
    return FALSE;
  }
      
  // play it safe: do not overwrite an existing backup file
  struct stat fileStats;
  status = stat (tmpfilename, &fileStats);
  if (!status) {
    fprintf (stderr, "ERROR: backup file %s already exists, exiting\n", tmpfilename);
    return FALSE;
  }
  
  // some error accessing the file.  there is only one acceptable error: file not found
  if (status && (errno != ENOENT)) {
    perror ("problem with output target");
    return FALSE;
  }

  if (fflush (catalog[0].f)) {
    perror ("fflush: ");
    fprintf (stderr, "failed to flush file %s\n", catalog[0].filename);
    return FALSE;
  }

  // closes f but does not set back to NULL
  if (!fclearlockfile (catalog[0].filename, catalog[0].f, catalog[0].lockmode, &dbstate)) {
    fprintf (stderr, "failed to unlock or close file\n");
    return FALSE;
  }

  status = rename (catalog->filename, tmpfilename);
  if (status) {
    fprintf (stderr, "failed to rename catalog %s\n", catalog->filename);
    return FALSE;
  }

  // re-lock file, create stream f 
  catalog[0].f = fsetlockfile (catalog[0].filename, 3600.0, catalog[0].lockmode, &dbstate);
  if (catalog[0].f == NULL)   return FALSE;
  if (dbstate != LCK_EMPTY)   return FALSE;

  if (fseeko (catalog[0].f, 0, SEEK_SET)) {
    perror ("fseeko: ");
    return FALSE;
  }
  
  return TRUE;
}

// CATDIR is supplied globally
int delete_duplicate_measures_parallel (SkyList *sky) {

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
    strextend (&command, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -dup-measures", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)    	 { strextend (&command, "-v");                  }
    if (VERBOSE2)   	 { strextend (&command, "-vv");                 }
    if (SINGLE_CPT) 	 { strextend (&command, "-cpt %s", SINGLE_CPT); }
    if (UPDATE)     	 { strextend (&command, "-update");             }
    if (SAVE_DUPLICATES) { strextend (&command, "-save-duplicates");    }

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

DeleteMeasureResult delete_duplicate_measures_catalog (Catalog *catalog) {

  off_t i, j, n, m, N, D, currentAve;

  Measure *measureOut = NULL;
  Average *averageOut = NULL;
  SecFilt *secfiltOut = NULL;

  off_t *measureDrop, *measureSeqRaw, *measureSeqOut, *measureRefOut, *measureAveRaw, *averageNmeas, *averageDmeas, *averageSeqOut, *averageRefOut, *measureAveOut;
  uint64_t *fullID;
  int *seq, *duplicates;

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
  ALLOCATE (measureDrop,   off_t, Nmeasure);
  ALLOCATE (measureSeqRaw, off_t, Nmeasure);
  ALLOCATE (measureSeqOut, off_t, Nmeasure);
  ALLOCATE (measureRefOut, off_t, Nmeasure);
  ALLOCATE (measureAveRaw, off_t, Nmeasure);
  ALLOCATE (averageNmeas,  off_t, Naverage);
  ALLOCATE (averageDmeas,  off_t, Naverage);
  ALLOCATE (averageSeqOut, off_t, Naverage);
  ALLOCATE (averageRefOut, off_t, Naverage);

  ALLOCATE (fullID,     uint64_t, Nmeasure);
  ALLOCATE (seq,             int, Nmeasure);
  ALLOCATE (duplicates,      int, Nmeasure);

  memset (measureDrop, 0, sizeof(off_t) * Nmeasure);
  memset (duplicates,  0, sizeof(int) * Nmeasure);

  for (i = 0; i < Naverage; i++) {
    averageNmeas[i] = 0;
    averageDmeas[i] = -1;
  }

  // make an array of the full IDs
  for (i = 0; i < Nmeasure; i++) {
    fullID[i] = ((uint64_t) (measure[i].imageID) << 32) | (measure[i].detID);
    seq[i] = i;
  }

  // sort the array and sequence number
  sort_fullIDs (fullID, seq, catalog[0].Nmeasure);

  uint64_t current = fullID[0];	      // current unique value
  int count = 0;		      // number of duplicates for the current unique value
  duplicates[0] = count;	      // duplicate sequence

  for (i = 1; i < catalog[0].Nmeasure; i++) {
    off_t j = seq[i];
    int newID = (measure[j].imageID == 0) || (fullID[i] != current);
    if (newID) {
      count = 0;
      current = fullID[i];
    } else {
      count ++;
    }
    duplicates[i] = count;
  }
    
# if (1)
  FILE *fsave = NULL;
  if (SAVE_DUPLICATES) {
    char savename[DVO_MAX_PATH];
    snprintf (savename, DVO_MAX_PATH, "%s.save.0914", catalog->filename);
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
    if (!duplicates[i]) continue;
    off_t j = seq[i];
    measureDrop[j] = TRUE;
    off_t N = measure[j].averef;
    if (VERBOSE) fprintf (stderr, "0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
    if (fsave) {
      fprintf (fsave, "0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
    }
    if (isGPC1chip(measure[j].photcode)) {
      result.NdelChip ++;
      continue;
    } 
    if (isGPC1warp(measure[j].photcode)) {
      result.NdelWarp ++;
      continue;
    } 
    if (isGPC1stack(measure[j].photcode)) {
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
    j = measureSeqRaw[i];
    if (measureDrop[j]) {
	continue;
    }
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
    j = measureSeqRaw[i];
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
    j = measureSeqOut[i]; // measure[j] = measureOut[i]
    n = measureAveRaw[j]; // average[n] : measure[j]
    N = averageRefOut[n]; // averageOut[N] = average[n];
    measureAveOut[i] = N; // measureOut[i].averef = measureAveOut[i] 
  }

  // copy the (kept) measurements in the sorted order
  ALLOCATE (measureOut, Measure, NmeasOut);
  for (i = 0; i < NmeasOut; i++) {
    j = measureSeqOut[i];
    measureOut[i] = measure[j];
    measureOut[i].averef = measureAveOut[i];
  }

  // copy the (kept) average entries
  ALLOCATE (averageOut, Average, NaveOut);
  for (i = 0; i < NaveOut; i++) {
    j = averageSeqOut[i];
    averageOut[i] = average[j];
    averageOut[i].Nmeasure      = averageNmeas[j]; 
    averageOut[i].measureOffset = averageDmeas[j];
  }

  // copy the secfilt entries for the (kept) average entries
  ALLOCATE (secfiltOut, SecFilt, NaveOut*Nsecfilt);
  for (i = 0; i < NaveOut; i++) {
    j = averageSeqOut[i];
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
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {
      averefOK &= (catalog[0].measure[m+j].averef == i);
      if (VERBOSE2 && !(catalog[0].measure[m+j].averef == i)) {
	fprintf (stderr, "averef broken: %d vs %d (measure %d)\n", (int) i, catalog[0].measure[m+j].averef, (int) (m+j));
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

  // MARKTIME("  match time %9.4f sec for %7lld measures, %6lld average\n", dtime, (long long) Nmeasure, (long long) Naverage);

  catalog[0].sorted = TRUE;

  if (VERBOSE) fprintf (stderr, "ending with Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT"\n",  catalog[0].Naverage,  catalog[0].Nmeasure);

  result.NdelAves = Naverage - catalog[0].Naverage;
  result.NdelMeas = Nmeasure - catalog[0].Nmeasure;

  fprintf (stderr, "deleting from %s : "OFF_T_FMT" meas, "OFF_T_FMT" aves : "OFF_T_FMT" chip, "OFF_T_FMT" stack, "OFF_T_FMT" warp, "OFF_T_FMT" other\n", catalog[0].filename, result.NdelMeas, result.NdelAves, result.NdelChip, result.NdelStack, result.NdelWarp, result.NdelOther);

  FREE (fullID);
  FREE (seq);
  FREE (duplicates);

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

// sort fullId vector and an index vector
void sort_fullIDs (uint64_t *I, int *S, off_t N) {

# define SWAPFUNC(A,B){ uint64_t tmp_i; int tmp_s;	\
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
  tmp_s = S[A]; S[A] = S[B]; S[B] = tmp_s; \
}
# define COMPARE(A,B)(I[A] < I[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// for now (20140710) I need to identify gpc1 chips explicitly.  generalize in the future
int isGPC1chip (int photcode) {

  if ((photcode > 10000) && (photcode < 10077)) return TRUE; // g-band
  if ((photcode > 10100) && (photcode < 10177)) return TRUE; // r-band
  if ((photcode > 10200) && (photcode < 10277)) return TRUE; // i-band
  if ((photcode > 10300) && (photcode < 10377)) return TRUE; // z-band
  if ((photcode > 10400) && (photcode < 10477)) return TRUE; // y-band
  if ((photcode > 10500) && (photcode < 10577)) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1stack (int photcode) {

  if (photcode == 11000) return TRUE; // g-band
  if (photcode == 11100) return TRUE; // r-band
  if (photcode == 11200) return TRUE; // i-band
  if (photcode == 11300) return TRUE; // z-band
  if (photcode == 11400) return TRUE; // y-band
  if (photcode == 11500) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1warp (int photcode) {

  if (photcode == 12000) return TRUE; // g-band
  if (photcode == 12100) return TRUE; // r-band
  if (photcode == 12200) return TRUE; // i-band
  if (photcode == 12300) return TRUE; // z-band
  if (photcode == 12400) return TRUE; // y-band
  if (photcode == 12500) return TRUE; // w-band

  return FALSE;
}
