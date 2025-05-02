# include "addstar.h"
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
  int forcesort;
  char *filename;
  SkyRegion *region;
  off_t Naverage;
  off_t Nmeasure;
} ThreadData;

// the thread manager has three states: 
// WAIT : no data is available for this thread
// RUN  : data is available, attempt to process
// DONE : completed the analysis, wait for harvest
// FAIL : serious error resulting in system shutdown

void *resort_thread_job (void *inputData) {

  Catalog catalog;
  ThreadData *threadData = (ThreadData *) inputData;

  while (TRUE) {

    while (threadData->state != TS_RUN) {
      usleep (100);
    }

    // chose the catalog file (local or remote?)
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, threadData->region->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : threadData->filename;

    // set the parameters which guide catalog open/load/create
    catalog.catformat   = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode     = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catcompress = dvo_catalog_catcompress (CATCOMPRESS); // set the default catcompress from config data
    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_LENSING;
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();
  
    // an error exit status here is a significant error (disk I/O or file access)
    if (!dvo_catalog_open (&catalog, threadData->region, VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      threadData->Naverage = 0;
      threadData->Nmeasure = 0;
      threadData->state = TS_FAIL;
      continue;
    }

    // Naverage_disk == 0 implies an empty catalog file, skip empty catalogs
    if (catalog.Naverage_disk == 0) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      threadData->Naverage = 0;
      threadData->Nmeasure = 0;
      threadData->state = TS_DONE;
      continue;
    }

    // this is an overloaded value to mean 'force sort'
    if (threadData->forcesort) catalog.sorted = FALSE;
    resort_catalog (&catalog);

    /* report total updated values */
    threadData->Naverage = catalog.Naverage;
    threadData->Nmeasure = catalog.Nmeasure;
    // fprintf (stderr, "done in thread %d, %d aves, %d meas\n", threadData->iThread, (int) threadData->Naverage, (int) threadData->Nmeasure);

    // write out catalog, if appropriate
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
    threadData->state = TS_DONE;
  }
  return NULL;
}

int resort_threaded (SkyList *skylist, int ForceSort) {

  int j, launched, done, Nwait, Nrun;

  off_t i;
  off_t Naverage, Nmeasure;

  ThreadData *threadData;
  pthread_t *threads;

  ALLOCATE(threads, pthread_t, NTHREADS);
  ALLOCATE(threadData, ThreadData, NTHREADS);
  for (i = 0; i < NTHREADS; i++) {
    threadData[i].state = TS_WAIT;
    threadData[i].iThread = i;
  }
  for (i = 0; i < NTHREADS; i++) {
    pthread_create (&threads[i], NULL, &resort_thread_job, (void *) &threadData[i]);
  }

  // if we catch a failure at any point, goto the failure block and wait for threads to finish
  // continue in this loop until all regions have been launched (though not finished)
  Naverage = Nmeasure = 0;
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    launched = FALSE;
    while (!launched) {

      // are any threads done? can we harvest them?
      for (j = 0; j < NTHREADS; j++) {
	if (threadData[j].state == TS_FAIL) goto failure;
	if (threadData[j].state != TS_DONE) continue;
	Naverage += threadData[j].Naverage;
	Nmeasure += threadData[j].Nmeasure;
	// fprintf (stderr, "harvested thread %d, %d aves, %d meas (sums: %d aves, %d meas)\n", threadData[j].iThread, (int) threadData[j].Naverage, (int) threadData[j].Nmeasure, (int) Naverage, (int) Nmeasure);
	threadData[j].state = TS_WAIT;
      }
	
      // are any threads available? can we launch this job?
      for (j = 0; !launched && (j < NTHREADS); j++) {
	if (threadData[j].state == TS_FAIL) goto failure;
	if (threadData[j].state != TS_WAIT) continue;
	threadData[j].forcesort = ForceSort;
	threadData[j].filename = skylist[0].filename[i];
	threadData[j].region = skylist[0].regions[i];
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
      Naverage += threadData[j].Naverage;
      Nmeasure += threadData[j].Nmeasure;
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

  // for (i = 0; i < NTHREADS; i++) {
  //   pthread_cancel(threads[i]);
  // }
  // free (threads);
  free (threadData);

  return TRUE;

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

