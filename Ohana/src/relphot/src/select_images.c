# include "relphot.h"

/* this function returns a list of all images which overlap the given SkyList (set of
   SkyRegions).  All images in the image catalog are tested once, so there is no check that an
   image already has been included.  LineNum stores the locations in the Image database of the
   list of images */

typedef struct {
  double Xc[5];
  double Yc[5];
  double Rc;
  double Dc;
} SkyRegionCoords;

void MakeSkyCoordIndex (SkyRegionCoords **skycoords_out, double **RmaxSky_out, off_t **index_out, SkyList *skylist);
void dsortindex (double *X, off_t *Y, int N);
off_t getRegionStartByRA (double R, double *Rref, off_t Nregions);

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, char *inSubset, off_t **LineNumber, off_t *Nimage, SkyRegion *region) {
  
  Image *image;
  off_t i, j, k, m, nStart, iSky, nimage, NIMAGE, D_NIMAGE;
  off_t *line_number;
  int InRange, found;
  double Ri[5], Di[5], Xi[5], Yi[5];
  Coords tcoords;
  
  if (skylist[0].Nregions < 1) {
    *Nimage = 0;
    *LineNumber = NULL;
    if (VERBOSE) fprintf (stderr, "no matching sky regions\n");
    return NULL;
  }

  INITTIME;

  // FILE *ftest = fopen ("image.region.dat", "w");

  // the comparison is made in the catalog local projection. below we set crval1,2
  InitCoords (&tcoords, "DEC--TAN");
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  double RminSkyRegion = region[0].Rmin;
  double RmaxSkyRegion = region[0].Rmax;
  double DminSkyRegion = region[0].Dmin;
  double DmaxSkyRegion = region[0].Dmax;

  double dD = CALIBRATE_STACKS_AND_WARPS ? 0.0 : 5.0;
  double dR = dD / cos(RAD_DEG*DminSkyRegion);
  RminSkyRegion -= dR;
  RmaxSkyRegion += dR;
  DminSkyRegion -= dD;
  DmaxSkyRegion += dD;

  double RmidSkyRegion = 0.5*(RminSkyRegion + RmaxSkyRegion);

  // generate a set of Rmin/Rmax values using this RmidSkyRegion for DEC bands of width
  // dDecBand (0.1 deg).  DecIndex = (Dec + 90) / dDecBand
  double *RminBand, *RmaxBand;
  float dDecBand = 0.1;
  int NDecBands = 180 / dDecBand + 1;
  ALLOCATE (RminBand, double, NDecBands);
  ALLOCATE (RmaxBand, double, NDecBands);
  for (i = 0; i < NDecBands; i++) {
    RminBand[i] = +360.0;
    RmaxBand[i] = -360.0;
  }
  
   /* compare with each region file */
  for (i = 0; i < skylist[0].Nregions; i++) { 
    int iDecBandMin = (skylist[0].regions[i][0].Dmin + 90.0) / dDecBand;
    int iDecBandMax = (skylist[0].regions[i][0].Dmax + 90.0) / dDecBand;

    double RminAlt = ohana_normalize_angle_to_midpoint (skylist[0].regions[i][0].Rmin, RmidSkyRegion);
    double RmaxAlt = ohana_normalize_angle_to_midpoint (skylist[0].regions[i][0].Rmax, RmidSkyRegion);

    for (j = iDecBandMin; j <= iDecBandMax; j++) {
      RminBand[j] = MIN(RminBand[j], RminAlt);
      RmaxBand[j] = MAX(RmaxBand[j], RmaxAlt);
    }
  }

  SkyRegionCoords *skycoords = NULL;
  double *RmaxSky = NULL;
  off_t *index = NULL;

  if (!USE_BASIC_CHECK) {
    MakeSkyCoordIndex (&skycoords, &RmaxSky, &index, skylist);
  }

  if (VERBOSE) fprintf (stderr, "finding images\n");

  D_NIMAGE = 6000;

  nimage = 0;
  NIMAGE = D_NIMAGE;
  ALLOCATE (image, Image, NIMAGE);
  ALLOCATE (line_number, off_t, NIMAGE);
  
  // go through the complete list of images, selecting ones which overlap any region
  for (i = 0; i < Ntimage; i++) {
      
    if (!(i % 300000)) fprintf (stderr, ".");

    // only include active photcodes in the analysis
    if (timage[i].photcode) {
      int Ns = GetActivePhotcodeIndex (timage[i].photcode);
      if (Ns < 0) continue;
    }

    /* exclude images by time */
    if (TimeSelect) {
      if (timage[i].tzero < TSTART) continue;
      if (timage[i].tzero > TSTOP) continue;
    }
    
    /* define image corners - note the DIS images (mosaic phu) are special */
    if (!strcmp(&timage[i].coords.ctype[4], "-DIS")) {
      Xi[0] = -0.5*timage[i].NX; Yi[0] = -0.5*timage[i].NY;
      Xi[1] = +0.5*timage[i].NX; Yi[1] = -0.5*timage[i].NY;
      Xi[2] = +0.5*timage[i].NX; Yi[2] = +0.5*timage[i].NY;
      Xi[3] = -0.5*timage[i].NX; Yi[3] = +0.5*timage[i].NY;
      Xi[4] = -0.5*timage[i].NX; Yi[4] = -0.5*timage[i].NY;
    } else {
      Xi[0] = 0;            Yi[0] = 0;
      Xi[1] = timage[i].NX; Yi[1] = 0;
      Xi[2] = timage[i].NX; Yi[2] = timage[i].NY;
      Xi[3] = 0;            Yi[3] = timage[i].NY;
      Xi[4] = 0;            Yi[4] = 0;
    }
    found = FALSE;

    /* transform corners to ra,dec -- costs ~3sec for 3M images (pikake) */
    double RminImage = RmidSkyRegion + 180.0;
    double RmaxImage = RmidSkyRegion - 180.0;
    double DminImage = +90.0;
    double DmaxImage = -90.0;
    for (j = 0; j < 5; j++) {
      XY_to_RD (&Ri[j], &Di[j], Xi[j], Yi[j], &timage[i].coords);
      Ri[j] = ohana_normalize_angle_to_midpoint (Ri[j], RmidSkyRegion);

      RminImage = MIN(RminImage, Ri[j]);
      RmaxImage = MAX(RmaxImage, Ri[j]);
      DminImage = MIN(DminImage, Di[j]);
      DmaxImage = MAX(DmaxImage, Di[j]);
    }
    if (RmaxImage - RminImage > 180.0) {
      double tmp = RminImage;
      RminImage = RmaxImage - 360.0;
      RmaxImage = tmp;
    }

    // check that this image is even in range of the searched region
    if (USE_FULL_OVERLAP) {
      // for full overlap, the image must be completely inside the region of interest
      if (DmaxImage > DmaxSkyRegion) continue;
      if (DminImage < DminSkyRegion) continue;
      
      // the sky region RA is defined to be 0 - 360.0
      if (RmaxImage > RmaxSkyRegion) continue;
      if (RminImage < RminSkyRegion) continue;
    } else {
      if (DminImage > DmaxSkyRegion) continue;
      if (DmaxImage < DminSkyRegion) continue;
      
      // the sky region RA is defined to be 0 - 360.0
      if (RminImage > RmaxSkyRegion) continue;
      if (RmaxImage < RminSkyRegion) continue;
    }
      
    // the above checks are only valid for the outermost region.  however, at a given
    // declination, the range of RA is limited by the actual catalog boundaries
    // check if this image is in range for the Dmax and Dmin locations
    if (USE_FULL_OVERLAP && strcmp(&timage[i].coords.ctype[4], "-DIS")) {

      int iDecBandMin = (DminImage + 90.0) / dDecBand;
      int iDecBandMax = (DmaxImage + 90.0) / dDecBand;

      // the sky region RA is defined to be 0 - 360.0
      if (RminImage < RminBand[iDecBandMin]) { 
	if (VERBOSE2) fprintf (stderr, "skip image %s (%f,%f) on boundary\n", timage[i].name, 0.5*(RminImage + RmaxImage), 0.5*(DminImage + DmaxImage));
	continue;
      }
      if (RminImage < RminBand[iDecBandMax]) {
	if (VERBOSE2) fprintf (stderr, "skip image %s (%f,%f) on boundary\n", timage[i].name, 0.5*(RminImage + RmaxImage), 0.5*(DminImage + DmaxImage));
	continue;
      }
      if (RmaxImage > RmaxBand[iDecBandMin]) {
	if (VERBOSE2) fprintf (stderr, "skip image %s (%f,%f) on boundary\n", timage[i].name, 0.5*(RminImage + RmaxImage), 0.5*(DminImage + DmaxImage));
	continue;
      }
      if (RmaxImage > RmaxBand[iDecBandMax]) {
	if (VERBOSE2) fprintf (stderr, "skip image %s (%f,%f) on boundary\n", timage[i].name, 0.5*(RminImage + RmaxImage), 0.5*(DminImage + DmaxImage));
	continue;
      }
    }

    // fprintf (ftest, "%f %f : %f %f : %d %d\n", RminImage, RmaxImage, DminImage, DmaxImage, timage[i].tzero, timage[i].photcode);

    // image overlaps region, keep it
    if (USE_BASIC_CHECK) goto found_it;

    // RA(nStart) is guaranteed to be < RminImage: -- costs 0.5sec for 3M images
    nStart = getRegionStartByRA (RminImage, RmaxSky, skylist[0].Nregions);

    /* compare with each region file */
    for (iSky = nStart; (iSky < skylist[0].Nregions) && !found; iSky++) { 

      m = index[iSky];

      /* we make positional comparisons in the projection of catalog */
      tcoords.crval1 = skycoords[m].Rc;
      tcoords.crval2 = skycoords[m].Dc;

      /* transform corner coords to X,Y in this catalog system */
      InRange = TRUE;
      for (j = 0; (j < 5) && InRange; j++) {
	InRange = RD_to_XY (&Xi[j], &Yi[j], Ri[j], Di[j], &tcoords);
      }
      if (!InRange) continue;

      /* check if image corner inside catalog */
      for (j = 0; (j < 4) && !found; j++) {
	found = corner_check (&Xi[j], &Yi[j], &skycoords[m].Xc[0], &skycoords[m].Yc[0]);
	if (found) goto found_it;
      }
      /* check if catalog corner inside image */
      for (j = 0; (j < 4) && !found; j++) {
	found = corner_check (&skycoords[m].Xc[j], &skycoords[m].Yc[j], &Xi[0], &Yi[0]);
	if (found) goto found_it;
      }
      /* check if edges cross */
      for (j = 0; (j < 4) && !found; j++) {
	for (k = 0; (k < 4) && !found; k++) {
	  found = edge_check (&Xi[j], &Yi[j], &skycoords[m].Xc[k], &skycoords[m].Yc[k]);
	  if (found) goto found_it;
	}
      }
    }
    if (!found) continue;

  found_it:
    image[nimage] = timage[i]; 
    inSubset[i] = TRUE;
    line_number[nimage] = i;
    nimage ++;
    if (nimage == NIMAGE) {
      NIMAGE += D_NIMAGE;
      D_NIMAGE = MAX (100000, D_NIMAGE * 1.5);
      REALLOCATE (image, Image, NIMAGE);
      REALLOCATE (line_number, off_t, NIMAGE);
    }
  }
  MARKTIME("\nfinish image selection: %f sec\n", dtime);

  // fclose (ftest);

  if (VERBOSE) fprintf (stderr, "found "OFF_T_FMT" images\n", nimage);

  REALLOCATE (image, Image, MAX (nimage, 1));
  REALLOCATE (line_number, off_t, MAX (nimage, 1));

  FREE (RmaxBand);
  FREE (RminBand);

  if (!USE_BASIC_CHECK) {
    free (skycoords);
    free (RmaxSky);
    free (index);
  }

  *Nimage  = nimage;
  *LineNumber = line_number;
  return (image);
}

/* check if line between points 0 and 1 of x1
   crosses line between points 0 and 1 of x2 */
int edge_check (double *x1, double *y1, double *x2, double *y2) {

  double theta1, theta2;

  theta1 = opening_angle (x1[0], y1[0], x2[0], y2[0], x1[1], y1[1]); 
  theta2 = opening_angle (x1[0], y1[0], x2[0], y2[0], x2[1], y2[1]); 

  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  theta1 = opening_angle (x2[0], y2[0], x1[1], y1[1], x2[1], y2[1]); 
  theta2 = opening_angle (x2[0], y2[0], x1[1], y1[1], x1[0], y1[0]); 
  
 
  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  return (TRUE);

}

/* check if point x1,y1 is in box formed by x2[0-4] */
int corner_check (double *x1, double *y1, double *x2, double *y2) {

  int i;
  double theta;

  theta = 0;

  for (i = 0; i < 4; i++) {
    theta += opening_angle (x2[i], y2[i], x1[0], y1[0], x2[i+1], y2[i+1]); 
  }
  if (fabs(theta) > 6) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

/* returns the opening angle between the three points (2 is in middle) 
   in range -pi to pi */

double opening_angle (double x1, double y1, double x2, double y2, double x3, double y3) {

  double dx1, dy1, dx2, dy2, ct, st, theta;

  dx1 = x1 - x2;
  dy1 = y1 - y2;
  
  dx2 = x3 - x2;
  dy2 = y3 - y2;
  
  ct = (dx1*dx2 + dy1*dy2);
  st = (dx1*dy2 - dx2*dy1);

  theta = atan2 (st, ct);

  return (theta);

}

void dsortindex (double *X, off_t *Y, int N) {

# define SWAPFUNC(A,B){ double tmpf; off_t tmpi; \
  tmpf = X[A]; X[A] = X[B]; X[B] = tmpf; \
  tmpi = Y[A]; Y[A] = Y[B]; Y[B] = tmpi; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

off_t getRegionStartByRA (double R, double *Rref, off_t Nregions) {

  // use bisection to find the overlapping mosaic

  off_t Nlo, Nhi, N;

  // find the last mosaic before start
  Nlo = 0; Nhi = Nregions;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (Rref[N] < R) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N, Nregions);
    }
  }
  return (Nlo);
}

void MakeSkyCoordIndex (SkyRegionCoords **skycoords_out, double **RmaxSky_out, off_t **index_out, SkyList *skylist) {

  off_t i;
  SkyRegionCoords *skycoords;
  Coords tcoords;
  double *RmaxSky;
  off_t *index;
  double dx, dy;

  // the comparison is made in the catalog local projection. below we set crval1,2
  InitCoords (&tcoords, "DEC--TAN");
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  /* compare with each region file */
  ALLOCATE (skycoords, SkyRegionCoords, skylist[0].Nregions);

  ALLOCATE (RmaxSky, double, skylist[0].Nregions);
  ALLOCATE (index, off_t, skylist[0].Nregions);

  for (i = 0; i < skylist[0].Nregions; i++) { 

    /* we make positional comparisons in the projection of catalog */
    skycoords[i].Rc = 0.5*(skylist[0].regions[i][0].Rmax + skylist[0].regions[i][0].Rmin);
    skycoords[i].Dc = 0.5*(skylist[0].regions[i][0].Dmax + skylist[0].regions[i][0].Dmin);
    tcoords.crval1 = skycoords[i].Rc;
    tcoords.crval2 = skycoords[i].Dc;

    /* define catalog corners */
    RD_to_XY (&skycoords[i].Xc[0], &skycoords[i].Yc[0], skylist[0].regions[i][0].Rmin, skylist[0].regions[i][0].Dmin, &tcoords);
    RD_to_XY (&skycoords[i].Xc[1], &skycoords[i].Yc[1], skylist[0].regions[i][0].Rmax, skylist[0].regions[i][0].Dmin, &tcoords);
    RD_to_XY (&skycoords[i].Xc[2], &skycoords[i].Yc[2], skylist[0].regions[i][0].Rmax, skylist[0].regions[i][0].Dmax, &tcoords);
    RD_to_XY (&skycoords[i].Xc[3], &skycoords[i].Yc[3], skylist[0].regions[i][0].Rmin, skylist[0].regions[i][0].Dmax, &tcoords);
    skycoords[i].Xc[4] = skycoords[i].Xc[0];    
    skycoords[i].Yc[4] = skycoords[i].Yc[0];    

    RmaxSky[i] = skylist[0].regions[i][0].Rmax;
    index[i] = i;

    dx = 0.02*(skycoords[i].Xc[2] - skycoords[i].Xc[0]);
    dy = 0.02*(skycoords[i].Yc[2] - skycoords[i].Yc[0]);
    skycoords[i].Xc[0] -= dx; skycoords[i].Yc[0] -= dy;
    skycoords[i].Xc[1] += dx; skycoords[i].Yc[1] -= dy;
    skycoords[i].Xc[2] += dx; skycoords[i].Yc[2] += dy;
    skycoords[i].Xc[3] -= dx; skycoords[i].Yc[3] += dy;
    skycoords[i].Xc[4] -= dx; skycoords[i].Yc[4] -= dy;
  }

  dsortindex (RmaxSky, index, skylist[0].Nregions);

  *RmaxSky_out = RmaxSky;
  *index_out = index;
  *skycoords_out = skycoords;

  return;
}
