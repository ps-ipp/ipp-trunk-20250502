# include "delstar.h"

// this function identifies detections to be deleted as being duplicates based on imageID + detID

int delete_fix_LAP (ImageSubset *image, off_t Nimage) {

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

  // this is needed in the master and client jobs (in master, just so Nimage is set in ImageOpsFixLAP.c)
  initImageIndex (image, Nimage);

  // launch the remote jobs
  if (PARALLEL && !HOST_ID) {
    int status = delete_fix_LAP_parallel (skylist, image, Nimage);
    return status;
  }

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

    off_t NaverageStart = catalog.Naverage;
    off_t NmeasureStart = catalog.Nmeasure;
    delete_fix_LAP_catalog (&catalog, image, Nimage);

    // skip if nothing was deleted
    int noChange = (NaverageStart == catalog.Naverage) && (NmeasureStart == catalog.Nmeasure);
    if (UPDATE && !noChange) {
      SetProtect (TRUE);
      dvo_catalog_save_complete (&catalog, VERBOSE2);
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }

  ImageValidSave (IMSTATS_FILE);

  return TRUE;
}

// CATDIR is supplied globally
int delete_fix_LAP_parallel (SkyList *sky, ImageSubset *image, off_t Nimage) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // write out the subset table of image information
  char imageFile[512];
  snprintf_nowarn (imageFile, 512, "%s/delstar.fixLAP.%s.dat", CATDIR, uniquer);

  if (!ImageSubsetSave (imageFile, image, Nimage)) {
    fprintf (stderr, "failed to write image subset\n");
    exit (1);
  }

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

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf_nowarn (table->hosts[i].results, 1024, "%s/delstar.fixLAP.imstats.%s.dat", table->hosts[i].pathname, uniquer);

    char command[1024];
    snprintf_nowarn (command, 1024, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -images %s -imstats %s -region %f %f %f %f -fix-LAP", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, imageFile, table->hosts[i].results,
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

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

  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i].results) continue;
    if (!ImageValidLoad (table->hosts[i].results)) {
      fprintf (stderr, "failed to read data from %s\n", table->hosts[i].hostname);
    }
  }

  return TRUE;
}

// CATDIR is supplied globally
int delete_fix_LAP_setstats (ImageSubset *image, off_t Nimage) {

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
  fprintf (stderr, "setup sky\n");

  // this is needed in the master and client jobs (in master, just so Nimage is set in ImageOpsFixLAP.c)
  initImageIndex (image, Nimage);
  fprintf (stderr, "make index\n");

  // write out the subset table of image information
  char imageFile[512];
  snprintf_nowarn (imageFile, 512, "%s/delstar.fixLAP.%s.dat", CATDIR, UNIQUER);

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

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf_nowarn (table->hosts[i].results, 1024, "%s/delstar.fixLAP.imstats.%s.dat", table->hosts[i].pathname, UNIQUER);

    fprintf (stderr, "read %s\n", table->hosts[i].results);
    if (!ImageValidLoad (table->hosts[i].results)) {
      fprintf (stderr, "failed to read data from %s\n", table->hosts[i].hostname);
    }
  }

  return TRUE;
}

int UnpackPSPSStackDetectionID(int *sourceID, int *imageID, int *detID, uint64_t pspsStackID);

int Nvalid = 0;
int Ninvalid = 0;

int delete_fix_LAP_catalog (Catalog *catalog, ImageSubset *image, off_t Nimage) {

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
  
  Nvalid = 0;
  Ninvalid = 0;
  if (VERBOSE) fprintf (stderr, "starting with Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT"\n",  catalog[0].Naverage,  catalog[0].Nmeasure);

  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // measure[i].averef -> average[averef]
  // measure[i].objID = average[averef].objID
  // measure[i].catID = average[averef].catID

  // we want a sorted measure array with all averef entries in sequence, skipping the entries which match our criteria

  // arrays 
  ALLOCATE (measureDrop,   off_t, Nmeasure);
  delete_fix_LAP_measures (measureDrop, catalog, image, Nimage);

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
    fprintf (stderr, "deleting "OFF_T_FMT" measures and "OFF_T_FMT" averages (%d valid, %d invalid): %s\n",  NdelMeas, NdelAves, Nvalid, Ninvalid, catalog[0].filename);
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

  return TRUE;
}

// this function makes the needed modifications to the measure table (to imageIDs) and updates
// the array measureDrop to mark the entries we want to remove

// needed modifications:
// * delete all photcodes corresponding to 2mass, synth, or wise (not gpc1 or stack)
// * delete all duplicates
// * check the imageID 
// * re-assign the imageID if needed

int delete_fix_LAP_measures (off_t *measureDrop, Catalog *catalog, ImageSubset *image, off_t Nimage) {
  OHANA_UNUSED_PARAM(Nimage);

  /* internal counters */
  off_t Nmeasure = catalog[0].Nmeasure;

  Measure *measure = catalog[0].measure;
  // Average *average = catalog[0].average;
  
  uint64_t *fullID;
  int *seq, *duplicates;

  ALLOCATE (fullID,     uint64_t, Nmeasure);
  ALLOCATE (seq,             int, Nmeasure);
  ALLOCATE (duplicates,      int, Nmeasure);

  memset (measureDrop, 0, sizeof(off_t) * Nmeasure);
  memset (duplicates,  0, sizeof(int) * Nmeasure);

  // make an array of the full IDs
  off_t i;
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
    
  // mark the measures to be dropped
  for (i = 0; i < Nmeasure; i++) {
    off_t j = seq[i];

    int keep = TRUE;
    if (duplicates[i]) keep = FALSE;
    if (measure[j].photcode < 10000) keep = FALSE;  // Drop 2MASS, WISE, SYNTH here

    if (!keep) {
      measureDrop[j] = TRUE;
      // off_t N = measure[j].averef;
      // fprintf (stderr, "0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
    }
  }

  int maxID = 0;
  int *imageIndex = getImageIndex(&maxID);

  // check the image IDs for the remaining measures
  for (i = 0; i < Nmeasure; i++) {
    off_t j = seq[i];
    if (measureDrop[j]) continue;

    // for gpc1 exposures, 10000 < photcode < 10600
    if ((10000 < measure[j].photcode) && (measure[j].photcode < 10600)) {
      int resetID = FALSE;
      off_t imageID = measure[j].imageID;
      if (imageID == 0) {
	resetID = TRUE;
	goto resetID_exp;
      }
      if (imageID > maxID) {
	fprintf (stderr, "invalid image ID "OFF_T_FMT" for catalog %s\n", imageID, catalog[0].filename);
	exit (2);
      }
      off_t imageN = imageIndex[imageID];
      if (imageN < 0) {
	// image ID for this measure is not in the image table
	resetID = TRUE;
      }
      if ((measure[j].t < image[imageN].tmin) || (measure[j].t > image[imageN].tmax)) {
	// measure.imageID does not match correct image
	resetID = TRUE;
      }
      if (measure[j].photcode != image[imageN].photcode) {
	// measure.imageID does not match correct image
	resetID = TRUE;
      }

      if (!resetID) {
	// off_t N = measure[j].averef;
	// fprintf (stderr, "no repair 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
	BumpValidImage (imageN);
	Nvalid ++;
	  continue;
      }	

    resetID_exp:
      if (resetID) {
	// off_t N = measure[j].averef;
	// fprintf (stderr, "repair 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
	// update the image ID
	short photcode = measure[j].photcode;
	e_time time = measure[j].t;
	
	off_t imageID, Seq;
	if (!FindIDexp (&imageID, &Seq, time, photcode)) {
	  fprintf (stderr, "error cannot find matching image ID\n");
	  exit (5);
	}
	if (imageID <= 0) {
	  fprintf (stderr, "error cannot find matching image ID\n");
	  exit (5);
	}
	measure[j].imageID = imageID;
	BumpInvalidImage (Seq);
	Ninvalid ++;
	continue;
      }
    }

    // for gpc1 exposures, 10000 < photcode < 10600
    if ((11000 <= measure[j].photcode) && (measure[j].photcode < 11600)) {
      int sourceID = -1;
      int externID = -1;
      int detID = -1;

      int resetID = FALSE;
      off_t imageID = measure[j].imageID;
      if (imageID == 0) {
	resetID = TRUE;
	goto resetID_stk;
      }
      if (imageID > maxID) {
	fprintf (stderr, "invalid image ID "OFF_T_FMT" for catalog %s\n", imageID, catalog[0].filename);
	exit (2);
      }

      off_t imageN = imageIndex[imageID];
      if (imageN < 0) {
	// image ID for this measure is not in the image table
	resetID = TRUE;
      }

      UnpackPSPSStackDetectionID (&sourceID, &externID, &detID, measure[j].extID);
      if (externID != image[imageN].externID) {
	// measure.imageID does not match correct image
	resetID = TRUE;
      }
      if (measure[j].photcode != image[imageN].photcode) {
	// measure.imageID does not match correct image
	resetID = TRUE;
      }

      if (!resetID) {
	// off_t N = measure[j].averef;
	// fprintf (stderr, "no repair 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
	BumpValidImage (imageN);
	Nvalid ++;
	continue;
      }	

    resetID_stk:
      if (resetID) {
	// off_t N = measure[j].averef;
	// fprintf (stderr, "repair 0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
	// update the image ID
	off_t imageID, Seq;
	short Photcode;
	if (!FindIDstk (&imageID, &Seq, &Photcode, externID)) {
	  fprintf (stderr, "error : cannot find matching stack image ID\n");
	  exit (6);
	}
	measure[j].imageID = imageID;
	BumpInvalidImage (Seq);
	Ninvalid ++;
	continue;
      }
    }
  }

  FREE (fullID);
  FREE (seq);
  FREE (duplicates);

  return TRUE;
}

int UnpackPSPSStackDetectionID(int *sourceID, int *imageID, int *detID, uint64_t pspsStackID)
{
  // sourceID : ID of database + table that tracked the image (< 0x7f = 127)
  // imageID : external ID of the image which provided the detections (< 0x1000.0000 ~ 2.7e8)
  // detID : detection sequence in image (< 0x1000.0000 ~ 2.7e8)

  // 0x0000.0000.0000.0000

  // bits  0 - 27 : 0x0000.0000.0fff.ffff
  // bits 28 - 55 : 0x00ff.ffff.f000.0000
  // bits 56 - 63 : 0xff00.0000.0000.0000

  *sourceID = (pspsStackID & (uint64_t) 0xff00000000000000) >> 56;
  *imageID  = (pspsStackID & (uint64_t) 0x00fffffff0000000) >> 28;
  *detID    = (pspsStackID & (uint64_t) 0x000000000fffffff);

  return TRUE;
}
