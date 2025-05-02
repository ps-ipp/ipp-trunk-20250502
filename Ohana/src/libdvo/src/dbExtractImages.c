# include "dvo.h"

/* time concepts */
static time_t TimeReference;
static int TimeFormat;

static CoordTransform *celestial_to_galactic = NULL;
static CoordTransform *celestial_to_ecliptic = NULL;

static int haveGalactic = FALSE;
static double GLON = 0.0;
static double GLAT = 0.0;

static int haveEcliptic = FALSE;
static double ELON = 0.0;
static double ELAT = 0.0;

static int haveCelestial = FALSE;
static double RAo = 0.0;
static double DECo = 0.0;

// define a locally-static transform
int dbExtractImagesInitTransform (CoordTransformSystem target) {

  // galactic transform is kept forever
  if (target == COORD_GALACTIC) {
    if (celestial_to_galactic != NULL) return (TRUE);
    celestial_to_galactic = InitTransform (COORD_CELESTIAL, target);
    return (TRUE);
  }

  // ecliptic transform must be updated (is weakly time-dependent)
  if (target == COORD_ECLIPTIC) {
    if (celestial_to_ecliptic != NULL) {
      free (celestial_to_ecliptic);
    }
    celestial_to_ecliptic = InitTransform (COORD_CELESTIAL, target);
    return (TRUE);
  }
  return (FALSE);
}

int dbExtractImagesInit () {
  GetTimeFormat (&TimeReference, &TimeFormat);
  return (TRUE);
}

int dbExtractImagesReset () {
  haveGalactic  = FALSE;
  haveEcliptic  = FALSE;
  haveCelestial = FALSE;
  return (TRUE);
}

/* return image.field based on the selection */
dbValue dbExtractImages (Image *image, off_t Nimage, off_t N, dbField *field) {
  OHANA_UNUSED_PARAM(Nimage); // XXX: not sure why this is supplied

  double x, y;
  time_t t;
  dbValue value;

  value.Flt = NAN;
  value.Int =   0;

  /* assign vector values */
  switch (field->ID) {
    case IMAGE_RA:
      if (!haveCelestial) {
	if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	  x = 0.0;
	  y = 0.0;
	} else {
	  x = 0.5*image[N].NX;
	  y = 0.5*image[N].NY;
	}
	XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	haveCelestial = TRUE;
      }
      value.Flt = RAo;
      break;
    case IMAGE_DEC:
      if (!haveCelestial) {
	if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	  x = 0.0;
	  y = 0.0;
	} else {
	  x = 0.5*image[N].NX;
	  y = 0.5*image[N].NY;
	}
	XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	haveCelestial = TRUE;
      }
      value.Flt = DECo;
      break;
    case IMAGE_GLON:
      if (!haveGalactic) {
	if (!haveCelestial) {
	  if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	    x = 0.0;
	    y = 0.0;
	  } else {
	    x = 0.5*image[N].NX;
	    y = 0.5*image[N].NY;
	  }
	  XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	  haveCelestial = TRUE;
	}
	ApplyTransform (&GLON, &GLAT, RAo, DECo, celestial_to_galactic);
	haveGalactic = TRUE;
      }
      value.Flt = GLON;
      break;
    case IMAGE_GLAT:
      if (!haveGalactic) {
	if (!haveCelestial) {
	  if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	    x = 0.0;
	    y = 0.0;
	  } else {
	    x = 0.5*image[N].NX;
	    y = 0.5*image[N].NY;
	  }
	  XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	  haveCelestial = TRUE;
	}
	ApplyTransform (&GLON, &GLAT, RAo, DECo, celestial_to_galactic);
	haveGalactic = TRUE;
      }
      value.Flt = GLAT;
      break;
    case IMAGE_ELON:
      if (!haveEcliptic) {
	if (!haveCelestial) {
	  if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	    x = 0.0;
	    y = 0.0;
	  } else {
	    x = 0.5*image[N].NX;
	    y = 0.5*image[N].NY;
	  }
	  XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	  haveCelestial = TRUE;
	}
	ApplyTransform (&ELON, &ELAT, RAo, DECo, celestial_to_ecliptic);
	haveEcliptic = TRUE;
      }
      value.Flt = ELON;
      break;
    case IMAGE_ELAT:
      if (!haveEcliptic) {
	if (!haveCelestial) {
	  if (!strcmp(&image[N].coords.ctype[4], "-DIS")) {
	    x = 0.0;
	    y = 0.0;
	  } else {
	    x = 0.5*image[N].NX;
	    y = 0.5*image[N].NY;
	  }
	  XY_to_RD (&RAo, &DECo, x, y, &image[N].coords);
	  haveCelestial = TRUE;
	}
	ApplyTransform (&ELON, &ELAT, RAo, DECo, celestial_to_ecliptic);
	haveEcliptic = TRUE;
      }
      value.Flt = ELAT;
      break;

    case IMAGE_THETA: {
      double theta1, theta2, s1, s2;
      s1 = SIGN(image[N].coords.pc1_1);
      s2 = SIGN(image[N].coords.pc2_2);
      theta1 = DEG_RAD*atan2 (+s1*image[N].coords.pc1_2, s1*image[N].coords.pc1_1);
      theta2 = DEG_RAD*atan2 (-s2*image[N].coords.pc2_1, s2*image[N].coords.pc2_2);
      value.Flt = 0.5*(theta1+theta2);
      break; }
    case IMAGE_SKEW: {
      double theta1, theta2, s1, s2;
      s1 = SIGN(image[N].coords.pc1_1);
      s2 = SIGN(image[N].coords.pc2_2);
      theta1 = DEG_RAD*atan2 (+s1*image[N].coords.pc1_2, s1*image[N].coords.pc1_1);
      theta2 = DEG_RAD*atan2 (-s2*image[N].coords.pc2_1, s2*image[N].coords.pc2_2);
      value.Flt = (theta1-theta2);
      break; }
    case IMAGE_SCALE: {
      double scale1, scale2;
      scale1 = fabs(image[N].coords.cdelt1);
      scale2 = fabs(image[N].coords.cdelt2);
      value.Flt = 0.5*(scale1+scale2);
      break; }
    case IMAGE_DSCALE: {
      double scale1, scale2;
      scale1 = fabs(image[N].coords.cdelt1);
      scale2 = fabs(image[N].coords.cdelt2);
      value.Flt = (scale1-scale2);
      break; }

    case IMAGE_TIME:
      t = image[N].tzero + 0.5*image[N].NY * image[N].trate / 10000;
      value.Flt = TimeValue (t, TimeReference, TimeFormat);
      break;
    case IMAGE_NSTAR:
      value.Int = image[N].nstar;
      break;
    case IMAGE_AIRMASS:
      value.Flt = image[N].secz;
      break;
    case IMAGE_NX_PIX:
      value.Int = image[N].NX;
      break;
    case IMAGE_NY_PIX:
      value.Int = image[N].NY;
      break;
    case IMAGE_APRESID:
      value.Flt = image[N].apmifit;
      break;
    case IMAGE_DAPRESID:
      value.Flt = image[N].dapmifit;
      break;

    case IMAGE_MCAL_PSF:
      value.Flt = image[N].McalPSF;
      break;
    case IMAGE_MCAL_APER:
      value.Flt = image[N].McalAPER;
      break;
    case IMAGE_dMCAL:
      value.Flt = image[N].dMcal;
      break;
    case IMAGE_XM:
      value.Flt = image[N].McalChiSq;
      break;
    case IMAGE_PHOTCODE:
      value.Int = image[N].photcode;
      break;
    case IMAGE_EXPTIME:
      value.Flt = image[N].exptime;
      break;

    case IMAGE_EXPNAME_AS_INT:
      // XXX NOTE this does not handle gpc2 images
      if ((image[N].name[0] == 'o') && (image[N].name[5] == 'g') && (image[N].name[10] == 'o')) {
	int mjd  = atoi(&image[N].name[1]);
	int Nexp = atoi(&image[N].name[6]);
	value.Int = mjd * 10000 + Nexp;
      }
      break;

    case IMAGE_SIDTIME:
      value.Flt = image[N].sidtime;
      break;

    case IMAGE_LATITUDE:
      value.Flt = image[N].latitude;
      break;

    case IMAGE_DET_LIMIT:
      value.Flt = image[N].detection_limit * 0.1;
      break;
    case IMAGE_SAT_LIMIT:
      value.Flt = image[N].saturation_limit * 0.1;
      break;
    case IMAGE_CERROR:
      value.Flt = image[N].cerror / 50.0;
      break;

    case IMAGE_FWHM:
      value.Flt = (image[N].fwhm_x + image[N].fwhm_y) / 50.0;
      break;
    case IMAGE_FWHM_MAJ:
      value.Flt = image[N].fwhm_x / 25.0;
      break;
    case IMAGE_FWHM_MIN:
      value.Flt = image[N].fwhm_y / 25.0;
      break;

    case IMAGE_FWHM_MEDIAN:
      if (!image[N].parent) return value;
      value.Flt = (image[N].parent->fwhm_x + image[N].parent->fwhm_y) / 50.0;
      break;
    case IMAGE_FWHM_MAJ_MEDIAN:
      if (!image[N].parent) return value;
      value.Flt = image[N].parent->fwhm_x / 25.0;
      break;
    case IMAGE_FWHM_MIN_MEDIAN:
      if (!image[N].parent) return value;
      value.Flt = image[N].parent->fwhm_y / 25.0;
      break;

    case IMAGE_TRATE:
      value.Flt = image[N].trate / 10000.0;
      break;

    case IMAGE_NCAL:
      value.Int = image[N].nFitPhotom;
      break;
    case IMAGE_SKY:
      value.Flt = NAN;
      break;
    case IMAGE_FLAGS:
      value.Int = image[N].flags;
      break;
    case IMAGE_CCDNUM:
      value.Int = image[N].ccdnum;
      break;

    case IMAGE_IMAGE_ID:
      value.Int = image[N].imageID;
      break;
    case IMAGE_EXTERN_ID:
      value.Int = image[N].externID;
      break;
    case IMAGE_SOURCE_ID:
      value.Int = image[N].sourceID;
      break;

      // reference pixel extractions
    case IMAGE_X_LL_CHIP:
    case IMAGE_Y_LL_CHIP:
    case IMAGE_Y_LR_CHIP:
    case IMAGE_X_UL_CHIP:
      value.Flt = 0.0;
      break;
    case IMAGE_X_LR_CHIP:
    case IMAGE_X_UR_CHIP:
      value.Flt = image[N].NX;
      break;
    case IMAGE_Y_UL_CHIP:
    case IMAGE_Y_UR_CHIP:
      value.Flt = image[N].NX;
      break;

    case IMAGE_X_LL_FP:
    case IMAGE_Y_LL_FP:
      XY_to_LM (&x, &y, 0.0, 0.0, &image[N].coords);
      value.Flt = (field->ID == IMAGE_X_LL_FP) ? x : y;
      break;
    case IMAGE_X_LR_FP:
    case IMAGE_Y_LR_FP:
      XY_to_LM (&x, &y, image[N].NX, 0.0, &image[N].coords);
      value.Flt = (field->ID == IMAGE_X_LR_FP) ? x : y;
      break;
    case IMAGE_X_UL_FP:
    case IMAGE_Y_UL_FP:
      XY_to_LM (&x, &y, 0.0, image[N].NY, &image[N].coords);
      value.Flt = (field->ID == IMAGE_X_UL_FP) ? x : y;
      break;
    case IMAGE_X_UR_FP:
    case IMAGE_Y_UR_FP:
      XY_to_LM (&x, &y, image[N].NX, image[N].NY, &image[N].coords);
      value.Flt = (field->ID == IMAGE_X_UR_FP) ? x : y;
      break;

    case IMAGE_R_LL:
    case IMAGE_D_LL:
      XY_to_RD (&x, &y, 0.0, 0.0, &image[N].coords);
      value.Flt = (field->ID == IMAGE_R_LL) ? x : y;
      break;
    case IMAGE_R_LR:
    case IMAGE_D_LR:
      XY_to_RD (&x, &y, image[N].NX, 0.0, &image[N].coords);
      value.Flt = (field->ID == IMAGE_R_LR) ? x : y;
      break;
    case IMAGE_R_UL:
    case IMAGE_D_UL:
      XY_to_RD (&x, &y, 0.0, image[N].NY, &image[N].coords);
      value.Flt = (field->ID == IMAGE_R_UL) ? x : y;
      break;
    case IMAGE_R_UR:
    case IMAGE_D_UR:
      XY_to_RD (&x, &y, image[N].NX, image[N].NY, &image[N].coords);
      value.Flt = (field->ID == IMAGE_R_UR) ? x : y;
      break;

    case IMAGE_X_ERR_SYS:
      value.Flt = image[N].dXpixSys;
      break;
    case IMAGE_Y_ERR_SYS:
      value.Flt = image[N].dYpixSys;
      break;
    case IMAGE_MAG_ERR_SYS:
      value.Flt = image[N].dMagSys;
      break;

    case IMAGE_UBERCAL_DIST:
      value.Int = image[N].ubercalDist;
      break;

    case IMAGE_NFIT_PHOTOM:
      value.Int = image[N].nFitPhotom;
      break;
    case IMAGE_NFIT_ASTROM:
      value.Int = image[N].nFitAstrom;
      break;
    case IMAGE_NLINK_PHOTOM:
      value.Int = image[N].nLinkPhotom;
      break;
    case IMAGE_NLINK_ASTROM:
      value.Int = image[N].nLinkAstrom;
      break;

    case IMAGE_REF_COLOR_BLUE:
      value.Flt = image[N].refColorBlue;
      break;
    case IMAGE_REF_COLOR_RED:
      value.Flt = image[N].refColorRed;
      break;
  }
  return (value);
}  

