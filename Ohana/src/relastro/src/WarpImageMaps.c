# include "relastro.h"

// we have a list of warp images, each with obstime & photcode.  I need to make a list of the obstime groups

# define D_NOBSTIMES 10000

typedef struct {
  int obstime;
  unsigned short photcode;
  int Nwarps;
  int NWARPS;
  off_t *warps;
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

int FindWarpGroups (void) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

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
  warpObstimeIndex = myIndexInit ();
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

    if (i == 54950) {
      fprintf (stderr, "checking problem warpgroup\n");
    }

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
	// Rmin and Rmax are in the range 0 - 360.  For images at the 0,360 boundary, we need to recalculated Rmin,Rmax 
	// using 0.0 as the midpoint (allowing the range -180 to +180)

	XY_to_RD (&R, &D, 0.0, 0.0, &image[N].coords);
	R = ohana_normalize_angle_to_midpoint(R, 0.0);
	warpgroup[i].Rmin[j] = R;
	warpgroup[i].Rmax[j] = R;
	warpgroup[i].Dmin[j] = D;
	warpgroup[i].Dmax[j] = D;

	XY_to_RD (&R, &D, Nx, 0.0, &image[N].coords);
	R = ohana_normalize_angle_to_midpoint(R, 0.0);
	warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
	warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
	warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
	warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);
	
	XY_to_RD (&R, &D, 0.0, Ny, &image[N].coords);
	R = ohana_normalize_angle_to_midpoint(R, 0.0);
	warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
	warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
	warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
	warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);

	XY_to_RD (&R, &D, Nx, Ny, &image[N].coords);
	R = ohana_normalize_angle_to_midpoint(R, 0.0);
	warpgroup[i].Rmin[j] = MIN (warpgroup[i].Rmin[j], R);
	warpgroup[i].Rmax[j] = MAX (warpgroup[i].Rmax[j], R);
	warpgroup[i].Dmin[j] = MIN (warpgroup[i].Dmin[j], D);
	warpgroup[i].Dmax[j] = MAX (warpgroup[i].Dmax[j], D);

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

int GetWarpSeq (Image *image, int obstime, unsigned short photcode, double Rave, double Dave, float X, float Y) {

  int seq = myIndexGetEntry (warpObstimeIndex, obstime);
  myAssert (seq > -1, "ooops");
  myAssert (warpgroup[seq].photcode == photcode, "oops");
  
  double dPosMin = NAN;
  int nPosMin = -1;

  // we now have the warp group, but which is the correct warp?
  for (int i = 0; i < warpgroup[seq].Nwarps; i++) {
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

    // this is a possible image: check the coordinates:
    int N = warpgroup[seq].warps[i];

    // project Rave,Dave to image pixels
    double Xtst, Ytst;
    RD_to_XY (&Xtst, &Ytst, Rave, Dave, &image[N].coords);

    // find the pixel offset
    double dX = (Xtst - X);
    double dY = (Ytst - Y);
    
    // skip detections which are within a small distance from the expected location
    double dPos = hypot(dX,dY);
    if (dPos < 20.0) return N; // 20.0 pixels = 5.0 arcsec

    // check for multiple possible matches??

    // record the min pos so we can see how things fail
    if (isnan(dPosMin)) {
      dPosMin = dPos;
      nPosMin = i;
    } else {
      if (dPos < dPosMin) {
	dPosMin = dPos;
	nPosMin = i;
      }
    }
  }
  
  // if we did not find any matched, give up
  if (nPosMin < 0) return -1;

  // return closest image
  return warpgroup[seq].warps[nPosMin];
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
