# include "photdbc.h"

/* this function returns a list of all images which overlap the given SkyList (set of
   SkyRegions).  All images in the image catalog are tested once, so there is no check that an
   image already has been included.  LineNum stores the locations in the Image database of the
   list of images */

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage, int UseFullOverlap) {
  
  Image *image;
  off_t i, j, k, nimage, NIMAGE, D_NIMAGE;
  off_t *line_number;
  int found;
  double Ri[5], Di[5], Xi[5], Yi[5], dx, dy;
  Coords tcoords;
  SkyRegionCoords *skycoords;
  
  double RmaxSkyRegion, RminSkyRegion, RmidSkyRegion, DminSkyRegion, DmaxSkyRegion;

  double *RmaxSky;
  off_t *index;

  if (skylist[0].Nregions < 1) {
    *Nimage = 0;
    *LineNumber = NULL;
    if (VERBOSE) fprintf (stderr, "no matching sky regions\n");
    return NULL;
  }

  // if no region is selected, we are choosing the whole sky; I should sort-circuit
  // this function in that case

  INITTIME;

  // the comparison is made in the catalog local projection. below we set crval1,2
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
      
    /* exclude images by photcode, or equiv photcode, if specified */
    if (NphotcodesDrop > 0) {
      if (timage[i].photcode == 0) goto keep_image; // PHU image can be immediately passed in here
      found = FALSE; // here 'found' refers to a match to a drop photcode
      for (k = 0; (k < NphotcodesDrop) && !found; k++) {
	if (photcodesDrop[k][0].code == timage[i].photcode) found = TRUE;
	if (photcodesDrop[k][0].code == GetPhotcodeEquivCodebyCode(timage[i].photcode)) found = TRUE;
      }
      if (found) continue;
    }
keep_image:

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
	RmaxImage = RminImage;
	RminImage = tmp - 360.0;
    }

    // check that this image is even in range of the searched region
    if (DminImage > DmaxSkyRegion) continue;
    if (DmaxImage < DminSkyRegion) continue;
    
    // the sky region RA is defined to be 0 - 360.0
    if (RminImage > RmaxSkyRegion) continue;
    if (RmaxImage < RminSkyRegion) continue;

    // require that the full image be inside the region of interest (only for non PHU
    // images) XXX : if we calibrate the mosaic, require that the full mosaic be inside
    // the region and all of its chips..
    if (UseFullOverlap && strcmp(&timage[i].coords.ctype[4], "-DIS")) {
      if (RmaxImage > REGION.Rmax) continue;
      if (RminImage < REGION.Rmin) continue;
      if (DmaxImage > REGION.Dmax) continue;
      if (DminImage < REGION.Dmin) continue;
    }

    image[nimage] = timage[i]; 
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
