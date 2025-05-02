# include "delstar.h"

int delete_photcodes () {

  int i;
  Catalog catalog;

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
    exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);
  if (!skylist) {
    fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
    exit (2);
  }

  // launch the remote jobs
  if (PARALLEL && !HOST_ID) {
    int status = delete_photcodes_parallel (skylist);
    return status;
  }

  // xxx where does this go?
  int Nphotcodes = 0;
  PhotCode **photcodes = ParsePhotcodeList (PHOTCODE_LIST, &Nphotcodes, FALSE);

  // delete detections from region
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

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

    delete_photcodes_catalog (&catalog, photcodes, Nphotcodes);
    SetProtect (TRUE);
    dvo_catalog_save_complete (&catalog, VERBOSE2);
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }
  return TRUE;
}

// CATDIR is supplied globally
# define DEBUG 1
int delete_photcodes_parallel (SkyList *sky) {

  // launch the delstar_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // options / arguments that can affect relphot_client -update-objects:
    // VERBOSE

    char command[1024];
    snprintf_nowarn (command, 1024, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -photcodes %s", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax, 
	      PHOTCODE_LIST);

    char tmpline[1024];
    if (VERBOSE)       { snprintf_nowarn (tmpline, 1024, "%s -v",              command);                    strcpy (command, tmpline); }
    if (VERBOSE2)      { snprintf_nowarn (tmpline, 1024, "%s -vv",             command);                    strcpy (command, tmpline); }

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
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the delstar_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      

// sort the measure Sequence based on the average Sequence entries
void SortAveMeasMatch (off_t *MEAS, off_t *AVE, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp_meas; off_t tmp_ave;	\
    tmp_meas = MEAS[A]; MEAS[A] = MEAS[B]; MEAS[B] = tmp_meas;		\
    tmp_ave  = AVE[A];  AVE[A]  = AVE[B];  AVE[B]  = tmp_ave;		\
  }
# define COMPARE(A,B)(AVE[A] < AVE[B])
  OHANA_SORT (N, COMPARE, SWAPFUNC);
# undef SWAPFUNC
# undef COMPARE
}

int delete_photcodes_catalog (Catalog *catalog, PhotCode **photcodes, int Nphotcodes) {

  off_t i, j, n, m, N, D, currentAve;

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
  ALLOCATE (measureSeqRaw, off_t, Nmeasure);
  ALLOCATE (measureSeqOut, off_t, Nmeasure);
  ALLOCATE (measureRefOut, off_t, Nmeasure);
  ALLOCATE (measureAveRaw, off_t, Nmeasure);
  ALLOCATE (averageNmeas,  off_t, Naverage);
  ALLOCATE (averageDmeas,  off_t, Naverage);
  ALLOCATE (averageSeqOut, off_t, Naverage);
  ALLOCATE (averageRefOut, off_t, Naverage);

  // mark the measures to be dropped
  for (i = 0; i < Nmeasure; i++) {
    int drop = FALSE;
    for (j = 0; !drop && (j < Nphotcodes); j++) {
      drop |= (photcodes[j][0].code == measure[i].photcode);
    }
    measureDrop[i] = drop;
  }

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

  return TRUE;
}
