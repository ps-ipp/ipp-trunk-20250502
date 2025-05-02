# include "relastro.h"

static double *dPosSum    = NULL; // sum of dPos^2 for all measures on each image
static off_t  *nPosSum    = NULL; // sum of measures on each image (used for dPosSum)
static int    *isBadCoord = NULL; // keep or reject each image?

static Coords *oldCoords;   // list of available images
static off_t  NoldCoords;   // number of available images

void initCoords (void) {

  off_t N;

  // Images *images = getimages (&N, NULL); return value ignored
  getimages (&N, NULL);

  NoldCoords = N;
  ALLOCATE (oldCoords, Coords, NoldCoords);
  ALLOCATE (dPosSum, double, NoldCoords);
  ALLOCATE (nPosSum, off_t,  NoldCoords);
  ALLOCATE (isBadCoord, int, NoldCoords);
  memset (oldCoords,  0, N*sizeof(Coords));
  memset (dPosSum,    0, N*sizeof(double));
  memset (nPosSum,    0, N*sizeof(off_t));
  memset (isBadCoord, 0, N*sizeof(int));
}

int saveCoords (Coords *coords, off_t N) {

  if (N < 0) return FALSE;
  if (N >= NoldCoords) return FALSE;

  memcpy (&oldCoords[N], coords, sizeof(Coords));
  return TRUE;
}

Coords *getCoords (off_t N) {

  if (N < 0) return NULL;
  if (N >= NoldCoords) return NULL;

  return (&oldCoords[N]);
}

int badCoords (off_t N) {

  if (N < 0) return FALSE;
  if (N >= NoldCoords) return FALSE;

  return (isBadCoord[N]);
}
  
void setBadCoords (off_t N) {

  if (N < 0) return;
  if (N >= NoldCoords) return;

  isBadCoord[N] = TRUE;
  return;
}
  
void saveOffsets (double dPos, off_t nPos, off_t N) {

  if (N < 0) return;
  if (N >= NoldCoords) return;

  dPosSum[N] += dPos;
  nPosSum[N] += nPos;
  
  return;
}
  
void getOffsets (double *dPos, off_t *nPos, off_t N) {

  if (N < 0) return;
  if (N >= NoldCoords) return;

  *dPos = dPosSum[N];
  *nPos = nPosSum[N];
  
  return;
}
  
