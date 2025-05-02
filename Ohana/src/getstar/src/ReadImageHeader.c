# include "dvoImageOverlaps.h"

// for DIS/WRP sets, we need to save the DIS set and supply it to the WRP entries
static Coords *mosaic = NULL;

void initMosaicCoords () {
  mosaic = NULL;
}

void saveMosaicCoords (Coords *input) {
  mosaic = input;
}

/* read an image header corresponding to a CMF / CMP data block */
int ReadImageHeader (Header *header, Image *image) {

  int Nastro, Nx, Ny, haveNx, haveNy;
  double tmp;

  /* get astrometry information */
  if (!GetCoords (&image[0].coords, header)) {
    fprintf (stderr, "no astrometric solution in header\n");
    return (FALSE);
  }
  // XXX currently, image uses an unsigned short for NX,XY. this is rather restrictive
  // and needs to be at least checked.
  haveNx = gfits_scan (header, "NAXIS1",   "%d", 1, &Nx);
  haveNy = gfits_scan (header, "NAXIS2",   "%d", 1, &Ny);

  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (header, "IMNAXIS1",   "%d", 1, &Nx);
      haveNy = gfits_scan (header, "IMNAXIS2",   "%d", 1, &Ny);
  }

  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (header, "ZNAXIS1",   "%d", 1, &Nx);
      haveNy = gfits_scan (header, "ZNAXIS2",   "%d", 1, &Ny);
  }

  if (!haveNx || !haveNy) {
      fprintf (stderr, "missing image dimensions in header\n");
      return (FALSE);
  }

  if ((Nx < 0) || (Nx > 0xffff)) {
    fprintf (stderr, "WARNING: NX, NY out of range : image boundary will be wrong\n");
  }
  image[0].NX = Nx;
  image[0].NY = Ny;

  /* 
  if (!gfits_scan (header, "TZERO",   "%d",  1, &image[0].tzero)) {
    image[0].tzero = parse_time (header);
  }
  gfits_scan (header, ExptimeKeyword,  "%lf", 1, &tmp);
  image[0].exptime = tmp;
  */
  image[0].tzero = 1;
  image[0].exptime = 1;

  /* only load astrometry, NAXIS1,2, and time if this is a MOSAIC_PHU (ctype is ....-DIS) */
  if (!strcmp (&image[0].coords.ctype[4], "-DIS")) {
    saveMosaicCoords (&image[0].coords);
    return (TRUE);
  }

  /* require Nastro > 0 unless or ACCEPT_ASTROM */
  Nastro = 0;
  gfits_scan (header, "NASTRO", "%d", 1, &Nastro);
  if ((Nastro == 0) && !ACCEPT_ASTROM) {
    fprintf (stderr, "bad astrometric solution in header\n");
    return (FALSE);
  }
  if (!strcmp (&image[0].coords.ctype[4], "-WRP")) {
    if (mosaic == NULL) {
      fprintf (stderr, "no mosaic for WRP image (use -mosaic)\n");
      return (FALSE);
    }
    image[0].coords.mosaic = mosaic;
  } else {
    /* force image to lie in 0-360 range */
    image[0].coords.crval1 = ohana_normalize_angle (image[0].coords.crval1);
  }

  { 
    double R, D;
    /* sanity check on the image coordinates */
    XY_to_RD (&R, &D, 0.5*Nx, 0.5*Ny, &image[0].coords);
    if (!finite(R) || !finite(D)) {
      fprintf (stderr, "corrupted header coordinates, skipping\n");
      return (FALSE);
    }
  }
    
  // CERROR & CERSTD in data file are in arcsec, image structure uses units of 20 mas
  // if (!gfits_scan (header, "CERROR",   "%lf", 1, &tmp)) tmp = 1.0;
  // 2021.11.03 : getstar now checks CERSTD, not CERROR
  if (!gfits_scan (header, "CERSTD",   "%lf", 1, &tmp)) tmp = 1.0;
  image[0].cerror = tmp * 50.0;
 
  /*** why are we no longer using APMIFIT?? ***/
  tmp = 0;
  /* gfits_scan (header, "APMIFIT",  "%lf", 1, &tmp); */
  image[0].apmifit = tmp;

  tmp = 0;
  /* gfits_scan (header, "dAPMIFIT", "%lf", 1, &tmp); */
  image[0].dapmifit = tmp;

  tmp = 0;
  gfits_scan (header, "FLIMIT",   "%lf", 1, &tmp);
  image[0].detection_limit = tmp * 10.0;

  tmp = 0;
  gfits_scan (header, "FSATUR",   "%lf", 1, &tmp);
  image[0].saturation_limit = tmp * 10.0;

  tmp = 0;
  gfits_scan (header, "FWHM_X",   "%lf", 1, &tmp);
  image[0].fwhm_x = tmp * 25.0 * image[0].coords.cdelt1 * 3600.0;

  tmp = 0;
  gfits_scan (header, "FWHM_Y",   "%lf", 1, &tmp);
  image[0].fwhm_y = tmp * 25.0 * image[0].coords.cdelt1 * 3600.0;

  image[0].trate  = 0.0;
  image[0].secz   = NAN;
  image[0].ccdnum = 0;

  /* secz is in units milli-airmass */
  image[0].McalPSF   = 0.0;
  image[0].McalAPER  = 0.0;
  image[0].McalChiSq = NAN;
  image[0].flags = 0;

  return (TRUE);
}
