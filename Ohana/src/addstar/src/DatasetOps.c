# include "addstar.h"
# include <pthread.h>

/* the init function is an alternative */
/* pthread_mutex_init(&mutex, const pthread_mutexattr_t *mutexattr); */
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

static int Ndataset = 0;
static int NDATASET = 0;
static DVO_DATA **dataset = NULL;

int InitDataset () {

  int i;


  Ndataset = 0;
  NDATASET = 100;
  ALLOCATE (dataset, DVO_DATA *, NDATASET);
  for (i = 0; i < NDATASET; i++) {
    dataset[i] = NULL;
  }
  return (TRUE);
}

/* data a set dataset to the end of the stack */
int PushDataset (DVO_DATA *data) {

  int N;

  pthread_mutex_lock(&fastmutex);

  N = Ndataset;
  Ndataset ++;
  CHECK_REALLOCATE (dataset, DVO_DATA *, NDATASET, Ndataset, 100);

  dataset[N] = data;

  pthread_mutex_unlock(&fastmutex);
  return (TRUE);
}

/* remove and return first dataset */
DVO_DATA *PopDataset (void) {

  int i;
  DVO_DATA *data;

  pthread_mutex_lock(&fastmutex);

  if (Ndataset == 0) {
    pthread_mutex_unlock(&fastmutex);
    return (NULL);
  }

  data = dataset[0];
  
  for (i = 0; i < Ndataset - 1; i++) {
    dataset[i] = dataset[i+1];
  }
  Ndataset --;
  if ((Ndataset < NDATASET / 2) && (Ndataset > 50)) {
    NDATASET = Ndataset / 2;
    REALLOCATE (dataset, DVO_DATA *, NDATASET);
  }

  pthread_mutex_unlock(&fastmutex);
  return (data);
}

