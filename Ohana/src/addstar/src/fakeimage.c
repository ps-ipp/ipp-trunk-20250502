# include "addstar.h"

Image *fakeimage (char *rootname, off_t *Nimage, int photcode) {

  int i, Nx, Ny, Nchips;
  double pltscale, pixscale;
  double Rmin, Rmax, Dmin, Dmax;
  double dX, dY, r, d;
  char chipname[16], chipdata[256], name[117];
  char *config;
  Image *image;
  e_time MosaicTime;
  Coords MOSAIC;

  /* this is a somewhat bogus method to set a time for the exposure */
  struct timeval now;
  long int seedval;

  gettimeofday (&now, NULL);
  seedval = now.tv_sec + now.tv_usec;
  srand48(seedval);

  MosaicTime = 0xffffffff * drand48();
  fprintf (stderr, "time: %x\n", MosaicTime);

  /* load in the camera layout file */
  config = LoadConfigFile (CameraLayout);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find camera layout file %s\n", CameraLayout);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded camera layout file: %s\n", CameraLayout);

  /* create a mosaic distortion structure */
  InitCoords (&MOSAIC, "DEC--DIS");
  MOSAIC.crval1 = FAKE_RA;
  MOSAIC.crval2 = FAKE_DEC;
  
  /* mosaic 'pixels' are millimeters */
  ScanConfig (config, "PLATE_SCALE",   "%lf", 0, &pltscale);
  MOSAIC.cdelt1 = MOSAIC.cdelt2 = pltscale / 3600.0;

  MOSAIC.pc1_1 =  cos(FAKE_THETA*RAD_DEG);
  MOSAIC.pc1_2 = -sin(FAKE_THETA*RAD_DEG);
  MOSAIC.pc2_1 =  sin(FAKE_THETA*RAD_DEG);
  MOSAIC.pc2_2 =  cos(FAKE_THETA*RAD_DEG);

  MOSAIC.Npolyterms = 3;
  ScanConfig (config, "DPLATE_X",   "%lf", 0, &pltscale);
  MOSAIC.polyterms[3][0] = pltscale;  // L : X^3 Y^0
  MOSAIC.polyterms[5][0] = pltscale;  // L : X^1 Y^2
  ScanConfig (config, "DPLATE_Y",   "%lf", 0, &pltscale);
  MOSAIC.polyterms[4][1] = pltscale;  // M : X^0 Y^0
  MOSAIC.polyterms[6][1] = pltscale;  // M : X^2 Y^0

  /* some basic data about the chisp */
  ScanConfig (config, "NCHIPS", "%d", 0, &Nchips);
  ScanConfig (config, "NAXIS1", "%d", 0, &Nx);
  ScanConfig (config, "NAXIS2", "%d", 0, &Ny);
  ScanConfig (config, "PIXEL_SCALE", "%lf", 0, &pixscale);

  ALLOCATE (image, Image, Nchips + 1);
  
  Rmin = Rmax = Dmin = Dmax = 0;

  /* define the chip images (1 - Nchips) */
  for (i = 0; i < Nchips; i++) {
    /* this is the addstar name for the chip in the camera */
    snprintf (chipname, 16, "CHIP.%03d", i);
    ScanConfig (config, chipname, "%s", 0, chipdata);

    sscanf (chipdata, "%s %lf %lf", chipname, &dX, &dY);
    // if (VERBOSE) fprintf (stderr, "chip %s (%f,%f)\n", chipname, dX, dY);

    /* this is the camera-specific name of a chip */
    snprintf (name, 117, "%s.%s", rootname, chipname);
    strcpy (image[i+1].name, name);

    InitCoords (&image[i+1].coords, "DEC--WRP");
    
    image[i+1].coords.crval1 = dX*pixscale;
    image[i+1].coords.crval2 = dY*pixscale;

    image[i+1].coords.cdelt1 = image[i+1].coords.cdelt2 = pixscale;

    image[i+1].coords.mosaic = &MOSAIC;

    image[i+1].sidtime  = 0.0;
    image[i+1].latitude = 0.0;

    image[i+1].cerror = 0.0;
    
    image[i+1].NX = Nx;
    image[i+1].NY = Ny;

    image[i+1].photcode = photcode;

    image[i+1].exptime = 0.0;
  
    image[i+1].apmifit = 0.0;
    image[i+1].dapmifit = 0.0;

    image[i+1].detection_limit = 0.0;
    image[i+1].saturation_limit = 0.0;
    image[i+1].fwhm_x = 0.0;
    image[i+1].fwhm_y = 0.0;
    image[i+1].tzero = MosaicTime;
    image[i+1].trate = 0;
    image[i+1].secz = 1.0;
    image[i+1].ccdnum = 0xff;

    image[i+1].McalPSF   = 0.0;
    image[i+1].McalAPER  = 0.0;
    image[i+1].McalChiSq = NAN;
    image[i+1].dMcal     = NAN;
    image[i+1].flags = 0;

    image[i+1].nstar = 0;

    /* check if chip hits outer bounds of mosaic */
    XY_to_RD (&r, &d, 0, 0, &image[i+1].coords);
    Rmin = MIN (Rmin, r);
    Rmax = MAX (Rmax, r);
    Dmin = MIN (Dmin, d);
    Dmax = MAX (Dmax, d);
    XY_to_RD (&r, &d, Nx, 0, &image[i+1].coords);
    Rmin = MIN (Rmin, r);
    Rmax = MAX (Rmax, r);
    Dmin = MIN (Dmin, d);
    Dmax = MAX (Dmax, d);
    XY_to_RD (&r, &d, 0, Ny, &image[i+1].coords);
    Rmin = MIN (Rmin, r);
    Rmax = MAX (Rmax, r);
    Dmin = MIN (Dmin, d);
    Dmax = MAX (Dmax, d);
    XY_to_RD (&r, &d, Nx, Ny, &image[i+1].coords);
    Rmin = MIN (Rmin, r);
    Rmax = MAX (Rmax, r);
    Dmin = MIN (Dmin, d);
    Dmax = MAX (Dmax, d);
  }

  /* define the mosaic image */
  strcpy (image[0].name, rootname);

  image[0].coords = MOSAIC;
  strcpy (image[0].coords.ctype, MOSAIC.ctype);

  image[0].sidtime  = 0.0;
  image[0].latitude = 0.0;
  image[0].cerror = 0.0;
    
  image[0].NX = Rmax - Rmin;
  image[0].NY = Dmax - Dmin;

  image[0].photcode = photcode;

  image[0].exptime = 0.0;
  image[0].apmifit = 0.0;
  image[0].dapmifit = 0.0;
  image[0].detection_limit = 0.0;
  image[0].saturation_limit = 0.0;
  image[0].fwhm_x = 0.0;
  image[0].fwhm_y = 0.0;
  image[0].tzero = MosaicTime;
  image[0].trate = 0;
  image[0].secz = 1.0;
  image[0].ccdnum = 0xff;

  image[0].McalPSF   = 0.0;
  image[0].McalAPER  = 0.0;
  image[0].McalChiSq = NAN;
  image[0].dMcal     = NAN;
  image[0].flags = 0;

  image[0].nstar = 0;

  // XXX need to set the imageID here

  *Nimage = Nchips + 1;
  return (image);
}
