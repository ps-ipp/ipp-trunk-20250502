# include "delstar.h"

IndexArray *find_duplicates_obstime (Image *image, off_t Nimage, off_t *Nduplicates);
off_t find_obstime_range (Image *image, off_t Nimage, off_t firstEntry);
void sort_by_obstime (e_time *T, short *P, off_t *I, off_t N);
void sort_by_photcode (short *P, off_t *I, off_t N);

// this function identifies the images to be deleted based on duplication of the 
// externID values (ex 0).  The result is an array of imageID values to be deleted

// find & delete duplicate images
// duplicates based on externID (skip 0s)
int delete_duplicate_images (FITS_DB *db) {

  off_t i, Nimage;
  Image *image, *outimage;

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  off_t Nduplicates = 0;
  IndexArray *imageID = NULL;

  if (IMAGE_DUPLICATES_BY_OBSTIME) {
    imageID = find_duplicates_obstime (image, Nimage, &Nduplicates);
  } else {
    imageID = find_duplicates (image, Nimage, &Nduplicates);
  }

  if (Nduplicates == 0) {
    fprintf (stderr, "no duplicate images in this database, exiting\n");
    exit (0);
  }

  fprintf (stderr, "deleting measures for "OFF_T_FMT" duplicate images in this database\n", Nduplicates);
  myAbort ("do not attempt to delete detections");

  if (!IMAGE_ONLY) {
    delete_duplicate_image_measures (imageID);
  }

  if (!UPDATE) return TRUE;

  /* delete the identified images */
  off_t Noutimage = 0;
  ALLOCATE (outimage, Image, Nimage);
  for (i = 0; i < Nimage; i++) {
    if (image[i].imageID == 0) continue;
    off_t Ni = image[i].imageID - imageID->minID;
    myAssert (Ni >= 0, "oops");
    myAssert (Ni < imageID->range, "oops");
 
    // imageID->value[Ni] is TRUE if we want to delete the image
    if (imageID->value[Ni]) continue;
    outimage[Noutimage] = image[i];
    Noutimage ++;
  }
  free (image);
  
  fprintf (stderr, "removing "OFF_T_FMT" images (leaving "OFF_T_FMT" of "OFF_T_FMT")\n",  Nimage - Noutimage, Noutimage, Nimage);
  // gfits_table_set_Image (&db[0].ftable, outimage, Noutimage, TRUE);

  gfits_modify (&db[0].theader, "NAXIS2", OFF_T_FMT, 1,  Noutimage);
  gfits_modify (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  Noutimage);
  db[0].theader.Naxis[1] = Noutimage;
  db[0].ftable.buffer = (char *) outimage;

  if (!dvo_image_save (db, VERBOSE)) return FALSE;
  if (!dvo_image_unlock (db)) return FALSE;

  return TRUE;
}

int delete_duplicate_image_measures (IndexArray *imageID) {

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
    int status = delete_duplicate_image_measures_parallel (skylist, imageID);
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

    delete_duplicate_image_measures_catalog (&catalog, imageID);

    if (UPDATE) {
      SetProtect (TRUE);
      dvo_catalog_save_complete (&catalog, VERBOSE2);
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }
  return TRUE;
}

// CATDIR is supplied globally
int delete_duplicate_image_measures_parallel (SkyList *sky, IndexArray *imageID) {

  // write out the subset table of image information
  char imageFile[512];
  snprintf_nowarn (imageFile, 512, "%s/ImageIDs.tmp.fits", CATDIR);

  if (!ImageIDSave (imageFile, imageID)) {
    fprintf (stderr, "failed to write image ID table\n");
    exit (1);
  }

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

    char command[1024];
    snprintf_nowarn (command, 1024, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -dup-images", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

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

int delete_duplicate_image_measures_catalog (Catalog *catalog, IndexArray *imageID) {

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
    measureDrop[i] = 0;
    if (measure[i].imageID == 0) continue;
    off_t Ni = measure[i].imageID - imageID->minID;
    myAssert (Ni >= 0, "oops");
    myAssert (Ni < imageID->range, "oops");
    measureDrop[i] = imageID->value[Ni];
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

  off_t NdelAves = Naverage - catalog[0].Naverage;
  off_t NdelMeas = Nmeasure - catalog[0].Nmeasure;
  if (NdelAves || NdelMeas) {
    fprintf (stderr, "deleting "OFF_T_FMT" measures and "OFF_T_FMT" averages : %s\n",  NdelMeas, NdelAves, catalog[0].filename);
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

IndexArray *find_duplicates (Image *image, off_t Nimage, off_t *Nduplicates) {

  off_t i, *Ndelete;

  // how to find duplicates:
  // generate an index array of externID
  //  * find min & max value
  //  * array has length max - min
  // generate the count of externID values
  // mark those with N > 1

  // index->value array is set to 0 initially
  IndexArray *extID = make_index_array (image, Nimage, EXTERN_ID);

  ALLOCATE (Ndelete, off_t, extID->range);
  for (i = 0; i < extID->range; i++) Ndelete[i] = 0;

  // set extID->value to have the number of images with a given externID
  for (i = 0; i < Nimage; i++) {
    if (image[i].externID == 0) continue;
    off_t N = image[i].externID - extID->minID;
    myAssert (N >= 0, "oops");
    myAssert (N < extID->range, "oops");
    extID->value[N] ++;
  }
  // count has the number for each image.externID
  // now find things to be deleted:

  // now I know the externID values to delete, I need to get a list of imageID values
  // more specifically, i need an index of the imageIDs against an array saying delete or not

  // somewhere downstream I want to be able to say:
  // if (delete[measure.imageID]) delete_measure...

  IndexArray *imageID = make_index_array (image, Nimage, IMAGE_ID);

  off_t Ndup = 0;

  if (SKIP_DIFF_PAIRS) {
    int *pair1 = NULL;
    int *pair2 = NULL;

    // in this case, I need to generate a set of pair maps:
    ALLOCATE (pair1, int, extID->range); // first image for the extID
    ALLOCATE (pair2, int, extID->range); // second image for the extID
    for (i = 0; i < extID->range; i++) { 
      pair1[i] = -1;
      pair2[i] = -1;
    }
    for (i = 0; i < Nimage; i++) {
      if (image[i].externID == 0) continue;
      off_t Nx = image[i].externID - extID->minID;
      myAssert (Nx >= 0, "oops");
      myAssert (Nx < extID->range, "oops");

      // only map the pairs
      if (extID->value[Nx] != 2) continue;

      if (pair1[Nx] == -1) {
	pair1[Nx] = i;
	continue;
      }
      if (pair2[Nx] == -1) {
	pair2[Nx] = i;
	continue;
      }
    }

    for (i = 0; i < extID->range; i++) {
      // we have only 2 valid cases: 
      // 1) pair1 == pair2 == -1, 
      // 2) pair1 > -1, pair2 > -1

      // this extID does not correspond to a pair, skip it:
      if ((pair1[i] == -1) && (pair2[i] == -1)) continue;

      myAssert (extID->value[i] == 2, "oops");

      int valid = ((pair1[i]  > -1) && (pair2[i]  > -1));
      myAssert (valid, "oops");
      
      // we think we have a pair, check the names:
      int length1 = strlen(image[pair1[i]].name);
      int length2 = strlen(image[pair2[i]].name);

      // names should be of the form: foo.cmf[SkyChip.hdr], foo.inv.cmf[SkyChip.hdr]
      valid = (length1 == length2 + 4) || (length1 == length2 - 4);
      if (!valid) continue;

      int baselength = MIN(length1, length2) - 17;
      valid = !strncmp (image[pair1[i]].name, image[pair2[i]].name, baselength);
      if (!valid) continue;

      valid = TRUE;
      if (length1 > length2) {
	valid = valid && !strcmp (&image[pair1[i]].name[baselength], ".inv.cmf[SkyChip.hdr]");
	valid = valid && !strcmp (&image[pair2[i]].name[baselength], ".cmf[SkyChip.hdr]");
      } else {
	valid = valid && !strcmp (&image[pair1[i]].name[baselength], ".cmf[SkyChip.hdr]");
	valid = valid && !strcmp (&image[pair2[i]].name[baselength], ".inv.cmf[SkyChip.hdr]");
      }
      if (!valid) continue;

      // this pair is a valid diff image pair, do not delete:
      extID->value[i] = 1;
    }
  }

  // set imageID->value to TRUE for images we want to delete
  for (i = 0; i < Nimage; i++) {
    if (image[i].externID == 0) continue;
    off_t Nx = image[i].externID - extID->minID;
    myAssert (Nx >= 0, "oops");
    myAssert (Nx < extID->range, "oops");

    // do not delete any extID with only a single image
    if (extID->value[Nx] < 2) continue;

    // do not delete the last image
    if (Ndelete[Nx] == extID->value[Nx] - 1) continue;

    off_t Ni = image[i].imageID - imageID->minID;
    myAssert (Ni >= 0, "oops");
    myAssert (Ni < imageID->range, "oops");
    imageID->value[Ni] = TRUE;
    Ndelete[Nx] ++;

    Ndup ++;

    // mark the parentID for deletion as well..
    // XXX this is probably bad : it assumes all chips
    // should be deleted for a given exposure
    // off_t parentID = image[i].parentID;
    // if (parentID) {
    //   off_t Np = parentID - imageID->minID;
    //   myAssert (Np >= 0, "oops");
    //   myAssert (Np < imageID->range, "oops");
    //   imageID->value[Np] = TRUE;
    // }
  }

  for (i = 0; IMAGE_DETAILS && (i < Nimage); i++) {
    off_t Ni = image[i].imageID - imageID->minID;
    if (!imageID->value[Ni]) continue;
    fprintf (stderr, "delete image (i) " OFF_T_FMT ", extID = %d : %s\n", i, image[i].externID, image[i].name);
  }

  free (extID->value);
  free (extID);

  *Nduplicates = Ndup;

  return imageID;;
}

// sort by increasing obstime
void sort_by_obstime (e_time *T, short *P, off_t *I, off_t N) {

# define SWAPFUNC(A,B){ e_time tmpT; short tmpP; off_t tmpI; \
  tmpT = T[A]; T[A] = T[B]; T[B] = tmpT; \
  tmpP = P[A]; P[A] = P[B]; P[B] = tmpP; \
  tmpI = I[A]; I[A] = I[B]; I[B] = tmpI; \
}
# define COMPARE(A,B)(T[A] < T[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// sort by increasing photcode
void sort_by_photcode (short *P, off_t *I, off_t N) {

# define SWAPFUNC(A,B){ short tmpP; off_t tmpI; \
  tmpP = P[A]; P[A] = P[B]; P[B] = tmpP; \
  tmpI = I[A]; I[A] = I[B]; I[B] = tmpI; \
}
# define COMPARE(A,B)(P[A] < P[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

static e_time *obstime  = NULL;
static short  *photcode = NULL;
static off_t  *primary  = NULL;
static off_t  *idx      = NULL;
static char   *keep     = NULL;

static short  *photcode_subset = NULL;
static off_t  *idx_subset      = NULL;

static off_t Nsubset = 0;
static off_t NSUBSET = 300;

off_t find_obstime_range (Image *image, off_t Nimage, off_t firstEntry) {
  OHANA_UNUSED_PARAM(image);

  Nsubset = 0;

  // find all entries with the same obstime:
  e_time firstTime = obstime[firstEntry];

  idx_subset[Nsubset] = idx[firstEntry];
  photcode_subset[Nsubset] = photcode[firstEntry];
  Nsubset++;

  off_t i = firstEntry + Nsubset;
  while ((i < Nimage) && (obstime[i] == firstTime)) {
    idx_subset[Nsubset] = idx[i];
    photcode_subset[Nsubset] = photcode[i];
    Nsubset++;
    i++;
    if (Nsubset >= NSUBSET) {
      NSUBSET += 1000;
      REALLOCATE (photcode_subset, short, NSUBSET);
      REALLOCATE (idx_subset, off_t, NSUBSET);
    }
  }      

  sort_by_photcode (photcode_subset, idx_subset, Nsubset);

  return i;
}

// alternative version to find duplicates based on obstime and photcode
IndexArray *find_duplicates_obstime (Image *image, off_t Nimage, off_t *Nduplicates) {

  if (Nimage < 1) return NULL;

  // how to find duplicates:
  // generate a set of arrays (idx, obstime, photcode, keep)
  // sort the arrays by obstime
  // loop over obstime.  
  // for a given new obstime, scan through the entries with the same value
  // create a sub-array of photcodes, idx
  // sort by photcode
  // loop over entries
  // find matching photcodes
  // mark duplicate enties

  ALLOCATE (obstime, e_time, Nimage);
  ALLOCATE (photcode, short, Nimage);
  ALLOCATE (primary, off_t, Nimage);
  ALLOCATE (idx, off_t, Nimage);
  ALLOCATE (keep, char, Nimage);

  off_t i;

  INITTIME;

  // skip entries with photcode == 0?
  for (i = 0; i < Nimage; i++) {
    idx[i] = i;
    primary[i] = -1; // only duplicates get a value for primary
    keep[i] = TRUE;
    obstime[i] = image[i].tzero;
    photcode[i] = image[i].photcode;
  }
  MARKTIME("  generate index arrays: %f sec\n", dtime);

  // sort the 4 arrays
  sort_by_obstime (obstime, photcode, idx, Nimage);
  MARKTIME("  sort index arrays: %f sec\n", dtime);

  // image[idx[i]].tzero = obstime[i]
  // keep[i] -> keep image[i] ('keep' is NOT resorted)

  // allocate arrays to store the subsets (these are global static)
  // These get reallocated if necessary in find_obstime_range()
  ALLOCATE (photcode_subset, short, NSUBSET);
  ALLOCATE (idx_subset, off_t, NSUBSET);

  // entries of idx_subset correspond to the original image sequence:
  // image[idx_subset[i]].tzero = obstime[i]

  off_t firstEntry = 0;
  off_t nextEntry = 0;
  
  while (nextEntry < Nimage) {
    if (firstEntry >= Nimage) {
      fprintf (stderr, "error, too far?\n");
    }
    
    // generate photcode_subset, idx_subset in order of photcode for this unique obstime[firstEntry]
    // returned value is the first value of the next entry
    nextEntry = find_obstime_range (image, Nimage, firstEntry);
    
    // step through the photcodes and find duplicates
    int j;
    int firstCodeEntry = 0;
    short firstCode = photcode_subset[firstCodeEntry];
    for (j = 1; j < Nsubset; j++) {
      if (photcode_subset[j] == firstCode) {
	// mark as duplicate
	off_t dupIndex = idx_subset[j];
	keep[dupIndex] = FALSE;
	primary[dupIndex] = idx_subset[firstCodeEntry];
      } else {
	// new value of photcode, call if the first one
	firstCodeEntry = j;
	firstCode = photcode_subset[firstCodeEntry];
      }
    }
    firstEntry = nextEntry;
  }
  MARKTIME("  find duplicates: %f sec\n", dtime);

  IndexArray *imageID = make_index_array (image, Nimage, IMAGE_ID);
  MARKTIME("  make index array: %f sec\n", dtime);

  // set imageID->value to TRUE for images we want to delete
  off_t Ndup = 0;
  for (i = 0; i < Nimage; i++) {
    if (keep[i]) continue;
    off_t Ni = image[i].imageID - imageID->minID;
    myAssert (Ni >= 0, "oops");
    myAssert (Ni < imageID->range, "oops");
    imageID->value[Ni] = TRUE;
    Ndup ++;
  }
  MARKTIME("  mark duplicates: %f sec\n", dtime);

  for (i = 0; IMAGE_DETAILS && (i < Nimage); i++) {
    off_t Ni = image[i].imageID - imageID->minID;
    if (!imageID->value[Ni]) continue;
    
    char *date = NULL;
    date = ohana_sec_to_date (image[i].tzero);
    fprintf (stderr, "delete image : (" OFF_T_FMT "), extID = %d : %30s : %20s %5d  ==  ", i, image[i].externID, image[i].name, date, image[i].photcode);
    free (date);

    off_t myPrimary = primary[i];
    if (myPrimary < 0) {
      fprintf (stderr, "ERROR: this should never happen\n");
      abort();
    }
    date = ohana_sec_to_date (image[myPrimary].tzero);
    fprintf (stderr, "parent image : (" OFF_T_FMT "), extID = %d : %30s : %20s %5d\n", myPrimary, image[myPrimary].externID, image[myPrimary].name, date, image[myPrimary].photcode);
    free (date);
  }

  *Nduplicates = Ndup;

  free (photcode_subset);
  free (idx_subset);
  
  free (obstime);
  free (photcode);
  free (primary);
  free (idx);
  free (keep);

  return imageID;;
}

// find the min & max values of the given ID (externID or imageID)
// construct an empty array with length needed to fit IDs
IndexArray *make_index_array (Image *image, off_t Nimage, int mode) {

  off_t i;

  IndexArray *idxarray = NULL;
  ALLOCATE (idxarray, IndexArray, 1);

  idxarray->minID = -1;
  idxarray->maxID = -1;
  for (i = 0; i < Nimage; i++) {
    off_t value = 0;
    if (mode == EXTERN_ID) {
      value = image[i].externID;
      if (value == 0) continue;
    }
    if (mode == IMAGE_ID) {
      value = image[i].imageID;
      if (value == 0) continue;
    }
    if (idxarray->minID == -1) idxarray->minID = value;
    idxarray->minID = MIN(idxarray->minID, value);
    idxarray->maxID = MAX(idxarray->maxID, value);
  }
  idxarray->range = idxarray->maxID- idxarray->minID + 1;

  // check if this will be too large?
  ALLOCATE (idxarray->value, off_t, idxarray->range);

  for (i = 0; i < idxarray->range; i++) idxarray->value[i] = 0;

  return idxarray;
}

