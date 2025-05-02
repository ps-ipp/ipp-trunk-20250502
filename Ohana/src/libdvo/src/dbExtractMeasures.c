# include "dvo.h"
# include "dvodb.h"

// to add a new field to the extractions:
// 1) add the selection for the field below
// 2) add the field to the ParseMeasureField in dbFields.c
// 3) add the field to the measure enum list in dvoshell.h

/* time concepts */
static time_t TimeReference;
static int TimeFormat;

static CoordTransform *celestial_to_galactic = NULL;
static CoordTransform *celestial_to_ecliptic = NULL;

static int REMOTE_CLIENT = FALSE;

// the following values are calculated together in a single function, eg.,
// ApplyTransform() returning Glon & Glat.  for a single measurement, we want to do this
// calculation once and save both values in case both are requested (usually both are if
// either is)
static int haveGalacticAve = FALSE;
static double GLON_AVE = 0.0;
static double GLAT_AVE = 0.0;

static int haveEclipticAve = FALSE;
static double ELON_AVE = 0.0;
static double ELAT_AVE = 0.0;

static int haveGalacticMeas = FALSE;
static double GLON_MEAS = 0.0;
static double GLAT_MEAS = 0.0;

static int haveEclipticMeas = FALSE;
static double ELON_MEAS = 0.0;
static double ELAT_MEAS = 0.0;

static int haveMosaicMeas = FALSE;
static double XMOS_MEAS = 0.0;
static double YMOS_MEAS = 0.0;

static int haveFieldMeas = FALSE;
static double XFIELD_MEAS = 0.0;
static double YFIELD_MEAS = 0.0;

int dbExtractMeasuresInit (int isRemoteClient) {
  REMOTE_CLIENT = isRemoteClient;
  GetTimeFormat (&TimeReference, &TimeFormat);
  return (TRUE);
}

// define a locally-static transform
int dbExtractMeasuresInitTransform (CoordTransformSystem target) {

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

int dbExtractMeasuresInitAve () {
  haveGalacticAve = FALSE;
  haveEclipticAve = FALSE;
  return (TRUE);
}

int dbExtractMeasuresInitMeas () {
  haveMosaicMeas   = FALSE;
  haveGalacticMeas = FALSE;
  haveEclipticMeas = FALSE;
  return (TRUE);
}

/* return measure.field based on the selection */
dbValue dbExtractMeasures (Average *average, SecFilt *secfilt, Measure *measure, Lensing *lensing, StarPar *starpar, dbField *field) {

  int Nsec;
  dbValue value;
  double dT;
  float dR, dD;

  Coords *mosaic, *fieldc;

  PhotCode *equiv = NULL;

  value.Flt = NAN;
  value.Int =   0;

  switch (field->ID) {
    case MEAS_PHOT: { /* magnitudes are already determined above */
      PhotCode *myEquiv = GetPhotcodeEquivbyCode (measure[0].photcode);

      // if we request mag:ave, use equiv for photcode (ie a given measure, say GPC1.g.XY01, will return g for mag:ave)
      if  (field->photcode->type == PHOT_MAG) {
	equiv = myEquiv;
	goto valid_photcode;
      }

      // if we ask for 2MASS_K, etc (REF values), return NAN unless measure->code matches
      if ((field->photcode->type == PHOT_REF) && (measure[0].photcode == field->photcode->code)) goto valid_photcode;

      // if we ask for GPC1.g.XY03:rel, etc (DEP values), return NAN unless measure->code matches
      if ((field->photcode->type == PHOT_DEP) && (measure[0].photcode == field->photcode->code)) goto valid_photcode;

      // if we ask for g:ave, or other SEC-level values, return the corresponding field 
      if (field->photcode->type == PHOT_SEC) {
	switch (field->magLevel) {
	  // measure-like : return non-NAN if measure.equiv.photcode matches field.photcode
	  case MAG_LEVEL_INST:
	  case MAG_LEVEL_CAT:
	  case MAG_LEVEL_SYS:
	  case MAG_LEVEL_REL:
	  case MAG_LEVEL_CAL:
	    equiv = myEquiv;
	    if (equiv && (equiv->code == field->photcode->code)) goto valid_photcode;
	    break;

	    // mean-like : return value for the given photcode
	  case MAG_LEVEL_AVE:
	  case MAG_LEVEL_REF:
	    equiv = field->photcode;
	    goto valid_photcode;
	    break;
	  default:
	    fprintf (stderr, "error");
	    return value;
	}
      }
      break;

  valid_photcode:
      switch (field->magOption) {
	case MAG_OPTION_MAG:
	  switch (field->magLevel) {
	    case MAG_LEVEL_INST:
	      value.Flt = PhotInst (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_CAT:
	      value.Flt = PhotCat  (measure, field->magClass); 
	      break;
	    case MAG_LEVEL_SYS:
	      value.Flt = PhotSys  (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_REL:
	      value.Flt = PhotRel  (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_CAL:
	      value.Flt = PhotCal  (measure, average, secfilt, measure, equiv, field->magClass); 
	      break;
	    case MAG_LEVEL_AVE:
	      value.Flt = PhotAve  (equiv, average, secfilt, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_REF:
	      value.Flt = PhotRef  (equiv, average, secfilt, measure, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_ERR:
	  switch (field->magLevel) {
	    case MAG_LEVEL_INST:
	    case MAG_LEVEL_CAT:
	    case MAG_LEVEL_SYS:
	    case MAG_LEVEL_REL:
	      value.Flt = PhotErr (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_CAL:
	      value.Flt = PhotCalErr (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_AVE:
	    case MAG_LEVEL_REF:
	      value.Flt = PhotAveErr (equiv, average, secfilt, field->magClass, field->magSource);  
	      break;
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_FLUX:
	  switch (field->magLevel) {
	    case MAG_LEVEL_INST:
	      value.Flt = PhotFluxInst (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_CAT:
	      value.Flt = PhotFluxCat  (measure, field->magClass); 
	      break;
	    case MAG_LEVEL_SYS:
	      value.Flt = PhotFluxSys  (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_REL:
	      value.Flt = PhotFluxRel  (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_CAL:
	      value.Flt = PhotFluxCal  (measure, average, secfilt, measure, equiv, field->magClass); 
	      break;
	    case MAG_LEVEL_AVE:
	      value.Flt = PhotFluxAve  (equiv, average, secfilt, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_REF:
	      value.Flt = PhotFluxRef  (equiv, average, secfilt, measure, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_FLUX_ERR:
	  switch (field->magLevel) {
	    case MAG_LEVEL_INST:
	      value.Flt = PhotFluxInstErr (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_CAT:
	      value.Flt = PhotFluxCatErr (measure, field->magClass);  
	      break;
	    case MAG_LEVEL_SYS:
	      value.Flt = PhotFluxSysErr (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_REL:
	      value.Flt = PhotFluxRelErr (measure, average, secfilt, field->magClass); 
	      break;
	    case MAG_LEVEL_CAL:
	      // XXX not defined
	      break;
	    case MAG_LEVEL_AVE:
	      value.Flt = PhotFluxAveErr (equiv, average, secfilt, field->magClass, field->magSource);  
	      break;
	    case MAG_LEVEL_REF:
	      // XXX not defined
	      break;
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	  // the following are all values which are come from the secfit table
	case MAG_OPTION_STDEV:      { value.Flt = PhotMstdev (equiv, average, secfilt, field->magClass, field->magSource); break; }
	case MAG_OPTION_CHISQ:      { value.Flt = PhotXm (equiv, average, secfilt); break; }
	case MAG_OPTION_MIN:        { value.Flt = PhotMmin (equiv, average, secfilt); break;}
	case MAG_OPTION_MAX:        { value.Flt = PhotMmax (equiv, average, secfilt); break;}
	case MAG_OPTION_NCODE:      { value.Int = PhotNcode (equiv, average, secfilt); break; }
	case MAG_OPTION_NPHOT:      { value.Int = PhotNphot (equiv, average, secfilt, field->magClass, field->magSource); break; }
	case MAG_OPTION_NWARP:      { value.Int = PhotNwarp (equiv, average, secfilt); break; }
	case MAG_OPTION_NWARP_GOOD: { value.Int = PhotNwarpGood (equiv, average, secfilt); break; }
	case MAG_OPTION_NSTACK:     { value.Int = PhotNstack (equiv, average, secfilt); break; }
	case MAG_OPTION_NSTACK_DET: { value.Int = PhotNstackDet (equiv, average, secfilt); break; }
	case MAG_OPTION_UC_DIST:    { value.Flt = PhotUCdist (equiv, average, secfilt); break; }
	case MAG_OPTION_FLAGS:      { value.Int = PhotSecfiltFlags (equiv, average, secfilt); break; }
	case MAG_OPTION_PSF_QF:         { value.Flt = PhotSecfiltPsfQf        (equiv, average, secfilt); break; }
	case MAG_OPTION_PSF_QF_PERFECT: { value.Flt = PhotSecfiltPsfQfPerfect (equiv, average, secfilt); break; }
	  
	  // the following values come from the mean lensobj table
	case MAG_OPTION_X11_SM_OBJ: 
	case MAG_OPTION_X12_SM_OBJ: 
	case MAG_OPTION_X22_SM_OBJ: 
	case MAG_OPTION_E1_SM_OBJ: 
	case MAG_OPTION_E2_SM_OBJ: 
	case MAG_OPTION_X11_SH_OBJ: 
	case MAG_OPTION_X12_SH_OBJ: 
	case MAG_OPTION_X22_SH_OBJ: 
	case MAG_OPTION_E1_SH_OBJ: 
	case MAG_OPTION_E2_SH_OBJ: 
	case MAG_OPTION_X11_SM_PSF: 
	case MAG_OPTION_X12_SM_PSF: 
	case MAG_OPTION_X22_SM_PSF: 
	case MAG_OPTION_E1_SM_PSF: 
	case MAG_OPTION_E2_SM_PSF: 
	case MAG_OPTION_X11_SH_PSF: 
	case MAG_OPTION_X12_SH_PSF: 
	case MAG_OPTION_X22_SH_PSF: 
	case MAG_OPTION_E1_SH_PSF: 
	case MAG_OPTION_E2_SH_PSF: 

	case MAG_OPTION_E1_PSF: 
	case MAG_OPTION_E2_PSF: 

	case MAG_OPTION_F_AP_R5: 
	case MAG_OPTION_F_ERR_AP_R5: 
	case MAG_OPTION_F_STDEV_AP_R5: 
	case MAG_OPTION_F_FILL_AP_R5: 
	case MAG_OPTION_F_AP_R6: 
	case MAG_OPTION_F_ERR_AP_R6: 
	case MAG_OPTION_F_STDEV_AP_R6: 
	case MAG_OPTION_F_FILL_AP_R6: 
	case MAG_OPTION_F_AP_R7: 
	case MAG_OPTION_F_ERR_AP_R7: 
	case MAG_OPTION_F_STDEV_AP_R7: 
	case MAG_OPTION_F_FILL_AP_R7: 
	case MAG_OPTION_E1: 
	case MAG_OPTION_E2: 

	case MAG_OPTION_GAL_MAG:       
	case MAG_OPTION_GAL_MAG_ERR:   
	case MAG_OPTION_GAL_MAJ:       
	case MAG_OPTION_GAL_MAJ_ERR:   
	case MAG_OPTION_GAL_MIN:       
	case MAG_OPTION_GAL_MIN_ERR:   
	case MAG_OPTION_GAL_THETA:     
	case MAG_OPTION_GAL_THETA_ERR: 
	case MAG_OPTION_GAL_INDEX:     
	case MAG_OPTION_GAL_CHISQ:     
	case MAG_OPTION_GAL_NPIX:      
	case MAG_OPTION_GAL_FLAGS:      
	case MAG_OPTION_GAL_TYPE:      
	case MAG_OPTION_GAL_OBJ_ID:
	case MAG_OPTION_GAL_CAT_ID:
	case MAG_OPTION_GAL_DET_ID:
	case MAG_OPTION_GAL_IMAGE_ID:

	case MAG_OPTION_NONE:
	  break;
      }
      break;
    }
    case MEAS_RA: /* OK */
      value.Flt = measure[0].R;
      break;
    case MEAS_DEC: /* OK */
      value.Flt = measure[0].D;
      break;
    case MEAS_RA_AVE: /* OK */
      value.Flt = average[0].R;
      break;
    case MEAS_DEC_AVE: /* OK */
      value.Flt = average[0].D;
      break;

    case MEAS_GLON:
      if (!haveGalacticMeas) {
	ApplyTransform (&GLON_MEAS, &GLAT_MEAS, measure[0].R, measure[0].D, celestial_to_galactic);
	haveGalacticMeas = TRUE;
      }
      value.Flt = GLON_MEAS;
      break;
    case MEAS_GLAT:
      if (!haveGalacticMeas) {
	ApplyTransform (&GLON_MEAS, &GLAT_MEAS, measure[0].R, measure[0].D, celestial_to_galactic);
	haveGalacticMeas = TRUE;
      }
      value.Flt = GLAT_MEAS;
      break;
    case MEAS_ELON:
      if (!haveEclipticMeas) {
	ApplyTransform (&ELON_MEAS, &ELAT_MEAS, measure[0].R, measure[0].D, celestial_to_ecliptic);
	haveEclipticMeas = TRUE;
      }
      value.Flt = ELON_MEAS;
      break;
    case MEAS_ELAT:
      if (!haveEclipticMeas) {
	ApplyTransform (&ELON_MEAS, &ELAT_MEAS, measure[0].R, measure[0].D, celestial_to_ecliptic);
	haveEclipticMeas = TRUE;
      }
      value.Flt = ELAT_MEAS;
      break;

    case MEAS_GLON_AVE:
      if (!haveGalacticAve) {
	ApplyTransform (&GLON_AVE, &GLAT_AVE, average[0].R, average[0].D, celestial_to_galactic);
	haveGalacticAve = TRUE;
      }
      value.Flt = GLON_AVE;
      break;
    case MEAS_GLAT_AVE:
      if (!haveGalacticAve) {
	ApplyTransform (&GLON_AVE, &GLAT_AVE, average[0].R, average[0].D, celestial_to_galactic);
	haveGalacticAve = TRUE;
      }
      value.Flt = GLAT_AVE;
      break;
    case MEAS_ELON_AVE:
      if (!haveEclipticAve) {
	ApplyTransform (&ELON_AVE, &ELAT_AVE, average[0].R, average[0].D, celestial_to_ecliptic);
	haveEclipticAve = TRUE;
      }
      value.Flt = ELON_AVE;
      break;
    case MEAS_ELAT_AVE:
      if (!haveEclipticAve) {
	ApplyTransform (&ELON_AVE, &ELAT_AVE, average[0].R, average[0].D, celestial_to_ecliptic);
	haveEclipticAve = TRUE;
      }
      value.Flt = ELAT_AVE;
      break;

    case MEAS_RA_AVE_ERR: /* OK */
      value.Flt = average[0].dR;
      break;
    case MEAS_DEC_AVE_ERR: /* OK */
      value.Flt = average[0].dD;
      break;
    case MEAS_U_RA: /* OK */
      value.Flt = average[0].uR;
      break;
    case MEAS_U_DEC: /* OK */
      value.Flt = average[0].uD;
      break;
    case MEAS_U_RA_ERR: /* OK */
      value.Flt = average[0].duR;
      break;
    case MEAS_U_DEC_ERR: /* OK */
      value.Flt = average[0].duD;
      break;
    case MEAS_PAR: /* OK */
      value.Flt = average[0].P;
      break;
    case MEAS_PAR_ERR: /* OK */
      value.Flt = average[0].dP;
      break;
    case MEAS_CHISQ_POS: /* OK */
      value.Flt = average[0].ChiSqAve;
      break;
    case MEAS_CHISQ_PM: /* OK */
      value.Flt = average[0].ChiSqPM;
      break;
    case MEAS_CHISQ_PAR: /* OK */
      value.Flt = average[0].ChiSqPar;
      break;
    case MEAS_TMEAN: /* OK */
      value.Flt = TimeValue (average[0].Tmean, TimeReference, TimeFormat);
      break;
    case MEAS_TRANGE: /* OK */
      value.Flt = GetTimeRange (average[0].Trange, TimeFormat);
      break;
    case MEAS_NMEAS: /* OK */
      value.Int = average[0].Nmeasure;
      break;
    case MEAS_NMISS: /* OK */
      value.Int = average[0].Nmissing;
      break;
    case MEAS_NPOS: /* OK */
      value.Int = average[0].Npos;
      break;
    case MEAS_OBJ_FLAGS: /* OK */
      value.Int = average[0].flags;
      break;
    case MEAS_SECFILT_FLAGS: /* OK */
      equiv = GetPhotcodeEquivbyCode (measure[0].photcode);
      if (!equiv) break;
      Nsec = GetPhotcodeNsec (equiv->code);
      if (Nsec == -1) break;
      value.Int = secfilt[Nsec].flags;
      break;

    // note that these represent the ra displacement relative to the average, not 
    // the error.
    case MEAS_RA_OFFSET: /* OK */
      value.Flt = dvoOffsetR (measure, average);
      break;
    case MEAS_DEC_OFFSET: /* OK */
      value.Flt = dvoOffsetD (measure, average);
      break;
    case MEAS_RA_FIT_OFFSET: /* OK */
      // RA_epoch_fit = RA_mean + uR*(t - Tmean)/cos(dec) + plx*parR 
      // note that this extraction ignores parallax
      dT = (measure[0].t - average[0].Tmean) / (86400*365.25);
      dR = dvoOffsetR (measure, average); // RA_epoch - RA_mean (** NOT local linear distance **)
      value.Flt = dR*cos(RAD_DEG*measure[0].D) - average[0].uR * dT;
      // this is the local linear distance of the measurement from the fit 
      break;
    case MEAS_DEC_FIT_OFFSET: /* OK */
      dT = (measure[0].t - average[0].Tmean) / (86400*365.25);
      dD = dvoOffsetD (measure, average);
      value.Flt = dD - average[0].uD * dT;
      break;
    case MEAS_RA_OFFSET_ERR: /* OK */
      value.Flt = NAN;
      break;
    case MEAS_DEC_OFFSET_ERR: /* OK */
      value.Flt = NAN;
      break;
    case MEAS_AIRMASS: /* OK */
      value.Flt = measure[0].airmass;
      break;
    case MEAS_MEAN_AIRMASS: /* OK */
      if (REMOTE_CLIENT) {
	ImageMetadata *image = MatchImageMetadataDVO (measure[0].imageID);
	if (image == NULL) break;
	value.Flt = image[0].secz;
      } else {
	Image *image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	value.Flt = image[0].secz;
      }
      break;
    case MEAS_AZ: /* OK */
      value.Flt = measure[0].az;
      break;
    case MEAS_ALT: /* OK */
      value.Flt = 90.0 - DEG_RAD*acos(1.0/measure[0].airmass);
      break;
    case MEAS_EXPTIME: /* OK */
      value.Flt = pow (10.0, measure[0].dt * 0.4);
      break;
    case MEAS_PHOTCODE_EQUIV: /* OK */
      value.Int = GetPhotcodeEquivCodebyCode (measure[0].photcode);
      break;
    case MEAS_PHOTCODE: /* OK */
      value.Int = measure[0].photcode;
      break;
    case MEAS_PHOTCODE_KLAM: /* OK */
      { 
	PhotCode *code = GetPhotcodebyCode (measure[0].photcode);
	if (!code) break;
	value.Flt = code->K;
	break;
      }
    case MEAS_PHOTCODE_C: /* OK */
      { 
	PhotCode *code = GetPhotcodebyCode (measure[0].photcode);
	if (!code) break;
	value.Flt = code->C;
	break;
      }
    case MEAS_TIME: /* OK */
      value.Flt = TimeValue (measure[0].t, TimeReference, TimeFormat);
      break;
    case MEAS_FWHM: /* OK */
      value.Flt = FromShortPixels(measure[0].FWx + measure[0].FWy) / 2.0;
      break;
    case MEAS_FWHM_MAJ: /* OK */
      value.Flt = FromShortPixels(measure[0].FWx);
      break;
    case MEAS_FWHM_MIN: /* OK */
      value.Flt = FromShortPixels(measure[0].FWy);
      break;
    case MEAS_THETA: /* OK */
      value.Flt = FromShortDegrees(measure[0].theta);
      break;

    case MEAS_POSANGLE: /* OK */
      value.Flt = FromShortDegrees(measure[0].posangle);
      break;
    case MEAS_PLATESCALE: /* OK */
      value.Flt = measure[0].pltscale;
      break;

    case MEAS_REF_COLOR_BLUE:
      value.Flt = average[0].refColorBlue;
      break;
    case MEAS_REF_COLOR_RED:
      value.Flt = average[0].refColorRed;
      break;

    case MEAS_MXX: /* OK */
      value.Flt = FromShortPixels(measure[0].Mxx);
      break;
    case MEAS_MXY: /* OK */
      value.Flt = FromShortPixels(measure[0].Mxy);
      break;
    case MEAS_MYY: /* OK */
      value.Flt = FromShortPixels(measure[0].Myy);
      break;
    case MEAS_DOPHOT: /* OK */
      value.Int = (measure[0].photFlags >> 16);
      break;
    case MEAS_DB_FLAGS: /* ? */
      value.Int = measure[0].dbFlags;
      break;
    case MEAS_PHOT_FLAGS: /* ? */
      value.Int = measure[0].photFlags;
      break;
    case MEAS_PHOT_FLAGS2: /* ? */
      value.Int = measure[0].photFlags2;
      break;
    case MEAS_XCCD: /* OK */
# if MEASURE_HAS_XCCD
      value.Flt = measure[0].Xccd;
# else
      { 
	// I'm keeping this code because it gives a way of handing dvo dbs that don't have
	// measure.xccd if we need it
	Image *image;
	ra  = measure[0].R;
	dec = measure[0].D;
	image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	RD_to_XY (&x, &y, ra, dec, &image[0].coords);
	value.Flt = x;
      }
# endif
      break;
    case MEAS_YCCD: /* OK */
# if MEASURE_HAS_XCCD
      value.Flt = measure[0].Yccd;
# else
      {
	Image *image;
	ra  = measure[0].R;
	dec = measure[0].D;
	image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	RD_to_XY (&x, &y, ra, dec, &image[0].coords);
	value.Flt = y;
      }
# endif
      break;
    case MEAS_XFIX: { value.Flt = measure[0].Xfix; break; }
    case MEAS_YFIX: { value.Flt = measure[0].Yfix; break; }
    case MEAS_XCCD_ERR: { value.Flt = FromShortPixels(measure[0].dXccd); break; }
    case MEAS_YCCD_ERR: { value.Flt = FromShortPixels(measure[0].dYccd); break; }
    case MEAS_POS_SYS_ERR: { value.Flt = FromShortPixels(measure[0].dRsys); }

    case MEAS_XOFF_KH:  { value.Flt = measure[0].XoffKH;  break; }
    case MEAS_YOFF_KH:  { value.Flt = measure[0].YoffKH;  break; }
    case MEAS_XOFF_DCR: { value.Flt = measure[0].XoffDCR; break; }
    case MEAS_YOFF_DCR: { value.Flt = measure[0].YoffDCR; break; }
    case MEAS_XOFF_CAM: { value.Flt = measure[0].XoffCAM; break; }
    case MEAS_YOFF_CAM: { value.Flt = measure[0].YoffCAM; break; }

    case MEAS_XFIELD: /* offset relative to exposure center in ra,dec space */
      if (!haveFieldMeas) {
	if (REMOTE_CLIENT) {
	  fieldc = MatchFieldMetadata (measure[0].imageID);
	} else {
	  // fprintf (stderr, "non-parallel Xmos broken\n");
	  // abort();
	  // fieldc = MatchField (measure[0].t, measure[0].photcode);
	  fieldc = MatchFieldMetadata (measure[0].imageID);
	}
	if (fieldc == NULL) break;
	double Rm = measure[0].R;
	double Dm = measure[0].D;
	RD_to_XY (&XFIELD_MEAS, &YFIELD_MEAS, Rm, Dm, fieldc);
      }
      value.Flt = XFIELD_MEAS;
      break;
    case MEAS_YFIELD: /* OK */
      if (!haveFieldMeas) {
	if (REMOTE_CLIENT) {
	  fieldc = MatchFieldMetadata (measure[0].imageID);
	} else {
	  // fprintf (stderr, "non-parallel Xmos broken\n");
	  // abort();
	  // fieldc = MatchField (measure[0].t, measure[0].photcode);
	  fieldc = MatchFieldMetadata (measure[0].imageID);
	}
	if (fieldc == NULL) break;
	double Rm = measure[0].R;
	double Dm = measure[0].D;
	RD_to_XY (&XFIELD_MEAS, &YFIELD_MEAS, Rm, Dm, fieldc);
      }
      value.Flt = YFIELD_MEAS;
      break;

    case MEAS_XMOSAIC: /* offset relative to exposure center in camera coords */
      if (!haveMosaicMeas) {
	mosaic = MatchMosaicMetadata (measure[0].imageID);
	if (mosaic == NULL) break;
	double Rm = measure[0].R;
	double Dm = measure[0].D;
	RD_to_XY (&XMOS_MEAS, &YMOS_MEAS, Rm, Dm, mosaic);
      }
      value.Flt = XMOS_MEAS;
      break;
    case MEAS_YMOSAIC: /* OK */
      if (!haveMosaicMeas) {
	mosaic = MatchMosaicMetadata (measure[0].imageID);
	if (mosaic == NULL) break;
	double Rm = measure[0].R;
	double Dm = measure[0].D;
	RD_to_XY (&XMOS_MEAS, &YMOS_MEAS, Rm, Dm, mosaic);
      }
      value.Flt = YMOS_MEAS;
      break;

    case MEAS_SKY: /* OK */
      value.Flt = measure[0].Sky;
      break;
    case MEAS_dSKY: /* OK */
      value.Flt = measure[0].dSky;
      break;
    case MEAS_DET_ID: /* OK */
      value.Int = measure[0].detID;
      break;
    case MEAS_OBJ_ID: /* OK */
      value.Int = average[0].objID;
      break;
    case MEAS_CAT_ID: /* OK */
      value.Int = average[0].catID;
      break;
    case MEAS_IMAGE_ID: /* OK */
      value.Int = measure[0].imageID;
      break;
    case MEAS_EXT_ID: /* OK */
      value.Int = measure[0].extID;
      break;
    case MEAS_PSF_QF: /* OK */
      value.Flt = measure[0].psfQF;
      break;
    case MEAS_PSF_QF_PERFECT: /* OK */
      value.Flt = measure[0].psfQFperf;
      break;
    case MEAS_PSF_CHISQ: /* OK */
      value.Flt = measure[0].psfChisq;
      break;
    case MEAS_PSF_NDOF: /* OK */
      value.Int = measure[0].psfNdof;
      break;
    case MEAS_PSF_NPIX: /* OK */
      value.Int = measure[0].psfNpix;
      break;
    case MEAS_EXT_NSIGMA: /* OK */
      value.Flt = measure[0].extNsigma;
      break;
    case MEAS_IMAGE_EXTERN_ID: /* OK */
      if (REMOTE_CLIENT) {
	ImageMetadata *image = MatchImageMetadataDVO (measure[0].imageID);
	if (image == NULL) break;
	value.Int = image->externID;
      } else {
	Image *image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	value.Int = image[0].externID;
      }
      break;

    case MEAS_EXPNAME_AS_INT:
      if (REMOTE_CLIENT) {
	ImageMetadata *image = MatchImageMetadataDVO (measure[0].imageID);
	if (image == NULL) break;
	value.Int = image->expname;
      } else {
	Image *image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	// XXX very crude: if this matches oNNNNgNNNNo, then convert to an int
	// otherwise, leave as 0
	if ((image->name[0] == 'o') && (image->name[5] == 'g') && (image->name[10] == 'o')) {
	  int mjd  = atoi(&image->name[1]);
	  int Nexp = atoi(&image->name[6]);
	  value.Int = mjd * 10000 + Nexp;
	}
      }
      break;

    case MEAS_MCAL_OFFSET_PSF:  { value.Flt = measure[0].McalPSF;  break; }
    case MEAS_MCAL_OFFSET_APER: { value.Flt = measure[0].McalAPER; break; }
    case MEAS_FLAT: { value.Flt = measure[0].Mflat; break; }

      // we have measure[0].Xccd,Yccd and image[0].NX,NY.  Find the distance to the center
    case MEAS_CENTER_OFFSET: /* OK */
      if (REMOTE_CLIENT) {
	ImageMetadata *image = MatchImageMetadataDVO (measure[0].imageID);
	if (image == NULL) break;
	float Xcenter = image[0].Xcenter;
	float Ycenter = image[0].Ycenter;
	float distance = hypot (measure[0].Xccd - Xcenter, measure[0].Yccd - Ycenter);
	value.Flt = distance;
      } else {
	Image *image = MatchImageDVO (measure[0].t, measure[0].photcode, measure[0].imageID);
	if (image == NULL) break;
	// XXX we may hypotetically have images with -NX to +NX here (eg, projection center), but 
	// we do not get a detection from that type of image
	float Xcenter = 0.5*image[0].NX;
	float Ycenter = 0.5*image[0].NY;
	float distance = hypot (measure[0].Xccd - Xcenter, measure[0].Yccd - Ycenter);
	value.Flt = distance;
      }
      break;

    case MEAS_X11_SM_OBJ: { value.Flt = lensing ? lensing->X11_sm_obj : NAN; break; } 
    case MEAS_X12_SM_OBJ: { value.Flt = lensing ? lensing->X12_sm_obj : NAN; break; } 
    case MEAS_X22_SM_OBJ: { value.Flt = lensing ? lensing->X22_sm_obj : NAN; break; } 
    case MEAS_E1_SM_OBJ:  { value.Flt = lensing ? lensing->E1_sm_obj  : NAN; break; } 
    case MEAS_E2_SM_OBJ:  { value.Flt = lensing ? lensing->E2_sm_obj  : NAN; break; } 
    case MEAS_X11_SH_OBJ: { value.Flt = lensing ? lensing->X11_sh_obj : NAN; break; } 
    case MEAS_X12_SH_OBJ: { value.Flt = lensing ? lensing->X12_sh_obj : NAN; break; } 
    case MEAS_X22_SH_OBJ: { value.Flt = lensing ? lensing->X22_sh_obj : NAN; break; } 
    case MEAS_E1_SH_OBJ:  { value.Flt = lensing ? lensing->E1_sh_obj  : NAN; break; } 
    case MEAS_E2_SH_OBJ:  { value.Flt = lensing ? lensing->E2_sh_obj  : NAN; break; } 
    case MEAS_X11_SM_PSF: { value.Flt = lensing ? lensing->X11_sm_psf : NAN; break; } 
    case MEAS_X12_SM_PSF: { value.Flt = lensing ? lensing->X12_sm_psf : NAN; break; } 
    case MEAS_X22_SM_PSF: { value.Flt = lensing ? lensing->X22_sm_psf : NAN; break; } 
    case MEAS_E1_SM_PSF:  { value.Flt = lensing ? lensing->E1_sm_psf  : NAN; break; } 
    case MEAS_E2_SM_PSF:  { value.Flt = lensing ? lensing->E2_sm_psf  : NAN; break; } 
    case MEAS_X11_SH_PSF: { value.Flt = lensing ? lensing->X11_sh_psf : NAN; break; } 
    case MEAS_X12_SH_PSF: { value.Flt = lensing ? lensing->X12_sh_psf : NAN; break; } 
    case MEAS_X22_SH_PSF: { value.Flt = lensing ? lensing->X22_sh_psf : NAN; break; } 
    case MEAS_E1_SH_PSF:  { value.Flt = lensing ? lensing->E1_sh_psf  : NAN; break; } 
    case MEAS_E2_SH_PSF:  { value.Flt = lensing ? lensing->E2_sh_psf  : NAN; break; } 

    case MEAS_E1_PSF:     { value.Flt = lensing ? lensing->E1_psf     : NAN; break; } 
    case MEAS_E2_PSF:     { value.Flt = lensing ? lensing->E2_psf     : NAN; break; } 

    case MEAS_F_AP_R5:       {
      value.Flt = lensing ? lensing-> F_ApR5 : NAN; break;
    } 
    case MEAS_F_ERR_AP_R5:   { value.Flt = lensing ? lensing->dF_ApR5 : NAN; break; } 
    case MEAS_F_STDEV_AP_R5: { value.Flt = lensing ? lensing->sF_ApR5 : NAN; break; } 
    case MEAS_F_FILL_AP_R5:  { value.Flt = lensing ? lensing->fF_ApR5 : NAN; break; } 
    case MEAS_F_AP_R6:       { value.Flt = lensing ? lensing-> F_ApR6 : NAN; break; } 
    case MEAS_F_ERR_AP_R6:   { value.Flt = lensing ? lensing->dF_ApR6 : NAN; break; } 
    case MEAS_F_STDEV_AP_R6: { value.Flt = lensing ? lensing->sF_ApR6 : NAN; break; } 
    case MEAS_F_FILL_AP_R6:  { value.Flt = lensing ? lensing->fF_ApR6 : NAN; break; } 
    case MEAS_F_AP_R7:       { value.Flt = lensing ? lensing-> F_ApR7 : NAN; break; } 
    case MEAS_F_ERR_AP_R7:   { value.Flt = lensing ? lensing->dF_ApR7 : NAN; break; } 
    case MEAS_F_STDEV_AP_R7: { value.Flt = lensing ? lensing->sF_ApR7 : NAN; break; } 
    case MEAS_F_FILL_AP_R7:  { value.Flt = lensing ? lensing->fF_ApR7 : NAN; break; } 

    case MEAS_E_BV:             { value.Flt = starpar ? starpar->Ebv      : NAN; break; }
    case MEAS_E_BV_ERR:         { value.Flt = starpar ? starpar->dEbv     : NAN; break; }
    case MEAS_DISTANCE_MOD:     { value.Flt = starpar ? starpar->DistMag  : NAN; break; }
    case MEAS_DISTANCE_MOD_ERR: { value.Flt = starpar ? starpar->dDistMag : NAN; break; }
    case MEAS_M_R:              { value.Flt = starpar ? starpar->M_r      : NAN; break; }
    case MEAS_M_R_ERR:          { value.Flt = starpar ? starpar->dM_r     : NAN; break; }
    case MEAS_FEH:              { value.Flt = starpar ? starpar->FeH      : NAN; break; }
    case MEAS_FEH_ERR:          { value.Flt = starpar ? starpar->dFeH     : NAN; break; }
    case MEAS_URA_GALMODEL:     { value.Flt = starpar ? starpar->uRA      : NAN; break; }
    case MEAS_UDEC_GALMODEL:    { value.Flt = starpar ? starpar->uDEC     : NAN; break; }
    case MEAS_RA_GALMODEL:      { value.Flt = starpar ? starpar->R        : NAN; break; }
    case MEAS_DEC_GALMODEL:     { value.Flt = starpar ? starpar->D        : NAN; break; }

    // case MEAS_FLUX_PSF: /* OK */
    //   value.Flt = measure[0].FluxPSF;
    //   break;
    // case MEAS_FLUX_PSF_ERR: /* OK */
    //   value.Flt = measure[0].dFluxPSF;
    //   break;
    // case MEAS_FLUX_KRON: /* OK */
    //   value.Flt = measure[0].FluxKron;
    //   break;
    // case MEAS_FLUX_KRON_ERR: /* OK */
    //   value.Flt = measure[0].dFluxKron;
    //   break;
  }
  return (value);
}

/** the mosaic entries do not use the registered mosaic found 
    by MatchImage (via FindMosaicForImage).  Rather, they use
    a coordinate frame saved by SetImageSelection 
**/
