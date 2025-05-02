# include "dvo.h"
# include "libdvo_astro.h"

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

// define a locally-static transform
int dbExtractAveragesInitTransform (CoordTransformSystem target) {

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

int dbExtractAveragesInit () {
  GetTimeFormat (&TimeReference, &TimeFormat);
  return (TRUE);
}

int dbExtractAveragesInitAve () {
  haveGalactic = FALSE;
  haveEcliptic = FALSE;
  return (TRUE);
}

/* return average.field based on the selection */
// secfilt is a pointer to the first of Nsecfilt entries : code below decides which entry based on requested photcode
// measure is a pointer to the first of Nmeasure entries
// lensobj is a pointer to the first of Nlensobj entries : need to compare photcode to requested code
dbValue dbExtractAverages (Average *average, SecFilt *secfilt, Measure *measure, Lensobj *lensobj, StarPar *starpar, GalPhot *galphot, dbField *field) {

  // off_t i;
  int n;
  dbValue value;

  value.Flt = NAN;
  value.Int =   0;

  /* assign vector values */
  switch (field->ID) {
    case AVE_RA:
      value.Flt = average[0].R;
      break;
    case AVE_DEC:
      value.Flt = average[0].D;
      break;
    case AVE_GLON:
      if (!haveGalactic) {
	ApplyTransform (&GLON, &GLAT, average[0].R, average[0].D, celestial_to_galactic);
	haveGalactic = TRUE;
      }
      value.Flt = GLON;
      break;
    case AVE_GLAT:
      if (!haveGalactic) {
	ApplyTransform (&GLON, &GLAT, average[0].R, average[0].D, celestial_to_galactic);
	haveGalactic = TRUE;
      }
      value.Flt = GLAT;
      break;
    case AVE_ELON:
      if (!haveEcliptic) {
	ApplyTransform (&ELON, &ELAT, average[0].R, average[0].D, celestial_to_ecliptic);
	haveEcliptic = TRUE;
      }
      value.Flt = ELON;
      break;
    case AVE_ELAT:
      if (!haveEcliptic) {
	ApplyTransform (&ELON, &ELAT, average[0].R, average[0].D, celestial_to_ecliptic);
	haveEcliptic = TRUE;
      }
      value.Flt = ELAT;
      break;
    case AVE_RA_ERR:
      value.Flt = average[0].dR;
      break;
    case AVE_DEC_ERR:
      value.Flt = average[0].dD;
      break;

    case AVE_U_RA:
      value.Flt = average[0].uR;
      break;
    case AVE_U_DEC:
      value.Flt = average[0].uD;
      break;
    case AVE_U_RA_ERR:
      value.Flt = average[0].duR;
      break;
    case AVE_U_DEC_ERR:
      value.Flt = average[0].duD;
      break;

    case AVE_PAR:
      value.Flt = average[0].P;
      break;
    case AVE_PAR_ERR:
      value.Flt = average[0].dP;
      break;

    case AVE_NMEAS:
      value.Int = average[0].Nmeasure;
      break;
    case AVE_NMISS:
      value.Int = average[0].Nmissing;
      break;
    case AVE_NLENSING:
      value.Int = average[0].Nlensing;
      break;
    case AVE_NLENSOBJ:
      value.Int = average[0].Nlensobj;
      break;
    case AVE_NGALPHOT:
      value.Int = average[0].Ngalphot;
      break;
    case AVE_NSTARPAR:
      value.Int = average[0].Nstarpar;
      break;
    case AVE_NPOS:
      value.Int = average[0].Npos;
      break;
    case AVE_NWARP_OK:
      value.Int = average[0].NwarpOK;
      break;

    case AVE_CHISQ_POS:
      value.Flt = average[0].ChiSqAve;
      break;
    case AVE_CHISQ_PM:
      value.Flt = average[0].ChiSqPM;
      break;
    case AVE_CHISQ_PAR:
      value.Flt = average[0].ChiSqPar;
      break;

    // XXX case AVE_PM_GROUPS:
    // XXX   value.Int = GetProperMotionGroups (average, measure);
    // XXX   break;

    case AVE_TMEAN:
      value.Flt = TimeValue (average[0].Tmean, TimeReference, TimeFormat);
      break;
    case AVE_TRANGE:
      value.Flt = GetTimeRange (average[0].Trange, TimeFormat);
      break;

    case AVE_PSF_QF:
      value.Flt = average[0].psfQF;
      break;
    case AVE_PSF_QF_PERF:
      value.Flt = average[0].psfQFperf;
      break;
    case AVE_STARGAL:
      value.Flt = average[0].stargal;
      break;
    case AVE_REF_COLOR_BLUE:
      value.Flt = average[0].refColorBlue;
      break;
    case AVE_REF_COLOR_RED:
      value.Flt = average[0].refColorRed;
      break;

    case AVE_OBJ_FLAGS:
      value.Int = average[0].flags;
      break;
    case AVE_OBJID:
      value.Int = average[0].objID;
      break;
    case AVE_CATID:
      value.Int = average[0].catID;
      break;
    case AVE_EXTID_HI:
      value.Int = (0x00000000FFFFFFFF & (average[0].extID >> 32));
      break;
    case AVE_EXTID_LO:
      value.Int = (0x00000000FFFFFFFF & average[0].extID);
      break;

    case AVE_PHOT_FLAGS_HI:
      value.Int = average[0].photFlagsUpper;
      break;
    case AVE_PHOT_FLAGS_LO:
      value.Int = average[0].photFlagsLower;
      break;

    case AVE_PHOT:
      // if we request mag:ave, use equiv for photcode (ie a given measure, say GPC1.g.XY01, will return g for mag:ave)

      // this is an error (no exposed photcodes should be of type PHOT_MAG)
      if  (field->photcode->type == PHOT_MAG) break;

      switch (field->magOption) {
	case MAG_OPTION_MAG:
	  switch (field->magLevel) {
	    case MAG_LEVEL_AVE:
	      value.Flt = PhotAve  (field->photcode, average, secfilt, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_REF:
	      break;
	    case MAG_LEVEL_INST:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotInst (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAT:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotCat (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_SYS:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotSys  (&measure[n], average, secfilt, field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_REL:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotRel (&measure[n], average, secfilt, field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAL:
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_ERR:
	  switch (field->magLevel) {
	    case MAG_LEVEL_AVE:
	    case MAG_LEVEL_REF:
	      value.Flt = PhotAveErr (field->photcode, average, secfilt, field->magClass, field->magSource);  
	      break;
	    case MAG_LEVEL_INST:
	    case MAG_LEVEL_CAT:
	    case MAG_LEVEL_SYS:
	    case MAG_LEVEL_REL:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotErr (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAL:
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_FLUX:
	  switch (field->magLevel) {
	    case MAG_LEVEL_AVE:
	      value.Flt = PhotFluxAve  (field->photcode, average, secfilt, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_REF:
	      // XXX which measure is needed here?
	      // value.Flt = PhotFluxRef  (field->photcode, average, secfilt, measure, field->magClass, field->magSource); 
	      break;
	    case MAG_LEVEL_INST:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotFluxInst (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAT:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotFluxCat (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_SYS:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotFluxSys (&measure[n], average, secfilt, field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_REL:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotFluxRel (&measure[n], average, secfilt, field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAL:
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_FLUX_ERR:
	  switch (field->magLevel) {
	    case MAG_LEVEL_AVE:
	    case MAG_LEVEL_REF:
	      value.Flt = PhotFluxAveErr (field->photcode, average, secfilt, field->magClass, field->magSource);  
	      break;
	    case MAG_LEVEL_INST:
	    case MAG_LEVEL_CAT:
	    case MAG_LEVEL_SYS:
	    case MAG_LEVEL_REL:
	      // find an appropriate measure (if one exists)
	      for (n = 0; measure && n < average->Nmeasure; n++) {
		if (field->photcode->code != measure[n].photcode) continue;
		value.Flt = PhotFluxInstErr (&measure[n], field->magClass); 
		break;
	      }
	      break;
	    case MAG_LEVEL_CAL:
	    case MAG_LEVEL_NONE:
	      break;
	  }
	  break;

	case MAG_OPTION_STDEV:
	  value.Flt = PhotMstdev (field->photcode, average, secfilt, field->magClass, field->magSource);
	  break;
	case MAG_OPTION_CHISQ:
	  value.Flt = PhotXm (field->photcode, average, secfilt);
	  break;
	case MAG_OPTION_MIN:
	  value.Flt = PhotMmin (field->photcode, average, secfilt);
	  break;
	case MAG_OPTION_MAX:
	  value.Flt = PhotMmax (field->photcode, average, secfilt);
	  break;
	case MAG_OPTION_NCODE: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Int = secfilt[Nsec].Ncode;
	  break;
	}
	case MAG_OPTION_NPHOT: {
	  value.Int = PhotNphot (field->photcode, average, secfilt, field->magClass, field->magSource);
	  break;
	}
	case MAG_OPTION_NWARP: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Int = secfilt[Nsec].Nwarp;
	  break;
	}
	case MAG_OPTION_NWARP_GOOD: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Int = secfilt[Nsec].NwarpGood;
	  break;
	}
	case MAG_OPTION_NSTACK: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Int = secfilt[Nsec].Nstack;
	  break;
	}
	case MAG_OPTION_NSTACK_DET: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Int = secfilt[Nsec].NstackDet;
	  break;
	}
	case MAG_OPTION_UC_DIST:
	  value.Flt = PhotUCdist (field->photcode, average, secfilt);
	  break;
	case MAG_OPTION_FLAGS: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) {
	    for (n = 0; measure && n < average->Nmeasure; n++) {
	      if (field->photcode->code != measure[n].photcode) continue;
	      value.Int = measure[n].photFlags; 
	      break;
	    }
	    break;
	  }
	  value.Int = secfilt[Nsec].flags;
	  break;
	}
	case MAG_OPTION_PSF_QF: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Flt = secfilt[Nsec].psfQfMax;
	  break;
	}
	case MAG_OPTION_PSF_QF_PERFECT: {
	  int Nsec = GetPhotcodeNsec (field->photcode->code);
	  if (Nsec == -1) break;
	  value.Flt = secfilt[Nsec].psfQfPerfMax;
	  break;
	}

	  // g:X11_SM_OBJ, etc, are only valid for average 
	case MAG_OPTION_X11_SM_OBJ: { value.Flt = LensValue_X11_sm_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X12_SM_OBJ: { value.Flt = LensValue_X12_sm_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X22_SM_OBJ: { value.Flt = LensValue_X22_sm_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E1_SM_OBJ:  { value.Flt = LensValue_E1_sm_obj  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E2_SM_OBJ:  { value.Flt = LensValue_E2_sm_obj  (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_X11_SH_OBJ: { value.Flt = LensValue_X11_sh_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X12_SH_OBJ: { value.Flt = LensValue_X12_sh_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X22_SH_OBJ: { value.Flt = LensValue_X22_sh_obj (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E1_SH_OBJ:  { value.Flt = LensValue_E1_sh_obj  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E2_SH_OBJ:  { value.Flt = LensValue_E2_sh_obj  (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_X11_SM_PSF: { value.Flt = LensValue_X11_sm_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X12_SM_PSF: { value.Flt = LensValue_X12_sm_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X22_SM_PSF: { value.Flt = LensValue_X22_sm_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E1_SM_PSF:  { value.Flt = LensValue_E1_sm_psf  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E2_SM_PSF:  { value.Flt = LensValue_E2_sm_psf  (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_X11_SH_PSF: { value.Flt = LensValue_X11_sh_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X12_SH_PSF: { value.Flt = LensValue_X12_sh_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_X22_SH_PSF: { value.Flt = LensValue_X22_sh_psf (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E1_SH_PSF:  { value.Flt = LensValue_E1_sh_psf  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E2_SH_PSF:  { value.Flt = LensValue_E2_sh_psf  (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_F_AP_R5:       { value.Flt = LensValue_F_ApR5  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_ERR_AP_R5:   { value.Flt = LensValue_dF_ApR5 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_STDEV_AP_R5: { value.Flt = LensValue_sF_ApR5 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_FILL_AP_R5:  { value.Flt = LensValue_fF_ApR5 (field->photcode, lensobj, average->Nlensobj); break; }
												
	case MAG_OPTION_F_AP_R6:       { value.Flt = LensValue_F_ApR6  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_ERR_AP_R6:   { value.Flt = LensValue_dF_ApR6 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_STDEV_AP_R6: { value.Flt = LensValue_sF_ApR6 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_FILL_AP_R6:  { value.Flt = LensValue_fF_ApR6 (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_F_AP_R7:       { value.Flt = LensValue_F_ApR7  (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_ERR_AP_R7:   { value.Flt = LensValue_dF_ApR7 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_STDEV_AP_R7: { value.Flt = LensValue_sF_ApR7 (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_F_FILL_AP_R7:  { value.Flt = LensValue_fF_ApR7 (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_E1:  	       { value.Flt = LensValue_E1      (field->photcode, lensobj, average->Nlensobj); break; }
	case MAG_OPTION_E2:  	       { value.Flt = LensValue_E2      (field->photcode, lensobj, average->Nlensobj); break; }

	case MAG_OPTION_NONE:
	  break;

	case MAG_OPTION_GAL_MAG:      { value.Flt = GalphotValue_GAL_MAG      (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_MAG_ERR:  { value.Flt = GalphotValue_GAL_MAG_ERR  (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_MAJ:      { value.Flt = GalphotValue_GAL_MAJ      (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_MAJ_ERR:  { value.Flt = GalphotValue_GAL_MAJ_ERR  (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_MIN:      { value.Flt = GalphotValue_GAL_MIN      (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_MIN_ERR:  { value.Flt = GalphotValue_GAL_MIN_ERR  (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_THETA:    { value.Flt = GalphotValue_GAL_THETA    (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_THETA_ERR:{ value.Flt = GalphotValue_GAL_THETA_ERR(field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_INDEX:    { value.Flt = GalphotValue_GAL_INDEX    (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_CHISQ:    { value.Flt = GalphotValue_GAL_CHISQ    (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_NPIX:     { value.Flt = GalphotValue_GAL_NPIX     (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_TYPE:     { value.Flt = GalphotValue_GAL_TYPE     (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_FLAGS:    { value.Flt = GalphotValue_GAL_FLAGS    (field->photcode, field->magClass, galphot, average->Ngalphot); break; }

	case MAG_OPTION_GAL_OBJ_ID:   { value.Flt = GalphotValue_GAL_OBJ_ID   (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_CAT_ID:   { value.Flt = GalphotValue_GAL_CAT_ID   (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_DET_ID:   { value.Flt = GalphotValue_GAL_DET_ID   (field->photcode, field->magClass, galphot, average->Ngalphot); break; }
	case MAG_OPTION_GAL_IMAGE_ID: { value.Flt = GalphotValue_GAL_IMAGE_ID (field->photcode, field->magClass, galphot, average->Ngalphot); break; }

	default:
	  break;
      }
      break;

      // this section assumes a 1-to-1 match between average and starpar
    case AVE_E_BV:             { value.Flt = starpar->Ebv;      break; }
    case AVE_E_BV_ERR:         { value.Flt = starpar->dEbv;     break; }
    case AVE_DISTANCE_MOD:     { value.Flt = starpar->DistMag;  break; }
    case AVE_DISTANCE_MOD_ERR: { value.Flt = starpar->dDistMag; break; }
    case AVE_M_R:     	       { value.Flt = starpar->M_r;      break; }
    case AVE_M_R_ERR: 	       { value.Flt = starpar->dM_r;     break; }
    case AVE_FEH:     	       { value.Flt = starpar->FeH;      break; }
    case AVE_FEH_ERR: 	       { value.Flt = starpar->dFeH;     break; }
    case AVE_URA_GALMODEL:     { value.Flt = starpar->uRA;      break; }
    case AVE_UDEC_GALMODEL:    { value.Flt = starpar->uDEC;     break; }
    case AVE_RA_GALMODEL:      { value.Flt = starpar->R;        break; }
    case AVE_DEC_GALMODEL:     { value.Flt = starpar->D;        break; }

    case AVE_TYPE:
      break;
    case AVE_TYPEFRAC:
      break;


  }
  return (value);
}  

// XXX int GetProperMotionGroups (Average *average, Measure *measure) {
// XXX   // need the times, excluding ignored detections
// XXX   // sort the images
// XXX   for (i = 0; i < Ntimes - 1; i++)  {
// XXX     if (time[i+1] - time[i] < TRANGE) Ngroup ++;
// XXX   }
// XXX   return Ngroup;
// XXX }
// XXX 
