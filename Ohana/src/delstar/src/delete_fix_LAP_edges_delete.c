# include "delstar.h"

// this function deletes identified detections

int delete_fix_LAP_edges_delete () {

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
  SkyList *skylist = SINGLE_CPT ? SkyRegionByCPT (sky, SINGLE_CPT) : SkyListByPatch (sky, -1, &UserPatch);
  if (!skylist) {
    fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
    exit (2);
  }

  // launch the remote jobs
  if (PARALLEL && !HOST_ID) {
    int status = delete_fix_LAP_edges_delete_parallel (skylist);
    return status;
  }

  // load the detections to delete
  FILE *f = fopen (EDGE_DELETIONS, "r");
  if (!f) {
    fprintf (stderr, "ERROR: failed to read the list of detections to be deleted\n");
    exit (1);
  }

  // XXX accumulate detections in a local structure:
  off_t Nmeasure_edge = 0;
  off_t NMEASURE_EDGE = 1000;
  MeasureEdge *measure_edge = NULL;
  ALLOCATE (measure_edge, MeasureEdge, NMEASURE_EDGE);
  char line[1024];
  while (scan_line(f, line) != EOF) {
    double ra, dec;
    int imageID, detID, objID, catID;
    iparse (&imageID, 1, line);
    iparse (&detID,   2, line);
    iparse (&objID,   4, line);
    iparse (&catID,   5, line);
    dparse (&ra,      7, line);
    dparse (&dec,     8, line);
    measure_edge[Nmeasure_edge].imageID = imageID;
    measure_edge[Nmeasure_edge].detID   = detID;
    measure_edge[Nmeasure_edge].objID   = objID;
    measure_edge[Nmeasure_edge].catID   = catID;
    measure_edge[Nmeasure_edge].R       = ra;
    measure_edge[Nmeasure_edge].D       = dec;
    Nmeasure_edge ++;
    CHECK_REALLOCATE (measure_edge, MeasureEdge, NMEASURE_EDGE, Nmeasure_edge, 1000);
  }
  // sort_measure_edge_by_catID (measure_edge, Nmeasure_edge);

  // create a catID count table
  int maxCatID = 0;
  for (i = 0; i < Nmeasure_edge; i++) {
    maxCatID = MAX(maxCatID, measure_edge[i].catID);
  }
  
  // catIDcount[N] = count(catID == N)
  int *catIDcount = NULL;
  ALLOCATE (catIDcount, int, maxCatID + 1);
  memset (catIDcount, 0, (maxCatID + 1) * sizeof(int));
  for (i = 0; i < Nmeasure_edge; i++) {
    catIDcount[measure_edge[i].catID] ++;
  }

  // delete detections from region
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    int myCatID = skylist[0].regions[i][0].index;
    if (myCatID > maxCatID) continue;
    if (!catIDcount[myCatID]) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf_nowarn (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : skylist[0].filename[i];
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

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

    off_t NaverageStart = catalog.Naverage;
    off_t NmeasureStart = catalog.Nmeasure;
    delete_fix_LAP_edges_drop_measures (&catalog, measure_edge, Nmeasure_edge, catIDcount[myCatID]);

    // skip if nothing was deleted
    int noChange = (NaverageStart == catalog.Naverage) && (NmeasureStart == catalog.Nmeasure);
    if (UPDATE && !noChange) {
      // XXX save a backup copy first
      // dvo_catalog_backup (&catalog, "~", TRUE);
      SetProtect (TRUE);
      dvo_catalog_save_complete (&catalog, VERBOSE2);
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }
  free (measure_edge);
  free (catIDcount);

  return TRUE;
}

// CATDIR is supplied globally
int delete_fix_LAP_edges_delete_parallel (SkyList *sky) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // launch the delstar_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  { 
    // ensure that the paths are absolute path names
    char *tmppath = abspath (EDGE_DELETIONS, DVO_MAX_PATH);
    free (EDGE_DELETIONS);
    EDGE_DELETIONS = tmppath;
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

    // ALLOCATE (table->hosts[i].results, char, 1024);
    // snprintf_nowarn (table->hosts[i].results, 1024, "%s/delstar.fixLAPedges.measures.%s.dat", table->hosts[i].pathname, uniquer);

    char command[1024];
    snprintf_nowarn (command, 1024, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -fix-LAP-edges-delete %s", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname,
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax,
	      EDGE_DELETIONS);

    char tmpline[1024];
    if (VERBOSE)    { snprintf_nowarn (tmpline, 1024, "%s -v",      command);             strcpy (command, tmpline); }
    if (VERBOSE2)   { snprintf_nowarn (tmpline, 1024, "%s -vv",     command);             strcpy (command, tmpline); }
    if (SINGLE_CPT) { snprintf_nowarn (tmpline, 1024, "%s -cpt %s", command, SINGLE_CPT); strcpy (command, tmpline); }
    if (UPDATE)     { snprintf_nowarn (tmpline, 1024, "%s -update", command);             strcpy (command, tmpline); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

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
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}

int delete_fix_LAP_edges_drop_measures (Catalog *catalog, MeasureEdge *measure_edge, off_t Nmeasure_edge, int catIDcount) {

  off_t i, j, n, m, N, D, currentAve;

  myAssert (catIDcount > 0, "oops : nothing to delete?");

  // get a list of detections values corresponding to this catID. 
  // there should be catIDcount of them

  int myCatID = catalog[0].catID;

  int NbadMeasures = 0;
  int *badMeasures = NULL;
  ALLOCATE (badMeasures, int, catIDcount);

  for (i = 0; i < Nmeasure_edge; i++) {
    if (measure_edge[i].catID != myCatID) continue;
    myAssert (NbadMeasures < catIDcount, "too many bad measures?");
    badMeasures[NbadMeasures] = i;
    NbadMeasures ++;
  }

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
  ALLOCATE (measureDrop,   off_t, Nmeasure);
  memset (measureDrop, 0, sizeof(off_t) * Nmeasure);
  
  int invalid = FALSE;
  int Ndrop = 0;
  for (i = 0; i < Nmeasure; i++) {
    for (j = 0; j < NbadMeasures; j++) {
      int seq = badMeasures[j];
      if (measure[i].imageID != measure_edge[seq].imageID) continue;
      if (measure[i].detID   != measure_edge[seq].detID) continue;
      // this should be a measure to delete
      if (measure[i].objID != measure_edge[seq].objID) {
	fprintf (stderr, "ERROR: this is NOT the valid match!\n");
	invalid = TRUE;
	continue;
      }
      if (measure[i].catID != measure_edge[seq].catID) {
	fprintf (stderr, "ERROR: this is NOT the valid match!\n");
	invalid = TRUE;
	continue;
      }
      measureDrop[i] = TRUE;
      Ndrop ++;
    }
  }

  if (Ndrop == 0) {
    fprintf (stderr, "duplicates already deleted for %s\n", catalog[0].filename);
    free (measureDrop);
    free (badMeasures);
    return FALSE;
  } else { 
    if (Ndrop != NbadMeasures) {
      fprintf (stderr, "ERROR: wrong number of bad detections!\n");
      invalid = TRUE;
    }
  }

  if (invalid) {
    fprintf (stderr, "ERROR: bad detection selection seems wrong, aborting\n");
    exit (2);
  }

  ALLOCATE (measureSeqRaw, off_t, Nmeasure);
  ALLOCATE (measureSeqOut, off_t, Nmeasure);
  ALLOCATE (measureRefOut, off_t, Nmeasure);
  ALLOCATE (measureAveRaw, off_t, Nmeasure);
  ALLOCATE (averageNmeas,  off_t, Naverage);
  ALLOCATE (averageDmeas,  off_t, Naverage);
  ALLOCATE (averageSeqOut, off_t, Naverage);
  ALLOCATE (averageRefOut, off_t, Naverage);

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
  off_t NaveOut = 0;
  for (i = 0; i < Naverage; i++) {
    if (averageNmeas[i] == 0) continue;
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
      fprintf (stderr, "offset + Nmeasure too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].measureOffset, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
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

  off_t NdelAves = Naverage - catalog[0].Naverage;
  off_t NdelMeas = Nmeasure - catalog[0].Nmeasure;
  if (NdelAves || NdelMeas) {
    fprintf (stderr, "deleting "OFF_T_FMT" measures and "OFF_T_FMT" averages: %s\n",  NdelMeas, NdelAves, catalog[0].filename);
  }

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
  FREE (badMeasures);

  return TRUE;
}
