# include "dvolens.h"

// we have a list of warp images, each with obstime & photcode.  I need to make a list of the obstime groups

# define D_NOBSTIMES 10000

typedef struct {
  int obstime;
  int Nwarps;
  int NWARPS;
  off_t *warps;
  unsigned short photcode;
  double *Rmin;
  double *Rmax;
  double *Dmin;
  double *Dmax;
  int    *onBoundary;
} WarpGroup;

WarpGroup *warpgroup = NULL;
int Nwarpgroup = 0;
int NWARPGROUP = 0;

myIndexType *warpObstimeIndex = NULL;

static off_t Nimage = 0;
static Image *image = NULL;

int FindWarpGroups (void) {

  // keep a local static copy of the image pointer
  image = getimages (&Nimage);

  int *obstimes = NULL;
  int Nobstimes = 0;
  int NOBSTIMES = D_NOBSTIMES;
  ALLOCATE (obstimes, int, NOBSTIMES);

  // first generate a list of obstime values
  for (off_t i = 0; i < Nimage; i++) {
    if (!isGPC1warp(image[i].photcode)) continue;

    obstimes[Nobstimes] = image[i].tzero;
    Nobstimes ++;

    CHECK_REALLOCATE (obstimes, int, NOBSTIMES, Nobstimes, D_NOBSTIMES);
  }

  // sort the list:
  isort (obstimes, Nobstimes);

  // find the unique obstimes
  int *uniqtimes = NULL;
  int *uniqcount = NULL;
  int Nuniqtimes = 0;
  ALLOCATE (uniqtimes, int, Nobstimes);
  ALLOCATE (uniqcount, int, Nobstimes);

  // generate a uniq set of obstimes
  for (int i = 0; i < Nobstimes; Nuniqtimes ++) {
    uniqtimes[Nuniqtimes] = obstimes[i];
    int Ndup = 0;
    int lastValue = uniqtimes[Nuniqtimes];
    while ((i < Nobstimes) && (obstimes[i] == lastValue)) {
      i++;
      Ndup ++;
      uniqcount[Nuniqtimes] = Ndup;
    }
  }
  REALLOCATE (uniqtimes, int, Nuniqtimes);
  REALLOCATE (uniqcount, int, Nuniqtimes);
  
  // now I need to assign all warp images to a warpgroup
  // I have 2 options to go from warp obstime to warpgroup:
  // a) use a bisection lookup [may be slow]
  // b) use an index on obstime [require ~5*3.14*1e7*4 bytes ~ 600MB for index]

  // generate an index on obstime
  warpObstimeIndex = myIndexAlloc ();
  myIndexInit (warpObstimeIndex);
  warpObstimeIndex->minID = obstimes[0];
  warpObstimeIndex->maxID = obstimes[Nobstimes-1];

  myIndexSetRange (warpObstimeIndex);

  // generate the warp groups (create the index as we go)
  Nwarpgroup = Nuniqtimes;
  ALLOCATE (warpgroup, WarpGroup, Nwarpgroup);
  
  for (int i = 0; i < Nuniqtimes; i++) {
    warpgroup[i].obstime = uniqtimes[i];
    warpgroup[i].photcode = 0; // not yet set
    warpgroup[i].Nwarps = 0;
    warpgroup[i].NWARPS = 100;
    ALLOCATE (warpgroup[i].warps, off_t, warpgroup[i].NWARPS);
    myIndexSetEntry (warpObstimeIndex, uniqtimes[i], i);
  }

  // assign all warps to one of the warpgroups
  for (off_t i = 0; i < Nimage; i++) {
    if (!isGPC1warp(image[i].photcode)) continue;

    int seq = myIndexGetEntry (warpObstimeIndex, image[i].tzero);
    myAssert (warpgroup[seq].obstime == image[i].tzero, "oops");

    if (!warpgroup[seq].photcode) {
      warpgroup[seq].photcode = image[i].photcode;
    } else {
      myAssert (warpgroup[seq].photcode == image[i].photcode, "oops");
    }

    int N = warpgroup[seq].Nwarps;
    warpgroup[seq].warps[N] = i;
    warpgroup[seq].Nwarps ++;
    
    CHECK_REALLOCATE (warpgroup[seq].warps, off_t, warpgroup[seq].NWARPS, warpgroup[seq].Nwarps, 100);
  }

  // we now have the warps assigned to groups.  now we need to generate the grid data to find the warp assignments
  for (int i = 0; i < Nwarpgroup; i++) {

    myAssert (warpgroup[i].Nwarps == uniqcount[i], "failure");
    ALLOCATE (warpgroup[i].Rmin, double, warpgroup[i].Nwarps);
    ALLOCATE (warpgroup[i].Rmax, double, warpgroup[i].Nwarps);
    ALLOCATE (warpgroup[i].Dmin, double, warpgroup[i].Nwarps);
    ALLOCATE (warpgroup[i].Dmax, double, warpgroup[i].Nwarps);
    ALLOCATE (warpgroup[i].onBoundary, int, warpgroup[i].Nwarps);

    for (int j = 0; j < warpgroup[i].Nwarps; j++) {

      off_t N = warpgroup[i].warps[j];
      
      int Nx = image[N].NX;
      int Ny = image[N].NY;

      warpgroup[i].onBoundary[j] = FALSE;

      double R, D;
      XY_to_RD (&R, &D, 0.0, 0.0, &image[N].coords);
      R = ohana_normalize_angle_to_midpoint(R, 180.0);
      warpgroup[i].Rmin[j] = R;
      warpgroup[i].Rmax[j] = R;
      warpgroup[i].Dmin[j] = D;
      warpgroup[i].Dmax[j] = D;

      // XXX need to worry about 0,360 boundary
      XY_to_RD (&R, &D, Nx, 0.0, &image[N].coords);
      R = ohana_normalize_angle_to_midpoint(R, 180.0);
      warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
      warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
      warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
      warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);

      // XXX need to worry about 0,360 boundary
      XY_to_RD (&R, &D, 0.0, Ny, &image[N].coords);
      R = ohana_normalize_angle_to_midpoint(R, 180.0);
      warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
      warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
      warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
      warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);

      // XXX need to worry about 0,360 boundary
      XY_to_RD (&R, &D, Nx, Ny, &image[N].coords);
      R = ohana_normalize_angle_to_midpoint(R, 180.0);
      warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
      warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
      warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
      warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);

      // bump Rmin,Rmax,Dmin,Dmax 10 arcsec worth of padding
      double dR = 10.0/3600.0 / cos (RAD_DEG*warpgroup[i].Dmin[j]);
      double dD = 10.0/3600.0;

      // at north pole, force test of all nearby skycells
      if (warpgroup[i].Dmax[j] > 89.8) {
	warpgroup[i].Rmin[j] =   0.0;
	warpgroup[i].Rmax[j] = 360.0;
	warpgroup[i].Dmax[j] =  90.0;
	warpgroup[i].Dmin[j] -=  dD;
	continue;
      } 

      if (warpgroup[i].Rmax[j] - warpgroup[i].Rmin[j] > 270.0) {
	// Rmin and Rmax are in the range 0 - 360.  For images at the 0,360 boundary,
	// "Rmax" is the lower edge, and "Rmin" is the upper edge.  we need to flip them 
	// and then break the 0-360 range:
	double tmp = warpgroup[i].Rmin[j];
	warpgroup[i].Rmin[j] = warpgroup[i].Rmax[j] - 360.0;
	warpgroup[i].Rmax[j] = tmp;
	warpgroup[i].onBoundary[j] = TRUE;
      }

      warpgroup[i].Rmin[j] -=  dR;
      warpgroup[i].Rmax[j] +=  dR;
      warpgroup[i].Dmin[j] -=  dD;
      warpgroup[i].Dmax[j] +=  dD;
    }
  }

  free (uniqtimes); 
  free (uniqcount); 
  free (obstimes);
  return TRUE;
}

int RecoverLensingIndex (Average *average, mySequenceType *measureSeq, Lensing *lensing) {

  // Although the imageID is wrong (no matching measures), the image should be one of the
  // correct set of warps with the same obstime. Find that set so we can find a matching image
  int im = getImageByID (lensing->imageID);
  if (im < 0) {
    return -1;
  }

  int seq = myIndexGetEntry (warpObstimeIndex, image[im].tzero);
  myAssert (seq > -1, "oops");

  // can we find an image in this set which matches one of our measures?
  double Rave = average->R;
  double Dave = average->D;

  // we now have the warp group, but which is the correct warp?
  for (int i = 0; i < warpgroup[seq].Nwarps; i++) {

    // XXX check on Rmin,Rmax,Dmin,Dmax
    // average->R,D could pin-point more quickly

    if (warpgroup[seq].onBoundary[i]) {
      int inRange1 = (Rave >= warpgroup[seq].Rmin[i]) && (Rave <= warpgroup[seq].Rmax[i]);
      int inRange2 = (Rave >= warpgroup[seq].Rmin[i] + 360.0) && (Rave <= warpgroup[seq].Rmax[i] + 360.0);
      if (!inRange1 && !inRange2) continue;
    } else {
      if (Rave < warpgroup[seq].Rmin[i]) continue;
      if (Rave > warpgroup[seq].Rmax[i]) continue;
    }
    if (Dave < warpgroup[seq].Dmin[i]) continue;
    if (Dave > warpgroup[seq].Dmax[i]) continue;

    // this is a possible image based on coordinates: does it match a warp?
    int N = warpgroup[seq].warps[i];

    int Mj = mySequenceGetEntry (measureSeq, image[N].imageID);
    if (Mj < 0) continue;

    // We have a match between a warp measure and an image in this obstime group.  this
    // may not be the only possible solution, but any other solution will have to be one
    // of the matching overlapping images.  I'm not sure I can distinguish these.
    lensing->oldImID = lensing->imageID;
    lensing->imageID = image[N].imageID;
    return Mj;
  }
  
  return -1;
}

void FreeWarpGroups (void) {

  for (int i = 0; i < Nwarpgroup; i++) {
    free (warpgroup[i].warps);
    free (warpgroup[i].Rmin);
    free (warpgroup[i].Rmax);
    free (warpgroup[i].Dmin);
    free (warpgroup[i].Dmax);
    free (warpgroup[i].onBoundary);
  }    

  free (warpgroup);
  myIndexFree (warpObstimeIndex);
}
