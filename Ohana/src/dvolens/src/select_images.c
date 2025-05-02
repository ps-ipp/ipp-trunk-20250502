# include "dvolens.h"

// for the purposes of dvolens / RepairLensing, we are only keeping warp images
int isGPC1warp (int photcode);

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t *Nimage) {
  
  Image *image;
  off_t i, j;
  double Ri[5], Di[5], Xi[5], Yi[5];
  
  double RmaxSkyRegion, RminSkyRegion, RmidSkyRegion, DminSkyRegion, DmaxSkyRegion;

  if (skylist[0].Nregions < 1) {
    *Nimage = 0;
    if (VERBOSE) fprintf (stderr, "no matching sky regions\n");
    return NULL;
  }

  INITTIME;

  RminSkyRegion = +360.0;
  RmaxSkyRegion = -360.0;
  DminSkyRegion = +90.0;
  DmaxSkyRegion = -90.0;

  /* compare with each region file */
  for (i = 0; i < skylist[0].Nregions; i++) { 
    RminSkyRegion = MIN(RminSkyRegion, skylist[0].regions[i][0].Rmin);
    RmaxSkyRegion = MAX(RmaxSkyRegion, skylist[0].regions[i][0].Rmax);
    DminSkyRegion = MIN(DminSkyRegion, skylist[0].regions[i][0].Dmin);
    DmaxSkyRegion = MAX(DmaxSkyRegion, skylist[0].regions[i][0].Dmax);
  }
  RmidSkyRegion = 0.5*(RminSkyRegion + RmaxSkyRegion);
  MARKTIME("create sky region coords: %f sec\n", dtime);

  if (VERBOSE) fprintf (stderr, "finding images\n");

  off_t D_NIMAGE = 6000;

  off_t nimage = 0;
  off_t NIMAGE = D_NIMAGE;
  ALLOCATE (image, Image, NIMAGE);
  
  // I need to ensure we get the images from any skycells in a warp
  double dD = 4.0;
  double dR = dD / cos(RAD_DEG*DminSkyRegion);
  RminSkyRegion -= dR;
  RmaxSkyRegion += dR;
  DminSkyRegion -= dD;
  DmaxSkyRegion += dD;

  // go through the complete list of images, selecting ones which overlap any region
  for (i = 0; i < Ntimage; i++) {
      
    if (!isGPC1warp(timage[i].photcode)) continue;
   
    /* define image corners - only Warps (TAN) /*/
    Xi[0] = 0;            Yi[0] = 0;
    Xi[1] = timage[i].NX; Yi[1] = 0;
    Xi[2] = timage[i].NX; Yi[2] = timage[i].NY;
    Xi[3] = 0;            Yi[3] = timage[i].NY;
    Xi[4] = 0;            Yi[4] = 0;

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

    // check that this image is even in range of the searched region
    if (DminImage > DmaxSkyRegion) continue;
    if (DmaxImage < DminSkyRegion) continue;
    
    // the sky region RA is defined to be 0 - 360.0
    if (RminImage > RmaxSkyRegion) continue;
    if (RmaxImage < RminSkyRegion) continue;

    image[nimage] = timage[i]; 

    nimage ++;
    if (nimage == NIMAGE) {
      NIMAGE += D_NIMAGE;
      D_NIMAGE = MAX (100000, D_NIMAGE * 1.5);
      REALLOCATE (image, Image, NIMAGE);
    }
  }
  MARKTIME("finish image selection: %f sec\n", dtime);

  if (VERBOSE) fprintf (stderr, "found "OFF_T_FMT" images\n", nimage);

  REALLOCATE (image, Image, MAX (nimage, 1));

  *Nimage  = nimage;
  return (image);
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1warp (int photcode) {

  if (photcode == 12000) return TRUE; // g-band
  if (photcode == 12100) return TRUE; // r-band
  if (photcode == 12200) return TRUE; // i-band
  if (photcode == 12300) return TRUE; // z-band
  if (photcode == 12400) return TRUE; // y-band
  if (photcode == 12500) return TRUE; // w-band

  return FALSE;
}
