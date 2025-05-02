# include "checkastro.h"

/* this function returns a list of all images which overlap the given SkyList (set of
   SkyRegions).  All images in the image catalog are tested once, so there is no check that an
   image already has been included.  LineNum stores the locations in the Image database of the
   list of images */

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage) {
  
  Image *image;
  off_t i, k, nimage, NIMAGE, D_NIMAGE;
  off_t *line_number;
  int found;
  double Rexp, Dexp;
  
  double RmaxSkyRegion, RminSkyRegion, RmidSkyRegion, DminSkyRegion, DmaxSkyRegion;

  if (skylist[0].Nregions < 1) {
    *Nimage = 0;
    *LineNumber = NULL;
    if (VERBOSE) fprintf (stderr, "no matching sky regions\n");
    return NULL;
  }

  INITTIME;

  RminSkyRegion = +360.0;
  RmaxSkyRegion = -360.0;
  DminSkyRegion = +90.0;
  DmaxSkyRegion = -90.0;

  D_NIMAGE = 6000;

  /* compare with each region file */
  for (i = 0; i < skylist[0].Nregions; i++) { 
    RminSkyRegion = MIN(RminSkyRegion, skylist[0].regions[i][0].Rmin);
    RmaxSkyRegion = MAX(RmaxSkyRegion, skylist[0].regions[i][0].Rmax);
    DminSkyRegion = MIN(DminSkyRegion, skylist[0].regions[i][0].Dmin);
    DmaxSkyRegion = MAX(DmaxSkyRegion, skylist[0].regions[i][0].Dmax);
  }
  RmidSkyRegion = 0.5*(RminSkyRegion + RmaxSkyRegion);
  MARKTIME("define sky region: %f sec\n", dtime);

  if (VERBOSE) fprintf (stderr, "finding images\n");

  nimage = 0;
  NIMAGE = D_NIMAGE;
  ALLOCATE (image, Image, NIMAGE);
  ALLOCATE (line_number, off_t, NIMAGE);
  
  // we now have a region of interest: RminSkyRegion - RmaxSkyRegion, DminSkyRegion - DmaxSkyRegion, 
  // we now want to find all gpc1 chips (WRP) for which the exposure center (DIS) is in this region,

  // go through the complete list of images, selecting ones which overlap the region
  for (i = 0; i < Ntimage; i++) {
      
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
    
    /* only check exposure center */
    if (!strcmp(&timage[i].coords.ctype[4], "-DIS")) {
      XY_to_RD (&Rexp, &Dexp, 0.0, 0.0, &timage[i].coords);
    } else {
      XY_to_RD (&Rexp, &Dexp, 0.0, 0.0, &timage[i].parent->coords);
    }
    Rexp = ohana_normalize_angle_to_midpoint (Rexp, RmidSkyRegion);

    // check that this image is even in range of the searched region
    if (Dexp + 1.5 > UserPatch.Dmax) continue;
    if (Dexp - 1.5 < UserPatch.Dmin) continue;
    
    // the sky region RA is defined to be 0 - 360.0
    if ((Dexp < 88) && (Dexp > -88)) {
      if (Rexp + 1.5/cos(Dexp*RAD_DEG) > UserPatch.Rmax) continue;
      if (Rexp - 1.5/cos(Dexp*RAD_DEG) < UserPatch.Rmin) continue;
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

  *Nimage  = nimage;
  *LineNumber = line_number;
  return (image);
}

