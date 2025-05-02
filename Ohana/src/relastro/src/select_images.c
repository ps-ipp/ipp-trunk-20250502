# include "relastro.h"

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

void dsortindex (double *X, off_t *Y, int N);
off_t getRegionStartByRA (double R, double *Rref, off_t Nregions);

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage, int UseFullOverlap) {
  
  Image *image;
  off_t i, j, k, m, nStart, iSky, nimage, NIMAGE, D_NIMAGE;
  off_t *line_number;
  int InRange, found;
  double Ri[5], Di[5], Xi[5], Yi[5], dx, dy;
  SkyRegionCoords *skycoords;
  
  double RmaxSkyRegion, RminSkyRegion, RmidSkyRegion, DminSkyRegion, DmaxSkyRegion;

  double *RmaxSky;
  off_t *index;

  int badImage = 
    ID_IMAGE_ASTROM_POOR | 
    ID_IMAGE_ASTROM_FAIL | 
    ID_IMAGE_ASTROM_FEW;

  if (skylist[0].Nregions < 1) {
    *Nimage = 0;
    *LineNumber = NULL;
    if (VERBOSE) fprintf (stderr, "no matching sky regions\n");
    return NULL;
  }

  INITTIME;

  // the comparison is made in the catalog local projection. below we set crval1,2
  Coords tcoords;
  InitCoords (&tcoords, "DEC--TAN");
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  ALLOCATE (skycoords, SkyRegionCoords, skylist[0].Nregions);

  ALLOCATE (RmaxSky, double, skylist[0].Nregions);
  ALLOCATE (index, off_t, skylist[0].Nregions);

  RminSkyRegion = +360.0;
  RmaxSkyRegion = -360.0;
  DminSkyRegion = +90.0;
  DmaxSkyRegion = -90.0;

  D_NIMAGE = 6000;

  /* compare with each region file */
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

    RminSkyRegion = MIN(RminSkyRegion, skylist[0].regions[i][0].Rmin);
    RmaxSkyRegion = MAX(RmaxSkyRegion, skylist[0].regions[i][0].Rmax);
    DminSkyRegion = MIN(DminSkyRegion, skylist[0].regions[i][0].Dmin);
    DmaxSkyRegion = MAX(DmaxSkyRegion, skylist[0].regions[i][0].Dmax);
  }
  RmidSkyRegion = 0.5*(RminSkyRegion + RmaxSkyRegion);
  MARKTIME("create sky region coords: %f sec\n", dtime);

  dsortindex (RmaxSky, index, skylist[0].Nregions);
  MARKTIME("sort sky coords: %f sec\n", dtime);

  if (VERBOSE) fprintf (stderr, "finding images\n");

  nimage = 0;
  NIMAGE = D_NIMAGE;
  ALLOCATE (image, Image, NIMAGE);
  ALLOCATE (line_number, off_t, NIMAGE);
  
  // go through the complete list of images, selecting ones which overlap any region
  for (i = 0; i < Ntimage; i++) {
      
    if (FALSE && !strncmp(timage[i].name, "o6406g0242o", 10)) {
      fprintf (stderr, "test image 1\n");
    }
    if (FALSE && !strncmp(timage[i].name, "o6227g0311o", 10)) {
      fprintf (stderr, "test image 2\n");
    }

    // RINGS.V3.skycell.2634.034.stk.3411510.skycal.3811673.cmf[SkyChip.hdr]
    if (!strncmp(timage[i].name, "RINGS.V3.skycell.2634.034.stk.3411510", strlen("RINGS.V3.skycell.2634.034.stk.3411510"))) {
      fprintf (stderr, "test image 2\n");
    }

    if (FALSE && (i >= 42819857) && (i < 42819972)) {
      fprintf (stderr, "test image %s\n", timage[i].name);
    }

    // allow certain cameras to stay static
    if (SKIP_PS1_CHIP  && isGPC1chip (timage[i].photcode)) continue;
    if (SKIP_PS1_STACK && isGPC1stack(timage[i].photcode)) continue;
    if (SKIP_HSC       && isHSCchip  (timage[i].photcode)) continue;
    if (SKIP_CFH       && isCFHchip  (timage[i].photcode)) continue;
    
    /* select images by photcode, or equiv photcode, if specified */
    if (NphotcodesKeep > 0) {
      found = FALSE;
      // we have to keep DIS mosaics explicitly (photcode = 0)
      if (!strcmp(&timage[i].coords.ctype[4], "-DIS")) found = TRUE;
      for (k = 0; (k < NphotcodesKeep) && !found; k++) {
	if (photcodesKeep[k][0].code == timage[i].photcode) found = TRUE;
	if (photcodesKeep[k][0].code == GetPhotcodeEquivCodebyCode(timage[i].photcode)) found = TRUE;
      }
      if (!found) continue;
    }
    if (NphotcodesSkip > 0) {
      found = FALSE;
      for (k = 0; (k < NphotcodesSkip) && !found; k++) {
	if (photcodesSkip[k][0].code == timage[i].photcode) found = TRUE;
	if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(timage[i].photcode)) found = TRUE;
      }
      if (found) continue;
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
    if (FALSE && (RmaxImage - RminImage > 180.0)) {
	double tmp = RminImage;
	RmaxImage = RminImage;
	RminImage = tmp - 360.0;
    }

    // check that this image is even in range of the searched region
    if (DminImage > DmaxSkyRegion) continue;
    if (DmaxImage < DminSkyRegion) continue;
    
    if (KEEP_ALL_IMAGES_RA) goto found_it;

    // the sky region RA is defined to be 0 - 360.0
    if (RminImage > RmaxSkyRegion) continue;
    if (RmaxImage < RminSkyRegion) continue;

    if (FALSE && !strncmp(timage[i].name, "o5903g0638o", 10) && (timage[i].photcode == 10355)) {
      fprintf (stderr, "test image\n");
    }

    // require that the full image be inside the region of interest (only for non PHU
    // images) XXX : if we calibrate the mosaic, require that the full mosaic be inside
    // the region and all of its chips..
    if (UseFullOverlap && strcmp(&timage[i].coords.ctype[4], "-DIS")) {
      if (RmaxImage > UserPatch.Rmax) continue;
      if (RminImage < UserPatch.Rmin) continue;
      if (DmaxImage > UserPatch.Dmax) continue;
      if (DminImage < UserPatch.Dmin) continue;
    }

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
    /* always allow 'few' images to succeed, if possible */
    if (image[nimage].flags & ID_IMAGE_ASTROM_FEW) { 
      image[nimage].flags &= ~ID_IMAGE_ASTROM_FEW;
    }
    if (RESET) {
      // this only resets the astrometry image flags, not the photometry ones
      image[nimage].flags &= ~badImage;
    }
    line_number[nimage] = i;
    nimage ++;
    if (nimage == NIMAGE) {
      NIMAGE += D_NIMAGE;
      D_NIMAGE = MAX (100000, D_NIMAGE * 1.5);
      REALLOCATE (image, Image, NIMAGE);
      REALLOCATE (line_number, off_t, NIMAGE);
    }
  }
  MARKTIME("finish image selection: %f sec\n", dtime);

  if (VERBOSE) fprintf (stderr, "found "OFF_T_FMT" images\n", nimage);

  REALLOCATE (image, Image, MAX (nimage, 1));
  REALLOCATE (line_number, off_t, MAX (nimage, 1));
  free (skycoords);
  free (RmaxSky);
  free (index);

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
