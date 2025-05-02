# include "gophot.h"

# define TestConfig(A,B,C,D,E) { if (!ScanConfig (A,B,C,D,E)) { fprintf (stderr, B); exit (1); }}

int ConfigInit (int *argc, char **argv) {

  char *config, *file;
  float fwhm, ar, tilt;
  char line[128];
  int level;
  float gmajwid, gxwid, gywid, fwhmx, fwhmy;
  float scalefb, fbmin, scaleab, abmin, scalemb, ambmin;

  /*** load configuration info ***/
  file = argv[3];
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    exit (0);
  }

  set_verbosity (0);  /* initializes function data */

  /* flags -- OK for now, not really used much */
  flags[1] =  PGAUSS;  /* psf type */
  flags[2] =  NONE2;   /* sky type */
  flags[3] =  NONE3;   /* objtype_out */
  flags[4] =  FALSE;   /* output shadow file? */
  flags[5] =  FALSE;   /* image out? */
  flags[6] =  FALSE;   /* warmstart? */
  flags[7] =  FALSE;   /* load input shadow file? */
  flags[8] =  FALSE;   /* objtype_in */
  flags[10] = FALSE;   /* aperture correction file? */

  onestar = pseud2d;
  twostar = pseud4d;
  skyfun  = skyfun_plane;

  /* files:
     files[2] - image_out 
     files[3] - objects_in      - UNUSED
     files[5] - shadowfile_out  - UNUSED
     files[7] - shadowfile_in   - UNUSED
     files[6] - logfile         - UNUSED
     files[8] - apcorrfile      - UNUSED
  */
  
  /** legacy - delete? */
  n0left = 0;
  n0right = 0;
  nthpix = 0;

  TestConfig (config, "FWHM",                   "%f", 0,    &fwhm);               /* Approx FWHM of objects (pixels) along major axis. */
  TestConfig (config, "AXIS_RATIO",             "%f", 0,    &ar);                 /* For star objects.  AR=b/a; b=minor axis. */
  TestConfig (config, "TILT",                   "%f", 0,    &tilt);               /* Angle of major axis in degrees; +x=0; +y=90. */
  TestConfig (config, "SKY",                    "%f", 0,    &skyguess);           /* Approximate mean sky value in data numbers. */

  /* convert to internal values */
  fwhm *= 1.2; /* is this really necessary? */
  tilt = tilt/57.29578;
  gmajwid = SQ(fwhm/2.3548);
  gxwid = gmajwid*(SQ(cos(tilt)) + SQ(ar*sin(tilt)));
  gywid = gmajwid*(SQ(ar*cos(tilt)) + SQ(sin(tilt)));
  fwhmx = 2.3548*sqrt(gxwid);
  fwhmy = 2.3548*sqrt(gywid);
  mprint (1, "fwhm x & y: %f, %f\n", fwhmx, fwhmy);

  /* no longer use ava[0-3] */
  ava[4] = gxwid;
  ava[6] = gywid;
  ava[5] = 0.01/sqrt(gxwid*gywid);
  
  TestConfig (config, "NFITBOX_X",              "%d", 0,    &irect[1]);           /* Size of fit box in the x-direction. */
  TestConfig (config, "NFITBOX_Y",              "%d", 0,    &irect[2]);           /* Size of fit box in the y-direction. */
  TestConfig (config, "MASKBOX_X",              "%d", 0,    &ixby2);              /* Size of mask box size in x. */
  TestConfig (config, "MASKBOX_Y",              "%d", 0,    &iyby2);              /* Size of mask box size in y. */
  TestConfig (config, "APBOX_X",                "%f", 0,    &arect[1]);           /* Size of aperture photometry box in x. */
  TestConfig (config, "APBOX_Y",                "%f", 0,    &arect[2]);           /* Size of aperture photometry box in y. */
  TestConfig (config, "NGALBOX_X",              "%d", 0,    &grect[1]);           /* Size of fit box in the x-direction. */
  TestConfig (config, "NGALBOX_Y",              "%d", 0,    &grect[2]);           /* Size of fit box in the y-direction. */

  TestConfig (config, "IBOTTOM",                "%d", 0,    &ibot);               /* Lowest allowed data value in data numbers. */
  TestConfig (config, "ITOP",                   "%d", 0,    &itop);               /* Level where Saturation begins. */
  TestConfig (config, "THRESHMIN",              "%f", 0,    &tmin);               /* Sigmas above sky for min threshold */
  TestConfig (config, "THRESHMAX",              "%f", 0,    &tmax);               /* Value of maximum threshold. */
  TestConfig (config, "THRESHDEC",              "%f", 0,    &tfac);               /* Threshold decrement in powers-of-2. */
  TestConfig (config, "EPERDN",                 "%f", 0,    &eperdn);             /* Electrons per data number. */
  TestConfig (config, "RDNOISE",                "%f", 0,    &rnoise);             /* Readout noise in electrons. */
  rnoise = SQ (rnoise);  /* we will store SQ(rnoise) since this is always needed */

  /* I don't like AUTOSCALE because it is not dynamic - 
     it sets the values once here for the run, so FWHM better be right */
  TestConfig (config, "AUTOSCALE",              "%s", 0,    line);                /* Auto-scaling of sizes by FWHM. */
  if (!strncasecmp (line, "y", 1)) {
    ScanConfig (config, "SCALEFITBOX",          "%f", 0,    &scalefb);          /* Size of fit box in units of FWHM. */
    ScanConfig (config, "FITBOXMIN",            "%f", 0,    &fbmin);            /* Smallest allowed fit box size. */
    ScanConfig (config, "SCALEAPBOX",           "%f", 0,    &scaleab);          /* Size of aperture phot box in units of FWHM. */
    ScanConfig (config, "APBOXMIN",             "%f", 0,    &abmin);            /* Smallest allowed aperture phot box size. */
    ScanConfig (config, "SCALEMASKBOX",         "%f", 0,    &scalemb);          /* Size of mask box in units of FWHM. */
    ScanConfig (config, "AMASKBOXMIN",          "%f", 0,    &ambmin);           /* Smallest allowed mask box size. */
    irect[1] = MAX (fwhmx*scalefb, fbmin);
    irect[2] = MAX (fwhmy*scalefb, fbmin);
    arect[1] = MAX (fwhmx*scaleab, abmin);
    arect[2] = MAX (fwhmy*scaleab, abmin);
    ixby2 = MAX(fwhmx*scalemb, ambmin);
    ixby2 = MAX(fwhmy*scalemb, ambmin);
  }
  /* force boxes to have odd sizes */
  if (((int)arect[1]) % 2 == 0) arect[1]++;
  if (((int)arect[2]) % 2 == 0) arect[2]++;
  if (irect[1] % 2 == 0) irect[1]++;
  if (irect[2] % 2 == 0) irect[2]++;
  if (ixby2 % 2 == 0) ixby2 ++;
  if (iyby2 % 2 == 0) iyby2 ++;
  ixby2 = (ixby2 - 1)/2;
  iyby2 = (iyby2 - 1)/2;

  fixpos = FALSE;
  TestConfig (config, "FIXPOS", "%s", 0, line);                 /* Fix star positions? */
  if (!strncasecmp (line, "y", 1)) fixpos = TRUE;

  if (ScanConfig (config, "IMAGE_OUT", "%s", 0, files[2])) flags[5] = TRUE;    /* Output image name. */

  TestConfig (config, "OBJTYPE_OUT", "%s", 0, &line);                /* Output format: (COMPLETE, INCOMPLETE, INTERNAL) */
  if (!strcasecmp (line, "complete"))   flags[3] = COMPLETE;
  if (!strcasecmp (line, "incomplete")) flags[3] = INCOMPLETE;
  if (!strcasecmp (line, "internal"))   flags[3] = INTERNAL;
  if (!strcasecmp (line, "oldstyle"))   flags[3] = OLDSTYLE;
  if (flags[3] == NONE3) {
    fprintf (stderr, "invalid OBJTYPE_OUT: %s\n", line);
    exit (1);
  }
  
  TestConfig (config, "LOGVERBOSITY",           "%d", 0,    &level);               /* Verbosity of log file; (0-4). */
  set_verbosity (level);
  TestConfig (config, "RESIDNOISE",             "%f", 0,    &fac);                 /* Fraction of noise to ADD to noise file. */
  TestConfig (config, "FOOTPRINT_NOISE",        "%f", 0,    &xpnd);                /* Expand stars in noise file by this amount. */
  TestConfig (config, "NPHSUB",                 "%f", 0,    &nphsub);              /* Limiting surface brightness for subtractions. */
  TestConfig (config, "NPHOB",                  "%f", 0,    &nphob);               /* Limiting surface brightness for obliterations. */
  TestConfig (config, "ICRIT",                  "%d", 0,    &icrit);               /* Obliterate if # of pixels > ITOP exceeds this. */
  TestConfig (config, "CENTINTMAX",             "%f", 0,    &cmax);                /* Obliterate if central intensity exceeds this. */
  TestConfig (config, "CTPERSAT",               "%f", 0,    &ctpersat);            /* Assumed intensity for saturated pixels. */

  TestConfig (config, "STARGALKNOB",            "%f", 0,    &stograt);             /* Star/galaxy discriminator: bigger number, more stars */
  TestConfig (config, "STARCOSKNOB",            "%f", 0,    &discrim);             /* Object/cosmic-ray discriminator: bigger number, more cosmics */
  TestConfig (config, "SNLIM7",                 "%f", 0,    &crit7);               /* Minimum S/N for 7-parameter fit. */
  crit7 = SQ(crit7);
  TestConfig (config, "SNLIM",                  "%f", 0,    &snlim);               /* Minimum S/N for a pixel to be in fit subraster. */
  TestConfig (config, "SNLIMMASK",              "%f", 0,    &bumpcrit);            /* Minimum S/N through mask to identify an object. */
  TestConfig (config, "SNLIMCOS",               "%f", 0,    &sn2cos);              /* Minimum S/N to be called a cosmic ray. */
  sn2cos = SQ(sn2cos);
  TestConfig (config, "NBADLEFT",               "%d", 0,    &nbadleft);            /* Ignore pixels closer to the left edge than this. */
  TestConfig (config, "NBADRIGHT",              "%d", 0,    &nbadright);           /* Ignore pixels closer to the right edge than this. */
  TestConfig (config, "NBADTOP",                "%d", 0,    &nbadtop);             /* Ignore pixels closer to the top edge than this. */
  TestConfig (config, "NBADBOT",                "%d", 0,    &nbadbot);             /* Ignore pixels closer to the bottom edge than this. */

  TestConfig (config, "SKYTYPE",                "%s", 0,    line);                 /* SKY type: (PLANE, HUBBLE, MEDIAN) */
  if (!strcasecmp (line, "plane")) flags[2] =  PLANE;
  /* if (!strcasecmp (line, "hubble")) flags[2] = HUBBLE; */
  /* if (!strcasecmp (line, "median")) flags[2] = MEDIAN; */
  if (flags[2] == NONE2) {
    fprintf (stderr, "invalid SKYTYPE: %s\n", line);
    exit (1);
  }

  TestConfig (config, "NFITITER",               "%d", 0,    &nit);                 /* Maximum number of iterations. */
  TestConfig (config, "NFITBOXFIRST_X",         "%d", 0,    &krect[1]);            /* Size of fit box in x for first pass. */
  TestConfig (config, "NFITBOXFIRST_Y",         "%d", 0,    &krect[2]);            /* Size of fit box in y for first pass. */
  TestConfig (config, "CHI2MINBIG",             "%f", 0,    &chicrit);             /* Critical CHI-squared for a large object. */
  TestConfig (config, "XTRA",                   "%f", 0,    &xtra);                /* We need more S/N if some pixels are missing. */
  TestConfig (config, "SIGMA1",                 "%f", 0,    &sig[1]);              /* Max. frac. scatter in sigma_x for stars. */
  TestConfig (config, "SIGMA2",                 "%f", 0,    &sig[2]);              /* Max. scatter in xy cross term for stars. */
  TestConfig (config, "SIGMA3",                 "%f", 0,    &sig[3]);              /* Max. frac. scatter in sigma_y for stars. */
  TestConfig (config, "ENUFF4",                 "%f", 0,    &enuff4);              /* Fraction of pixels needed for 4-param fit. */
  TestConfig (config, "ENUFF7",                 "%f", 0,    &enuff7);              /* Fraction of pixels needed for 7-param fit. */
  TestConfig (config, "COSOBLSIZE",             "%f", 0,    &widobl);              /* Size of obliteration box for a cosmic ray. */
  TestConfig (config, "APMAG_MAXERR",           "%f", 0,    &apmagmaxerr);         /* Max anticipated error for aperture phot report. */
  TestConfig (config, "PIXTHRESH",              "%f", 0,    &pixthresh);           /* Trigger on pixels higher than noise*PIXTHRESH. */
  TestConfig (config, "BETA4",                  "%f", 0,    &beta4);               /* R**4 coefficient modifier. */
  TestConfig (config, "BETA6",                  "%f", 0,    &beta64);              /* R**6 coefficient modifier. */
  beta4 = 1.0;
  beta64 = 1.0;

  TestConfig (config, "RELACC1",                "%f", 0,    &acc[0]);              /* Convergence criterion for sky. */
  TestConfig (config, "RELACC2",                "%f", 0,    &acc[1]);              /* Convergence criterion for for central intensity. */
  TestConfig (config, "RELACC3",                "%f", 0,    &acc[2]);              /* Convergence criterion for x-position. */
  TestConfig (config, "RELACC4",                "%f", 0,    &acc[3]);              /* Convergence criterion for y-position. */
  TestConfig (config, "RELACC5",                "%f", 0,    &acc[4]);              /* Convergence criterion for sigma-x. */
  TestConfig (config, "RELACC6",                "%f", 0,    &acc[5]);              /* Convergence criterion for sigma-xy. */
  TestConfig (config, "RELACC7",                "%f", 0,    &acc[6]);              /* Convergence criterion for sigma-y. */
  TestConfig (config, "PARLIM1",                "%f", 0,    &parlim[0]);           /* Allowed change for sky value. */
  TestConfig (config, "PARLIM2",                "%f", 0,    &parlim[1]);           /* Allowed change for central intensity. */
  TestConfig (config, "PARLIM3",                "%f", 0,    &parlim[2]);           /* Allowed change for x-position. */
  TestConfig (config, "PARLIM4",                "%f", 0,    &parlim[3]);           /* Allowed change for y-position. */
  TestConfig (config, "PARLIM5",                "%f", 0,    &parlim[4]);           /* Allowed change for sigma-x. */
  TestConfig (config, "PARLIM6",                "%f", 0,    &parlim[5]);           /* Allowed change for sigma-xy. */
  TestConfig (config, "PARLIM7",                "%f", 0,    &parlim[6]);           /* Allowed change for sigma-y. */

  /* other initial values for parameters */
  needit = TRUE;
  fixxy = FALSE;
  test7 = FALSE;
  ufactor = 100;
  chipar = 0.9;

  free (config);
  /* free (file); */

  return 1;
}


# if (0) /* things used by MEDIAN sky, disabled */
  /* only if 'median' */
  ScanConfig (config, "JHXWID",                 "%f", 0,    &jhxwid);                 /* X Half-size of median box (.le. 0 -> autoscale) */
  ScanConfig (config, "JHYWID",                 "%f", 0,    &jhywid);                 /* Y (same as above) */
  ScanConfig (config, "MPREC",                  "%f", 0,    &mprec);                 /* Median precision in DN (use .le. 0 for autocalc) */
  ScanConfig (config, "NTHPIX",                 "%f", 0,    &nthpix);                 /* Frequency of sky updates in pixels for 1st pass */

  /* log is now going to stderr, up to user to redirect */
  ScanConfig (config, "LOGFILE",                "%s", 0,    &A);                 /* Log file name.  TERM for screen. */
  ScanConfig (config, "IMAGE_IN",               "%s", 0,    &A);                 /* Input image name.  */
  ScanConfig (config, "OBJECTS_OUT",            "%s", 0,    &A);                 /* Output object list file name. */
  ScanConfig (config, "PARAMS_DEFAULT",         "%s", 0,    &A);                 /* Default parameters file name. */
  ScanConfig (config, "PARAMS_OUT",             "%s", 0,    &A);                 /* Output parameters file name. */
  ScanConfig (config, "SHADOWFILE_OUT",         "%s", 0,    &A);                 /* Output shadow file name. */
  ScanConfig (config, "SHADOWFILE_IN",          "%s", 0,    &A);                 /* Input shadow file name. */
  
  ScanConfig (config, "ABSLIM1",                "%f", 0,    &A);                 /* Allowed range for sky value. */
  ScanConfig (config, "ABSLIM2",                "%f", 0,    &A);                 /* Allowed range for central intensity. */
  ScanConfig (config, "ABSLIM3",                "%f", 0,    &A);                 /* Allowed range for x-position. */
  ScanConfig (config, "ABSLIM4",                "%f", 0,    &A);                 /* Allowed range for y-position. */
  ScanConfig (config, "ABSLIM5",                "%f", 0,    &A);                 /* Allowed range for sigma-x. */
  ScanConfig (config, "ABSLIM6",                "%f", 0,    &A);                 /* Allowed range for sigma-xy. */
  ScanConfig (config, "ABSLIM7",                "%f", 0,    &A);                 /* Allowed range for sigma-y. */
  ScanConfig (config, "ABSLIM8",                "%f", 0,    &A);                 /* Allowed range for sigma-y. */
  
  ScanConfig (config, "NPARAM",                 "%f", 0,    &A);                 /* Maximum number of PSF fit parameters. */
  ScanConfig (config, "NFITMAG",                "%f", 0,    &A);                 /* No. of PSF parameters to get magnitudes. */
  ScanConfig (config, "NFITSHAPE",              "%f", 0,    &A);                 /* No. of PSF parameters to get shape and mags. */
  
  ScanConfig (config, "MAXSTARS",               "%f", 0,    &A);                 /* Ignore pixels closer to the bottom edge than this. */
  ScanConfig (config, "PSFTYPE",                "%s", 0,    &A);                 /* PSF type: (PGAUSS) */
# endif

# if (0) /* autothresh eliminated: we are using threshmin = Nsigma above sky always */
  Autothresh = FALSE;
  ScanConfig (config, "AUTOTHRESH",             "%s", 0,    line);                 /* Auto-scaling of thresholds. */
  if (!strncasecmp (line, "y")) Autothresh = TRUE;
  /* only if 'autothresh' */
  ScanConfig (config, "SIGMAIBOTTOM",           "%f", 0,    &sigbot);                 /* Level of IBOTTOM below sky in units of noise. */
  ScanConfig (config, "SIGMATHRESHMIN",         "%f", 0,    &sigthresh);                 /* Level of THRESHMIN above sky in units of noise. */
# endif

# if (0) /* warmstart code */  
  status = ScanConfig (config, "OBJECTS_IN",             "%s", 0,    &A);                 /* Input object list file name. */
  if (status) {
    flags[6] = TRUE;
    ScanConfig (config, "OBJTYPE_IN",             "%s", 0,    &A);                 /* Input format: (COMPLETE, INTERNAL) */
  }
# endif

# if (0) /* aperture correction file - there are several other parameters, see tuneup.f */ 
    ScanConfig (config, "APCORRFILE",             "%s", 0,    &A);                 /* Aperture correction file name. */
# endif
