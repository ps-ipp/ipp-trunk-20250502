/** @file pswarpThreads.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

typedef enum {
  PSWARP_TRANSFORM_TILE
} pswarpJobType;

typedef struct {
  pswarpJobType type;
  void *opts;
} pswarpJob;

static psArray *jobs = NULL;
static pthread_t *threads = NULL;

bool pswarpThreadsAddJob (pwarpJob *job) {

  if (jobs == NULL) {
    jobs = psArrayAlloc (16);
  }

  psArrayAdd (jobs, job);
  return true;
}

bool pswarpThreadsLaunchJobs () {

  while () {
    while ((job = psListGetAndRemove (jobs, PS_LIST_HEAD)) == NULL) {
      usleep (50000);
    }

    switch (job->type) {
      case PSWARP_TRANSFORM_TILE:
	status = pswarpTransformTile ((pswarpTransformTileOpts *)job->opts);
	// send a message somewhere if the job fails
	break;
    }
  }  
}

/**
 * create a pool of Nthreads
 */
bool pswarpThreadsCreate (int nThreads) {

  threads = (pthread_t *) psAlloc (sizeof(pthread_t));

  for (int i = 0; i < nThreads; i++) {
    pthread_create (&threads[i], NULL, &function, NULL);
  }

  return true;
}

