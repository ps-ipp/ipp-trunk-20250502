# include "dvoshell.h"

int imextract (int argc, char **argv) {
  
  off_t i, j, Nimage;
  int n, N, Npts, NPTS, last, next, state, Nfields, Nreturn, Nstack;
  char **cstack, name[1024];

  Vector **vec;
  Image *image;
  dbStack *stack;
  dbField *fields;
  dbValue *values;
  SkyRegionSelection *selection;

  /* defaults */
  vec = NULL;
  image = NULL;
  stack = NULL;
  fields = NULL;
  values = NULL;
  selection = NULL;

  if ((N = get_argument (argc, argv, "-h"))) goto help;
  if ((N = get_argument (argc, argv, "--help"))) goto help;

  // int VERBOSE = FALSE;
  // if ((N = get_argument (argc, argv, "-v"))) {
  //   remove_argument (N, &argc, argv);
  //   VERBOSE = TRUE;
  // }

  if (!InitPhotcodes ()) goto escape;

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    goto escape;
  }

  // command-line is of the form: imextract field,field, field [where (field op value)...]

  // parse the fields to be extracted and returned
  fields = dbCmdlineFields (argc, argv, DVO_TABLE_IMAGE, &last, &Nfields);
  if (fields == NULL) goto escape;
  if (Nfields == 0) {
    FreeSkyRegionSelection (selection);
    dbFreeFields (fields, Nfields);
    goto help;
  }

  // examine line for 'where' or 'match to'.  'match to' is forbidden
  state = dbCmdlineConditions (argc, argv, last, &next);
  if (state == DVO_DB_CMDLINE_ERROR) goto escape;
  if (state == DVO_DB_CMDLINE_IS_MATCH) goto escape; // not allowed for mextract

  // parse the remainder of the line as a boolean math expression
  unsigned int Ncstack;
  cstack = isolate_elements (argc-next, &argv[next], &Ncstack);
  
  // construct the db Boolean math stack (frees cstack)
  stack = dbRPN (Ncstack, cstack, &Nstack);
  if (Ncstack && !Nstack) {
    print_error(); 
    goto escape; 
  }

  // add the skyregion limits to the where statement (or create)
  // XXX we may want to drop this and use just the image_subset function
  dbAstroRegionLimits (&stack, &Nstack, selection, DVO_TABLE_IMAGE);

  // parse stack elements into fields and scalars as needed
  Nreturn = Nfields; 
  if (!dbCheckStack (stack, Nstack, DVO_TABLE_IMAGE, &fields, &Nfields)) goto escape;

  /* create output storage vectors */
  Npts = 0;
  NPTS = 100;
  ALLOCATE (values, dbValue, Nfields);
  ALLOCATE (vec, Vector *, Nreturn);
  for (i = 0; i < Nreturn; i++) {
    if (ISNUM(fields[i].name[0])) {
      sprintf (name, "v_%s", fields[i].name);
    } else {
      sprintf (name, "%s", fields[i].name);
    }
    if ((vec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) goto escape;
    ResetVector (vec[i], fields[i].type, NPTS);
  }

  if ((image = LoadImagesDVO (&Nimage)) == NULL) goto escape;
  // BuildChipMatch (image, Nimage);
  dbExtractImagesInit (); 

  // XXX do I need to use this, or the region portion
  // image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);

  // grab data from all selected sky regions
  struct sigaction *old_sigaction = SetInterrupt();
  for (j = 0; (j < Nimage) && !interrupt; j++) {

    // reset counters for saved fields, extract fields
    dbExtractImagesReset (); 
    for (n = 0; n < Nfields; n++) {
      values[n] = dbExtractImages (image, Nimage, j, &fields[n]);
    }

    // test the conditional statement
    if (!dbBooleanCond (stack, Nstack, values)) continue;
    for (n = 0; n < Nreturn; n++) {
      if (vec[n][0].type == OPIHI_FLT) {
	vec[n][0].elements.Flt[Npts] = values[n].Flt;
      } else {
	vec[n][0].elements.Int[Npts] = values[n].Int;
      }
    }
    Npts++;
    if (Npts >= NPTS) {
      NPTS += 2000;
      for (n = 0; n < Nreturn; n++) {
	if (vec[n][0].type == OPIHI_FLT) {
	  REALLOCATE (vec[n][0].elements.Flt, opihi_flt, NPTS);
	} else {
	  REALLOCATE (vec[n][0].elements.Int, opihi_int, NPTS);
	}
      }
    }
  }
  ClearInterrupt (old_sigaction);

  for (n = 0; n < Nreturn; n++) {
    vec[n][0].Nelements = Npts;
    if (vec[n][0].type == OPIHI_FLT) {
      REALLOCATE (vec[n][0].elements.Flt, opihi_flt, MAX(1,Npts));
    } else {
      REALLOCATE (vec[n][0].elements.Int, opihi_int, MAX(1,Npts));
    }
  }

  // free (subset);
  FreeImagesDVO (image);

  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  if (stack) free (stack);
  return (TRUE);
  
 escape:
  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  if (stack) free (stack);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: imextract field[,field,field...] where (expression)\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, " USAGE: imextract field[,field,field...] where (expression)\n");
    gprint (GP_ERR, "  RA : right ascension of field center (J2000)\n");
    gprint (GP_ERR, "  DEC : declination of field center\n");
    gprint (GP_ERR, "  GLON : galactic longitude of field center (J2000)\n");
    gprint (GP_ERR, "  GLAT : galactic latitude of field center (J2000)\n");
    gprint (GP_ERR, "  ELON : ecliptic longitude of field center (J2000)\n");
    gprint (GP_ERR, "  ELAT : ecliptic latitude of field center (J2000)\n");

    gprint (GP_ERR, "  theta : position angle of image\n");
    gprint (GP_ERR, "  skew : distortion from rectangle\n");
    gprint (GP_ERR, "  scale : pixel scale\n");
    gprint (GP_ERR, "  dscale : pixel-scale error (or variation?)\n");

    gprint (GP_ERR, "  time : time of exposure\n");
    gprint (GP_ERR, "  nstar : number of stars detected in exposure\n");
    gprint (GP_ERR, "  airmass : mean airmass of exposure\n");
    gprint (GP_ERR, "  NX : image dimensions\n");
    gprint (GP_ERR, "  NY : image dimensions\n");

    gprint (GP_ERR, "  apresid : aperture - fit magnitude\n");
    gprint (GP_ERR, "  dapresid : aperture - fit magnitude scatter\n");

    gprint (GP_ERR, "  Mcal : photometry calibration (mags)\n");
    gprint (GP_ERR, "  dMcal : photometry calibration error (mags)\n");
    gprint (GP_ERR, "  Mchisq : chisq of photometry calibration\n");
    gprint (GP_ERR, "  photcode : numeric photcode value for image\n");
    gprint (GP_ERR, "  exptime : exposure duration (seconds)\n");
    gprint (GP_ERR, "  sidtime : sidereal time of exposure\n");

    gprint (GP_ERR, "  latitude : observatory latitude\n");

    gprint (GP_ERR, "  detlimit : detection limit of exposure\n");
    gprint (GP_ERR, "  satlimit : saturation limit of exposure\n");
    gprint (GP_ERR, "  cerror : astrometric scatter\n");

    gprint (GP_ERR, "  -- Note: the follow FWHM are from the PSF model --\n");
    gprint (GP_ERR, "  FWHM : mean fwhm of chip\n");
    gprint (GP_ERR, "  FWHM_MAJ : fwhm of chip (major axis)\n");
    gprint (GP_ERR, "  FWHM_MIN : fwhm of chip (minor axis)\n");
    gprint (GP_ERR, "  FWHM_MAJOR : fwhm of chip (major axis)\n");
    gprint (GP_ERR, "  FWHM_MININ : fwhm of chip (minor axis)\n");

    gprint (GP_ERR, "  FWHM_MEDIAN : median fwhm of exposure\n");
    gprint (GP_ERR, "  FWHM_MAJ_MEDIAN : median fwhm of major axis\n");
    gprint (GP_ERR, "  FWHM_MIN_MEDIAN : median fwhm of minor axis\n");
    gprint (GP_ERR, "  FWHM_MAJOR_MEDIAN : median fwhm of major axis\n");
    gprint (GP_ERR, "  FWHM_MININ_MEDIAN : median fwhm of minor axis\n");

    gprint (GP_ERR, "  trate : tracking rate for TDI images\n");

    gprint (GP_ERR, "  ncal : number of stars used for photometry calibration\n");
    gprint (GP_ERR, "  sky : mean background flux\n");

    gprint (GP_ERR, "  imflags : processing bit flags\n");
    gprint (GP_ERR, "  flags : processing bit flags\n");
    gprint (GP_ERR, "  ccdnum : identifier for CCD\n");

    gprint (GP_ERR, "  imageID  : unique image identifier\n");
    gprint (GP_ERR, "  externID : external image identifier\n");
    gprint (GP_ERR, "  sourceID : external db reference\n");

    gprint (GP_ERR, "  X_LL_CHIP : chip x-pixel coordinate of lower left corner\n");
    gprint (GP_ERR, "  X_LR_CHIP : chip x-pixel coordinate of lower right corner\n");
    gprint (GP_ERR, "  X_UL_CHIP : chip x-pixel coordinate of upper left corner\n");
    gprint (GP_ERR, "  X_UR_CHIP : chip x-pixel coordinate of upper right corner\n");
    gprint (GP_ERR, "  Y_LL_CHIP : chip y-pixel coordinate of lower left corner\n");
    gprint (GP_ERR, "  Y_LR_CHIP : chip y-pixel coordinate of lower right corner\n");
    gprint (GP_ERR, "  Y_UL_CHIP : chip y-pixel coordinate of upper left corner\n");
    gprint (GP_ERR, "  Y_UR_CHIP : chip y-pixel coordinate of upper right corner\n");
    gprint (GP_ERR, "  X_LL_FP   : focal-plane x-pixel coordinate of lower left corner\n");
    gprint (GP_ERR, "  X_LR_FP   : focal-plane x-pixel coordinate of lower right corner\n");
    gprint (GP_ERR, "  X_UL_FP   : focal-plane x-pixel coordinate of upper left corner\n");
    gprint (GP_ERR, "  X_UR_FP   : focal-plane x-pixel coordinate of upper right corner\n");
    gprint (GP_ERR, "  Y_LL_FP   : focal-plane y-pixel coordinate of lower left corner\n");
    gprint (GP_ERR, "  Y_LR_FP   : focal-plane y-pixel coordinate of lower right corner\n");
    gprint (GP_ERR, "  Y_UL_FP   : focal-plane y-pixel coordinate of upper left corner\n");
    gprint (GP_ERR, "  Y_UR_FP   : focal-plane y-pixel coordinate of upper right corner\n");
    return (FALSE);
  }
  gprint (GP_ERR, " imextract --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
  
