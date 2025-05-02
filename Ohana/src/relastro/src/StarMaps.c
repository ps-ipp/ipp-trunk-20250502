# include "relastro.h"

// XXX mark the image grid based on the loaded detections
// XXX modify tcatalog to exclude any photcode or other choices that
// only affect image selection?

// XXX this stuff is only going to work for SIMPLE and CHIP; need to
// use parentID to get from imageID to the mosaic pseudo-image

typedef struct {
  double L;
  double M;
  double X;
  double Y;
  double dL;
  double dM;
} StarMapPoint;

typedef struct {
  int  Nx;
  int  Ny;
  int *stars; // arrays to count the number of stars in each map bin
  StarMapPoint *points; // test points generated based on map
  int Npoints;
} StarMap;

static StarMap *starmap = NULL; 
// static int         Nstarmap;  -- is it always == Nimages?

/** we have two possible options for how the user defines the map bins:
    a) the user specifies the number of cells in the map, leading to varying binning
    b) the user specifies the binning factor, leading to variable grid sizes.
    
    We want to use (b) since we know the order of the model
**/

int initStarMaps () {

  Image *images;
  off_t i, Nimages;

  images = getimages(&Nimages, NULL);

  ALLOCATE (starmap, StarMap, Nimages);

  for (i = 0; i < Nimages; i++) {
    starmap[i].Npoints = 0;
    starmap[i].points = NULL;
    starmap[i].Nx = images[i].NX / NX_MAP;
    starmap[i].Ny = images[i].NY / NY_MAP;
    ALLOCATE (starmap[i].stars, int, NX_MAP*NY_MAP);
    memset (starmap[i].stars, 0, sizeof(int)*NX_MAP*NY_MAP);
  }
  return (TRUE);
}

void freeStarMaps () {

  off_t i, Nimages;

  getimages(&Nimages, NULL);

  for (i = 0; i < Nimages; i++) {
    FREE (starmap[i].points);
    FREE (starmap[i].stars);
  }

  FREE (starmap);
  return;
}

int updateStarMaps(Catalog *catalog) {

  off_t i, N, Nimages;
  int xbin, ybin;

  INITTIME;

  // Images *images = getimages(&Nimages, NULL); return value ignored
  getimages(&Nimages, NULL);

  for (i = 0; i < catalog[0].Nmeasure; i++) {
    
    MeasureTiny *measure = &catalog[0].measureT[i];

    N = getImageByID(measure[0].imageID);
    if (N < 0) continue;

    // NOTE: we use Xccd,Yccd even if USE_FIXED_PIXCOORDS is true: the difference is too
    // small to be relevant for this analysis
    xbin = measure[0].Xccd / starmap[N].Nx;
    ybin = measure[0].Yccd / starmap[N].Ny;

    xbin = MAX(0, MIN(NX_MAP-1, xbin));
    ybin = MAX(0, MIN(NY_MAP-1, ybin));

    starmap[N].stars[ybin*NX_MAP + xbin] ++;
  }
  if (VERBOSE2) { MARKTIME("assign stars to starmap bins: %f sec\n", dtime); }

  return (TRUE);
}

int createStarMap (Catalog *catalog, int Ncatalog) {

  int i;

  initStarMaps();

  for (i = 0; i < Ncatalog; i++) {
    // check coverage per chip (operates on (Average, MeasureTiny, Secfilt)
    updateStarMaps (&catalog[i]);
  }

  createStarMapPoints();
  return TRUE;
}

int createStarMapPoints() {

  Image *images;
  off_t i, Nimages;
  int ix, iy;

  images = getimages(&Nimages, NULL);

  for (i = 0; i < Nimages; i++) {
    
    assert (!starmap[i].points);

    ALLOCATE (starmap[i].points, StarMapPoint, NX_MAP*NY_MAP);
    starmap[i].Npoints = 0;

    for (ix = 0; ix < NX_MAP; ix++) {
      for (iy = 0; iy < NY_MAP; iy++) {
	if (starmap[i].stars[iy*NX_MAP + ix] < 1) continue;
	StarMapPoint *point = &starmap[i].points[starmap[i].Npoints];
	starmap[i].Npoints++;

	// set the pixel coordinates
	point[0].X = ix * starmap[i].Nx; // XXX fix 0.5 pixel offset
	point[0].Y = iy * starmap[i].Nx; // XXX fix 0.5 pixel offset

	// set the transformed coordinates
	XY_to_LM (&point[0].L, &point[0].M, point[0].X, point[0].Y, &images[i].coords);

	point[0].dL = 0.0;
	point[0].dM = 0.0;
      }
    }

    if (VERBOSE2) fprintf (stderr, "starmap: %d points for image %s\n", starmap[i].Npoints, images[i].name);
  }

  return (TRUE);
}

int checkStarMap(int N) {

  Image *images;
  off_t i, Nimages;
  double L, M, dLmax, dMmax;

  images = getimages(&Nimages, NULL);

  dLmax = dMmax = 0.0;

  float plateScale;
  if (images[N].coords.mosaic) {
    // NOTE: for the full pixel to sky plate scale, use this:
    // float plateScaleX = 3600.0*images[N].coords.mosaic->cdelt1*images[N].coords.cdelt1;
    // float plateScaleY = 3600.0*images[N].coords.mosaic->cdelt2*images[N].coords.cdelt2;

    // since we are compare L,M values, just need to compensate for focal plate to sky:
    float plateScaleX = 3600.0*fabs(images[N].coords.mosaic->cdelt1);
    float plateScaleY = 3600.0*fabs(images[N].coords.mosaic->cdelt2);
    plateScale = 0.5*(plateScaleX + plateScaleY);
  } else {
    // since we are compare L,M values, just need to compensate for arcsec vs degrees:
    plateScale = 3600.0;
  }

  for (i = 0; i < starmap[N].Npoints; i++) {

    // set the transformed coordinates
    XY_to_LM (&L, &M, starmap[N].points[i].X, starmap[N].points[i].Y, &images[N].coords);

    starmap[N].points[i].dL = plateScale*(starmap[N].points[i].L - L);
    starmap[N].points[i].dM = plateScale*(starmap[N].points[i].M - M);

    dLmax = MAX(fabs(starmap[N].points[i].dL), dLmax);
    dMmax = MAX(fabs(starmap[N].points[i].dM), dMmax);
  }

  if (VERBOSE2) fprintf (stderr, "max deviations for %s using %d pts (%d fitted) : %f, %f\n", images[N].name, starmap[N].Npoints, images[N].nFitAstrom, dLmax, dMmax);

  if (dLmax > DPOS_MAX) {
    if (VERBOSE) fprintf (stderr, "max deviations for %s using %d pts (%d fitted) : %f, %f\n", images[N].name, starmap[N].Npoints, images[N].nFitAstrom, dLmax, dMmax);
      return (FALSE);
  }
  if (dMmax > DPOS_MAX) {
      if (VERBOSE) fprintf (stderr, "max deviations for %s using %d pts (%d fitted) : %f, %f\n", images[N].name, starmap[N].Npoints, images[N].nFitAstrom, dLmax, dMmax);
      return (FALSE);
  }
  return (TRUE);
}

int printStarMap(int N, char *filename) {

  off_t i, Nimages;
  Image *images = getimages(&Nimages, NULL);
  double L, M;

  FILE *f = fopen (filename, "w");

  for (i = 0; i < starmap[N].Npoints; i++) {
    // set the transformed coordinates
    XY_to_LM (&L, &M, starmap[N].points[i].X, starmap[N].points[i].Y, &images[N].coords);

    fprintf (f, "%d %7.2f %f : %7.2f %7.2f : %7.2f %7.2f : %7.2f %7.2f\n", (int) i, starmap[N].points[i].X, starmap[N].points[i].Y, starmap[N].points[i].L, starmap[N].points[i].M, L, M, starmap[N].points[i].L - L, starmap[N].points[i].M - M);
  }
  fclose (f);
  return (TRUE);
}

