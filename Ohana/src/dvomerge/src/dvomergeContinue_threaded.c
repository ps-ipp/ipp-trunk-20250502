NOTE:
/// this is now not used; it has been merged with dvomergeUpdate

# include "dvomerge.h"
# include <pthread.h>

enum {
  TS_WAIT,
  TS_RUN,
  TS_DONE,
  TS_FAIL,
  TS_EXIT,
};

typedef struct {
  int iThread;
  int state;
  // elements used to carry data to the threadjob
  char *name;
  char *filename;
  SkyRegion *region;
  int NsecfiltInput;
  int NsecfiltOutput;
  IDmapType *IDmap;
  SkyTable *outsky;
  int *secfiltMap;
  int depth;
} ThreadData;

void *dvomergeUpdate_threadjob (void *inputData) {

  off_t j;
  Catalog incatalog, outcatalog;
  SkyList *outlist;

  ThreadData *threadData = (ThreadData *) inputData;

  while (TRUE) {

    while (threadData->state != TS_RUN) {
      usleep (100);
    }

    if (VERBOSE) fprintf (stderr, "input: %s\n", threadData->name);

    // load / create output catalog (if catalog does not exist, it will be created)
    LoadCatalog (&incatalog, threadData->region, threadData->filename, "r", threadData->NsecfiltInput);
    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
      threadData->state = TS_DONE;
      continue;
    }

    // combine only tables at equal or larger depth.

    // NOTE: this requirement is especially important for the threaded version: if the
    // output catalog is guaranteed to be the same size or finer than the input, then two
    // input catalogs cannot collide on their target output catalog.
      
    // load in all of the tables from input for this region
    // SkyListByBounds will return neighbor catalogs if the boundaries exactly match (due to rounding).  Since the regions are not infinitely small, 
    // compare to a slightly reduced footprint
    float dPos = 2.0/3600.0;
    outlist = SkyListByBounds (threadData->outsky, threadData->depth, threadData->region->Rmin + dPos, threadData->region->Rmax - dPos, threadData->region->Dmin + dPos, threadData->region->Dmax - dPos);
    for (j = 0; (j < outlist[0].Nregions) && (threadData->state != TS_FAIL); j++) {
      if (VERBOSE) fprintf (stderr, "output : %s\n", outlist[0].regions[j][0].name);

      // load input catalog
      LoadCatalog (&outcatalog, outlist[0].regions[j], outlist[0].filename[j], "w", threadData->NsecfiltOutput);

      dvo_update_image_IDs (threadData->IDmap, &incatalog);
      merge_catalogs_old (&threadData->outsky->regions[j], &outcatalog, &incatalog, RADIUS, threadData->secfiltMap);

      // if we receive a signal which would cause us to exit, wait until the full catalog is written
      SetProtect (TRUE);
      if (!dvo_catalog_save (&outcatalog, VERBOSE)) {
	fprintf (stderr, "ERROR: failed to save catalog %s\n", outcatalog.filename);
	threadData->state = TS_FAIL;
	continue;
      }
      if (!dvo_catalog_unlock (&outcatalog)) {
	fprintf (stderr, "ERROR: failed to unlock catalog %s\n", outcatalog.filename);
	threadData->state = TS_FAIL;
	continue;
      }
      SetProtect (FALSE);

      dvo_catalog_free (&outcatalog);

      fprintf (stderr, "merged %s into %s\n", threadData->name, outlist[0].regions[j][0].name);
    }
    SkyListFree (outlist);

    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
    threadData->state = TS_DONE;
  }
  return NULL;
}

int dvomergeUpdate_threaded (int argc, char **argv) {

  int j, launched, done, Nwait, Nrun;

  ThreadData *threadData;
  pthread_t *threads;

  int depth;
  off_t i, Ns, Ne;
  SkyTable *outsky, *insky;
  SkyList *inlist;
  char filename[256], *input, *output;
  IDmapType IDmap;
  PhotCodeData *inputPhotcodes;
  PhotCodeData *outputPhotcodes;
  int *secfiltMap = NULL;
  int NsecfiltInput, NsecfiltOutput;

  double dtime;
  struct timeval start, stop;
  gettimeofday (&start, NULL);

  CONTINUE = FALSE;
  if (strcasecmp (argv[2], "into")) dvomerge_usage();
  if (argc == 5) {
    if (strcasecmp (argv[4], "continue")) dvomerge_usage();
    CONTINUE = TRUE;
  }

  input  = argv[1];
  output = argv[3];

  if (ALTERNATE_PHOTCODE_FILE) {
    fprintf (stderr, "cannot specify photcodes when merging into an existing catdir\n");
    exit (1);
  }

  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", input);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading input database directory %s\n", input);
    exit (1);
  }	
  inputPhotcodes = GetPhotcodeTable();
  NsecfiltInput = GetPhotcodeNsecfilt();

  // since we are merging the input db into the output db, the output defines the photcode
  // table & db layout but, this requires the output to exist.  if it does not, instead use the
  // input.

  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", output);
  if (LoadPhotcodes (filename, NULL, FALSE)) {

    outputPhotcodes = GetPhotcodeTable();
    NsecfiltOutput = GetPhotcodeNsecfilt();

    secfiltMap = GetSecFiltMap(outputPhotcodes, inputPhotcodes);
    if (!secfiltMap) {
      fprintf (stderr, "failed to map input secfilt photcodes to output photcodes table\n");
      exit (1);
    }
  } else {
    fprintf(stderr, "%s not found using photcodes from %s/Photcodes.dat\n", filename, input);
    outputPhotcodes = inputPhotcodes;
    NsecfiltOutput = NsecfiltInput;

    if (!check_dir_access (output, VERBOSE)) {
      fprintf (stderr, "error creating output database directory %s\n", output);
      exit (1);
    }
    SetPhotcodeTable(inputPhotcodes);
    if (!SavePhotcodesFITS (filename)) {
      fprintf (stderr, "error saving photcode table in %s/Photcodes.dat\n", output);
      exit (1);
    }
  }

  if (CONTINUE) {
    // need to determine the mapping from the input to the output images
    dvomergeImagesGetMap (&IDmap, input, output);
  } else {
    dvomergeImagesUpdate (&IDmap, input, output);
  }

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (input, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  if (!insky) {
      Shutdown ("can't read SkyTable for %s", input);
  }
  SkyTableSetFilenames (insky, input, "cpt");

  // XXX apply this...generate the subset matching the user-selected region
  inlist = SkyListByPatch (insky, -1, &UserPatch);

  // generate an output table populated at the desired depth
  outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  if (!outsky) {
      Shutdown ("can't read or create SkyTable for %s", output);
  }
  SkyTableSetFilenames (outsky, output, "cpt");

  // loop over the populatable output tables; check for data in input in the corresponding regions

  SkyListPopulatedRange (&Ns, &Ne, inlist, 0);
  depth = inlist[0].regions[Ns][0].depth;
  
  SetPhotcodeTable(NULL);

  ALLOCATE(threads, pthread_t, NTHREADS);
  ALLOCATE(threadData, ThreadData, NTHREADS);
  for (i = 0; i < NTHREADS; i++) {
    threadData[i].state = TS_WAIT;
    threadData[i].iThread = i;
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_create (&threads[i], NULL, &dvomergeUpdate_threadjob, (void *) &threadData[i]);
  }

  // loop over the populated input regions
  for (i = 0; i < inlist[0].Nregions; i++) {
    if (!inlist[0].regions[i][0].table) continue;

    launched = FALSE;
    while (!launched) {

      // are any threads done? can we harvest them?
      for (j = 0; j < NTHREADS; j++) {
	if (threadData[j].state == TS_FAIL) goto failure;
	if (threadData[j].state != TS_DONE) continue;
	// fprintf (stderr, "harvested thread %d\n", threadData[j].iThread);
	threadData[j].state = TS_WAIT;
      }
	
      // are any threads available? can we launch this job?
      for (j = 0; !launched && (j < NTHREADS); j++) {
	if (threadData[j].state == TS_FAIL) goto failure;
	if (threadData[j].state != TS_WAIT) continue;

	threadData[j].name = inlist[0].regions[i][0].name;
	threadData[j].filename = inlist[0].filename[i];
	threadData[j].region = inlist[0].regions[i];
	threadData[j].NsecfiltInput = NsecfiltInput;
	threadData[j].NsecfiltOutput = NsecfiltOutput;
	threadData[j].IDmap = &IDmap;
	threadData[j].outsky = outsky;
	threadData[j].secfiltMap = secfiltMap;
	threadData[j].depth = depth;
	threadData[j].state = TS_RUN;
	launched = TRUE;
      }
      if (!launched) usleep (100);
    }
  }

  // wait for all threads to be done
  done = FALSE;
  while (!done) {
    // are any threads done? can we harvest them?
    for (j = 0; j < NTHREADS; j++) {
      if (threadData[j].state == TS_FAIL) goto failure;
      if (threadData[j].state != TS_DONE) continue;
      threadData[j].state = TS_WAIT;
    }

    // how many are waiting now?
    Nwait = 0;
    for (j = 0; j < NTHREADS; j++) {
      if (threadData[j].state == TS_FAIL) goto failure;
      if (threadData[j].state != TS_WAIT) continue;
      Nwait ++;
    }

    if (Nwait < NTHREADS) { 
      usleep (100); 
    } else {
      done = TRUE;
    }
  }

  // save the output sky table copy
  char *skyfile = SkyTableFilename  (output);
  check_file_access (skyfile, TRUE, TRUE, VERBOSE);
  if (!SkyTableSave (outsky, skyfile)) {
    fprintf (stderr, "ERROR: failed to save sky table for %s\n", output);
    exit (1);
  }

  gettimeofday (&stop, NULL);
  dtime = DTIME (stop, start);
  fprintf (stderr, "SUCCESS: elapsed time %9.4f sec\n", dtime);

  exit (0);

failure:

  // wait for all threads to be done
  done = FALSE;
  while (!done) {

    // how many are still running?
    Nrun = 0;
    for (j = 0; j < NTHREADS; j++) {
      if (threadData[j].state != TS_RUN) continue;
      Nrun ++;
    }

    if (Nrun) { 
      usleep (100); 
    } else {
      done = TRUE;
    }
  }
  exit (1);
}

