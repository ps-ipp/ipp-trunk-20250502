# include "dvo.h"

void dbFreeFields (dbField *fields, int Nfields) {

  int i;

  if (fields == NULL) return;

  for (i = 0; i < Nfields; i++) {
    if (fields[i].name != NULL) free (fields[i].name);
  }
  free (fields);
}

void dbInitField (dbField *field) {
  field->name = NULL;
  field->extract = FALSE;
  field->table = 0;
  field->ID = 0;
  field->type = OPIHI_FLT;

  field->magSource = MAG_SRC_NONE;
  field->magLevel  = MAG_LEVEL_NONE;
  field->magClass  = MAG_CLASS_NONE;
  field->magOption = MAG_OPTION_NONE;

  field->photcode = NULL;
}

int dbFieldNeedMeasure (dbField *fields, int Nfields) {
  int i;
  for (i = 0; i < Nfields; i++) {
    if (fields[i].photcode == NULL) continue; // non-measure fields do not have a photcode
    if (fields[i].photcode[0].type == PHOT_REF) return TRUE;
    if (fields[i].photcode[0].type == PHOT_DEP) return TRUE;
  }
  return FALSE;
}

int dbFieldNeedLensing (dbField *fields, int Nfields) {
  int i;
  for (i = 0; i < Nfields; i++) {
    if (fields[i].ID == MEAS_X11_SM_OBJ) return TRUE;
    if (fields[i].ID == MEAS_X12_SM_OBJ) return TRUE;
    if (fields[i].ID == MEAS_X22_SM_OBJ) return TRUE;
    if (fields[i].ID == MEAS_E1_SM_OBJ)  return TRUE;
    if (fields[i].ID == MEAS_E2_SM_OBJ)  return TRUE;
    if (fields[i].ID == MEAS_X11_SH_OBJ) return TRUE;
    if (fields[i].ID == MEAS_X12_SH_OBJ) return TRUE;
    if (fields[i].ID == MEAS_X22_SH_OBJ) return TRUE;
    if (fields[i].ID == MEAS_E1_SH_OBJ)  return TRUE;
    if (fields[i].ID == MEAS_E2_SH_OBJ)  return TRUE;
    if (fields[i].ID == MEAS_X11_SM_PSF) return TRUE;
    if (fields[i].ID == MEAS_X12_SM_PSF) return TRUE;
    if (fields[i].ID == MEAS_X22_SM_PSF) return TRUE;
    if (fields[i].ID == MEAS_E1_SM_PSF)  return TRUE;
    if (fields[i].ID == MEAS_E2_SM_PSF)  return TRUE;
    if (fields[i].ID == MEAS_X11_SH_PSF) return TRUE;
    if (fields[i].ID == MEAS_X12_SH_PSF) return TRUE;
    if (fields[i].ID == MEAS_X22_SH_PSF) return TRUE;
    if (fields[i].ID == MEAS_E1_SH_PSF)  return TRUE;
    if (fields[i].ID == MEAS_E2_SH_PSF)  return TRUE;

    if (fields[i].ID == MEAS_E1_PSF)     return TRUE;
    if (fields[i].ID == MEAS_E2_PSF)     return TRUE;

    if (fields[i].ID == MEAS_F_AP_R5)       return TRUE;
    if (fields[i].ID == MEAS_F_ERR_AP_R5)   return TRUE;
    if (fields[i].ID == MEAS_F_STDEV_AP_R5) return TRUE;
    if (fields[i].ID == MEAS_F_FILL_AP_R5)  return TRUE;

    if (fields[i].ID == MEAS_F_AP_R6)       return TRUE;
    if (fields[i].ID == MEAS_F_ERR_AP_R6)   return TRUE;
    if (fields[i].ID == MEAS_F_STDEV_AP_R6) return TRUE;
    if (fields[i].ID == MEAS_F_FILL_AP_R6)  return TRUE;

    if (fields[i].ID == MEAS_F_AP_R7)       return TRUE;
    if (fields[i].ID == MEAS_F_ERR_AP_R7)   return TRUE;
    if (fields[i].ID == MEAS_F_STDEV_AP_R7) return TRUE;
    if (fields[i].ID == MEAS_F_FILL_AP_R7)  return TRUE;
  }
  return FALSE;
}

int dbFieldNeedLensobj (dbField *fields, int Nfields) {
  int i;
  for (i = 0; i < Nfields; i++) {
    if (fields[i].magOption == MAG_OPTION_NONE) continue; // non-measure fields do not have a photcode

    if (fields[i].magOption == MAG_OPTION_X11_SM_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X12_SM_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X22_SM_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_E1_SM_OBJ)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2_SM_OBJ)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_X11_SH_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X12_SH_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X22_SH_OBJ) return TRUE;
    if (fields[i].magOption == MAG_OPTION_E1_SH_OBJ)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2_SH_OBJ)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_X11_SM_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X12_SM_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X22_SM_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_E1_SM_PSF)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2_SM_PSF)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_X11_SH_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X12_SH_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_X22_SH_PSF) return TRUE;
    if (fields[i].magOption == MAG_OPTION_E1_SH_PSF)  return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2_SH_PSF)  return TRUE;

    if (fields[i].magOption == MAG_OPTION_E1_PSF)     return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2_PSF)     return TRUE;

    if (fields[i].magOption == MAG_OPTION_F_AP_R5)       return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_ERR_AP_R5)   return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_STDEV_AP_R5) return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_FILL_AP_R5)  return TRUE;

    if (fields[i].magOption == MAG_OPTION_F_AP_R6)       return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_ERR_AP_R6)   return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_STDEV_AP_R6) return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_FILL_AP_R6)  return TRUE;

    if (fields[i].magOption == MAG_OPTION_F_AP_R7)       return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_ERR_AP_R7)   return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_STDEV_AP_R7) return TRUE;
    if (fields[i].magOption == MAG_OPTION_F_FILL_AP_R7)  return TRUE;

    if (fields[i].magOption == MAG_OPTION_E1)            return TRUE;
    if (fields[i].magOption == MAG_OPTION_E2)            return TRUE;
  }
  return FALSE;
}

int dbFieldNeedStarpar (dbField *fields, int Nfields, int isAverage) {
  int i;
  for (i = 0; i < Nfields; i++) {
    if (isAverage) {
      if (fields[i].ID == AVE_E_BV)             return TRUE;
      if (fields[i].ID == AVE_E_BV_ERR)         return TRUE;
      if (fields[i].ID == AVE_DISTANCE_MOD)     return TRUE;
      if (fields[i].ID == AVE_DISTANCE_MOD_ERR) return TRUE;
      if (fields[i].ID == AVE_M_R)              return TRUE;
      if (fields[i].ID == AVE_M_R_ERR)          return TRUE;
      if (fields[i].ID == AVE_FEH)              return TRUE;
      if (fields[i].ID == AVE_FEH_ERR)          return TRUE;
      if (fields[i].ID == AVE_URA_GALMODEL)     return TRUE;
      if (fields[i].ID == AVE_UDEC_GALMODEL)    return TRUE;
      if (fields[i].ID == AVE_RA_GALMODEL)      return TRUE;
      if (fields[i].ID == AVE_DEC_GALMODEL)     return TRUE;
    } else {
      if (fields[i].ID == MEAS_E_BV)             return TRUE;
      if (fields[i].ID == MEAS_E_BV_ERR)         return TRUE;
      if (fields[i].ID == MEAS_DISTANCE_MOD)     return TRUE;
      if (fields[i].ID == MEAS_DISTANCE_MOD_ERR) return TRUE;
      if (fields[i].ID == MEAS_M_R)              return TRUE;
      if (fields[i].ID == MEAS_M_R_ERR)          return TRUE;
      if (fields[i].ID == MEAS_FEH)              return TRUE;
      if (fields[i].ID == MEAS_FEH_ERR)          return TRUE;
      if (fields[i].ID == MEAS_URA_GALMODEL)     return TRUE;
      if (fields[i].ID == MEAS_UDEC_GALMODEL)    return TRUE;
      if (fields[i].ID == MEAS_RA_GALMODEL)      return TRUE;
      if (fields[i].ID == MEAS_DEC_GALMODEL)     return TRUE;
    }
  }
  return FALSE;
}

int OptionNeedGalphot (dvoMagOptionType magOption) {
  if (magOption == MAG_OPTION_GAL_MAG         ) return TRUE;
  if (magOption == MAG_OPTION_GAL_MAG_ERR     ) return TRUE;
  if (magOption == MAG_OPTION_GAL_MAJ         ) return TRUE;
  if (magOption == MAG_OPTION_GAL_MAJ_ERR     ) return TRUE;
  if (magOption == MAG_OPTION_GAL_MIN         ) return TRUE;
  if (magOption == MAG_OPTION_GAL_MIN_ERR     ) return TRUE;
  if (magOption == MAG_OPTION_GAL_THETA       ) return TRUE;
  if (magOption == MAG_OPTION_GAL_THETA_ERR   ) return TRUE;
  if (magOption == MAG_OPTION_GAL_INDEX       ) return TRUE;
  if (magOption == MAG_OPTION_GAL_CHISQ       ) return TRUE;
  if (magOption == MAG_OPTION_GAL_NPIX        ) return TRUE;
  if (magOption == MAG_OPTION_GAL_FLAGS       ) return TRUE;
  if (magOption == MAG_OPTION_GAL_TYPE        ) return TRUE;

  if (magOption == MAG_OPTION_GAL_OBJ_ID      ) return TRUE;
  if (magOption == MAG_OPTION_GAL_CAT_ID      ) return TRUE;
  if (magOption == MAG_OPTION_GAL_DET_ID      ) return TRUE;
  if (magOption == MAG_OPTION_GAL_IMAGE_ID    ) return TRUE;
  return FALSE;
}

int dbFieldNeedGalphot (dbField *fields, int Nfields) {
  int i;
  for (i = 0; i < Nfields; i++) {
    if (fields[i].magOption == MAG_OPTION_NONE) continue; // non-measure fields do not have a photcode
    if (OptionNeedGalphot (fields[i].magOption)) return TRUE;
  }
  return FALSE;
}

dvoMagSourceType GetMagSource (char *string) {

  if (!strcasecmp (string, "chip"))  return (MAG_SRC_CHP);
  if (!strcasecmp (string, "warp"))  return (MAG_SRC_WRP);
  if (!strcasecmp (string, "stack")) return (MAG_SRC_STK);

  if (!strcasecmp (string, "chp")) return (MAG_SRC_CHP);
  if (!strcasecmp (string, "wrp")) return (MAG_SRC_WRP);
  if (!strcasecmp (string, "stk")) return (MAG_SRC_STK);

  return MAG_SRC_NONE;
}

dvoMagLevelType GetMagLevel (char *string) {

  if (!strcasecmp (string, "inst")) return (MAG_LEVEL_INST);
  if (!strcasecmp (string, "cat"))  return (MAG_LEVEL_CAT);
  if (!strcasecmp (string, "sys"))  return (MAG_LEVEL_SYS);
  if (!strcasecmp (string, "rel"))  return (MAG_LEVEL_REL);
  if (!strcasecmp (string, "cal"))  return (MAG_LEVEL_CAL);
  if (!strcasecmp (string, "ave"))  return (MAG_LEVEL_AVE);
  if (!strcasecmp (string, "ref"))  return (MAG_LEVEL_REF);

  return MAG_LEVEL_NONE;
}

dvoMagClassType GetMagClass (char *string) {

  if (!strcasecmp (string, "psf"))   return (MAG_CLASS_PSF);
  if (!strcasecmp (string, "kron"))  return (MAG_CLASS_KRON);
  if (!strcasecmp (string, "aper"))  return (MAG_CLASS_APER);
  if (!strcasecmp (string, "ap"))    return (MAG_CLASS_APER);

  // these model classes are only valid for galphot mag options
  if (!strcasecmp (string, "dev"))    return (MAG_CLASS_DEV);
  if (!strcasecmp (string, "exp"))    return (MAG_CLASS_EXP);
  if (!strcasecmp (string, "ser"))    return (MAG_CLASS_SER);


  return MAG_CLASS_NONE;
}

dvoMagOptionType GetMagOption (char *string) {

  if (!strcasecmp (string, "mag"))           return MAG_OPTION_MAG;
  if (!strcasecmp (string, "err"))           return MAG_OPTION_ERR;
  if (!strcasecmp (string, "magerr"))        return MAG_OPTION_ERR;
  if (!strcasecmp (string, "flux"))          return MAG_OPTION_FLUX;
  if (!strcasecmp (string, "fluxerr"))       return MAG_OPTION_FLUX_ERR;
  if (!strcasecmp (string, "stdev"))         return MAG_OPTION_STDEV;
  if (!strcasecmp (string, "chisq"))         return MAG_OPTION_CHISQ;
  if (!strcasecmp (string, "min"))           return MAG_OPTION_MIN;
  if (!strcasecmp (string, "max"))           return MAG_OPTION_MAX;
  if (!strcasecmp (string, "ncode"))         return MAG_OPTION_NCODE;
  if (!strcasecmp (string, "nphot"))         return MAG_OPTION_NPHOT;
  if (!strcasecmp (string, "nwarp"))         return MAG_OPTION_NWARP;
  if (!strcasecmp (string, "nwarp_good"))    return MAG_OPTION_NWARP_GOOD;
  if (!strcasecmp (string, "nstack"))        return MAG_OPTION_NSTACK;
  if (!strcasecmp (string, "nstack_det"))    return MAG_OPTION_NSTACK_DET;
  if (!strcasecmp (string, "uc_dist"))       return MAG_OPTION_UC_DIST;
  if (!strcasecmp (string, "flags"))         return MAG_OPTION_FLAGS;
  if (!strcasecmp (string, "psfqf"))         return MAG_OPTION_PSF_QF;
  if (!strcasecmp (string, "psfqfperf"))     return MAG_OPTION_PSF_QF_PERFECT;
  if (!strcasecmp (string, "psfqfperfect"))  return MAG_OPTION_PSF_QF_PERFECT;
  if (!strcasecmp (string, "X11_SM_OBJ"))    return MAG_OPTION_X11_SM_OBJ; 
  if (!strcasecmp (string, "X12_SM_OBJ"))    return MAG_OPTION_X12_SM_OBJ; 
  if (!strcasecmp (string, "X22_SM_OBJ"))    return MAG_OPTION_X22_SM_OBJ; 
  if (!strcasecmp (string, "E1_SM_OBJ"))     return MAG_OPTION_E1_SM_OBJ; 
  if (!strcasecmp (string, "E2_SM_OBJ"))     return MAG_OPTION_E2_SM_OBJ; 
  if (!strcasecmp (string, "X11_SH_OBJ"))    return MAG_OPTION_X11_SH_OBJ; 
  if (!strcasecmp (string, "X12_SH_OBJ"))    return MAG_OPTION_X12_SH_OBJ; 
  if (!strcasecmp (string, "X22_SH_OBJ"))    return MAG_OPTION_X22_SH_OBJ; 
  if (!strcasecmp (string, "E1_SH_OBJ"))     return MAG_OPTION_E1_SH_OBJ; 
  if (!strcasecmp (string, "E2_SH_OBJ"))     return MAG_OPTION_E2_SH_OBJ; 
  if (!strcasecmp (string, "X11_SM_PSF"))    return MAG_OPTION_X11_SM_PSF; 
  if (!strcasecmp (string, "X12_SM_PSF"))    return MAG_OPTION_X12_SM_PSF; 
  if (!strcasecmp (string, "X22_SM_PSF"))    return MAG_OPTION_X22_SM_PSF; 
  if (!strcasecmp (string, "E1_SM_PSF"))     return MAG_OPTION_E1_SM_PSF; 
  if (!strcasecmp (string, "E2_SM_PSF"))     return MAG_OPTION_E2_SM_PSF; 
  if (!strcasecmp (string, "X11_SH_PSF"))    return MAG_OPTION_X11_SH_PSF; 
  if (!strcasecmp (string, "X12_SH_PSF"))    return MAG_OPTION_X12_SH_PSF; 
  if (!strcasecmp (string, "X22_SH_PSF"))    return MAG_OPTION_X22_SH_PSF; 
  if (!strcasecmp (string, "E1_SH_PSF"))     return MAG_OPTION_E1_SH_PSF; 
  if (!strcasecmp (string, "E2_SH_PSF"))     return MAG_OPTION_E2_SH_PSF; 
  if (!strcasecmp (string, "E1_PSF"))        return MAG_OPTION_E1_PSF; 
  if (!strcasecmp (string, "E2_PSF"))        return MAG_OPTION_E2_PSF; 

  if (!strcasecmp (string, "F_AP_R5"))       return MAG_OPTION_F_AP_R5; 
  if (!strcasecmp (string, "F_ERR_AP_R5"))   return MAG_OPTION_F_ERR_AP_R5; 
  if (!strcasecmp (string, "F_STDEV_AP_R5")) return MAG_OPTION_F_STDEV_AP_R5; 
  if (!strcasecmp (string, "F_FILL_AP_R5"))  return MAG_OPTION_F_FILL_AP_R5; 
  if (!strcasecmp (string, "F_AP_R6"))       return MAG_OPTION_F_AP_R6; 
  if (!strcasecmp (string, "F_ERR_AP_R6"))   return MAG_OPTION_F_ERR_AP_R6; 
  if (!strcasecmp (string, "F_STDEV_AP_R6")) return MAG_OPTION_F_STDEV_AP_R6; 
  if (!strcasecmp (string, "F_FILL_AP_R6"))  return MAG_OPTION_F_FILL_AP_R6; 
  if (!strcasecmp (string, "F_AP_R7"))       return MAG_OPTION_F_AP_R7; 
  if (!strcasecmp (string, "F_ERR_AP_R7"))   return MAG_OPTION_F_ERR_AP_R7; 
  if (!strcasecmp (string, "F_STDEV_AP_R7")) return MAG_OPTION_F_STDEV_AP_R7; 
  if (!strcasecmp (string, "F_FILL_AP_R7"))  return MAG_OPTION_F_FILL_AP_R7; 

  if (!strcasecmp (string, "F_AP_R5_C0"))       return MAG_OPTION_F_AP_R5; 
  if (!strcasecmp (string, "F_ERR_AP_R5_C0"))   return MAG_OPTION_F_ERR_AP_R5; 
  if (!strcasecmp (string, "F_STDEV_AP_R5_C0")) return MAG_OPTION_F_STDEV_AP_R5; 
  if (!strcasecmp (string, "F_FILL_AP_R5_C0"))  return MAG_OPTION_F_FILL_AP_R5; 
  if (!strcasecmp (string, "F_AP_R6_C0"))       return MAG_OPTION_F_AP_R6; 
  if (!strcasecmp (string, "F_ERR_AP_R6_C0"))   return MAG_OPTION_F_ERR_AP_R6; 
  if (!strcasecmp (string, "F_STDEV_AP_R6_C0")) return MAG_OPTION_F_STDEV_AP_R6; 
  if (!strcasecmp (string, "F_FILL_AP_R6_C0"))  return MAG_OPTION_F_FILL_AP_R6; 
  if (!strcasecmp (string, "F_AP_R7_C0"))       return MAG_OPTION_F_AP_R7; 
  if (!strcasecmp (string, "F_ERR_AP_R7_C0"))   return MAG_OPTION_F_ERR_AP_R7; 
  if (!strcasecmp (string, "F_STDEV_AP_R7_C0")) return MAG_OPTION_F_STDEV_AP_R7; 
  if (!strcasecmp (string, "F_FILL_AP_R7_C0"))  return MAG_OPTION_F_FILL_AP_R7; 

  if (!strcasecmp (string, "F_AP_R5_C1"))       return MAG_OPTION_X11_SM_OBJ;
  if (!strcasecmp (string, "F_ERR_AP_R5_C1"))   return MAG_OPTION_E1_SM_OBJ; 
  if (!strcasecmp (string, "F_AP_R6_C1"))       return MAG_OPTION_X22_SM_OBJ; 
  if (!strcasecmp (string, "F_ERR_AP_R6_C1"))   return MAG_OPTION_E2_SM_OBJ; 
  if (!strcasecmp (string, "F_AP_R7_C1"))       return MAG_OPTION_X11_SH_OBJ;
  if (!strcasecmp (string, "F_ERR_AP_R7_C1"))   return MAG_OPTION_E2_SH_OBJ; 

  if (!strcasecmp (string, "F_AP_R5_C2"))       return MAG_OPTION_X11_SM_PSF;
  if (!strcasecmp (string, "F_ERR_AP_R5_C2"))   return MAG_OPTION_E1_SM_PSF; 
  if (!strcasecmp (string, "F_AP_R6_C2"))       return MAG_OPTION_X22_SM_PSF; 
  if (!strcasecmp (string, "F_ERR_AP_R6_C2"))   return MAG_OPTION_E2_SM_PSF; 
  if (!strcasecmp (string, "F_AP_R7_C2"))       return MAG_OPTION_X11_SH_PSF;
  if (!strcasecmp (string, "F_ERR_AP_R7_C2"))   return MAG_OPTION_E2_SH_PSF; 

  if (!strcasecmp (string, "E1"))            return MAG_OPTION_E1; 
  if (!strcasecmp (string, "E2"))            return MAG_OPTION_E2; 

  if (!strcasecmp (string, "GAL_MAG"    ))   return MAG_OPTION_GAL_MAG       ;
  if (!strcasecmp (string, "GAL_MAG_ERR"))   return MAG_OPTION_GAL_MAG_ERR   ;
  if (!strcasecmp (string, "GAL_MAJ"    ))   return MAG_OPTION_GAL_MAJ       ;
  if (!strcasecmp (string, "GAL_MAJ_ERR"))   return MAG_OPTION_GAL_MAJ_ERR   ;
  if (!strcasecmp (string, "GAL_MIN"    ))   return MAG_OPTION_GAL_MIN       ;
  if (!strcasecmp (string, "GAL_MIN_ERR"))   return MAG_OPTION_GAL_MIN_ERR   ;
  if (!strcasecmp (string, "GAL_THETA"  ))   return MAG_OPTION_GAL_THETA     ;
  if (!strcasecmp (string, "GAL_THETA_ERR")) return MAG_OPTION_GAL_THETA_ERR ;
  if (!strcasecmp (string, "GAL_INDEX"  ))   return MAG_OPTION_GAL_INDEX     ;
  if (!strcasecmp (string, "GAL_CHISQ"  ))   return MAG_OPTION_GAL_CHISQ     ;
  if (!strcasecmp (string, "GAL_NPIX"   ))   return MAG_OPTION_GAL_NPIX      ;
  if (!strcasecmp (string, "GAL_FLAGS"  ))   return MAG_OPTION_GAL_FLAGS     ;
  if (!strcasecmp (string, "GAL_TYPE"   ))   return MAG_OPTION_GAL_TYPE      ;

  return MAG_OPTION_NONE;
}

// field may be of the form mag:psf:inst:wrp.  except for the first subword, 
// the words may be in any order
int ParsePhotcodeField (dbField *field, char *fieldName, int fieldID) {

  int j;

  // defaults:
  field->ID = fieldID; // change if not true in the end?

  // check what options are provided.  do not allow duplicate types
  // except mag and flux may exist with err
  dvoMagSourceType mySource = MAG_SRC_NONE;
  dvoMagLevelType myLevel = MAG_LEVEL_NONE;
  dvoMagClassType myClass = MAG_CLASS_NONE;
  dvoMagOptionType myOption = MAG_OPTION_NONE;

  field->magSource = MAG_SRC_CHP;
  field->magLevel  = MAG_LEVEL_AVE;
  field->magClass  = MAG_CLASS_PSF;
  field->magOption = MAG_OPTION_MAG;

  // make a local working copy of fieldName and replace ':' with spaces
  // XXX memory leak here:
  char *fieldCopy = strcreate(fieldName);
  for (j = 0; fieldCopy[j]; j++) { 
    if (fieldCopy[j] == ':') fieldCopy[j] = ' '; 
  }

  // XXX potential memory leak here
  char *firstWord = getword(fieldCopy);
  if (!firstWord) return FALSE;
  
  // firstWord may be either flux, mag or photcode
  if (!strcasecmp (firstWord, "MAG")) {
    // default level for 'mag' is rel
    field->magLevel  = MAG_LEVEL_REL;
    myOption = MAG_OPTION_MAG;
  } 
  if (!strcasecmp (firstWord, "FLUX")) {
    // default level for 'flux' is rel
    field->magLevel  = MAG_LEVEL_REL;
    myOption = MAG_OPTION_FLUX;
  } 
  PhotCode *code = GetPhotcodebyName (firstWord);
  free (firstWord);

  if (!code) return FALSE;

  // defaults for photcode clases
  if (code->type == PHOT_DEP) {
    field->magLevel = MAG_LEVEL_REL;
  }
  if (code->type == PHOT_SEC) {
    field->magLevel = MAG_LEVEL_AVE;
  }
  if (code->type == PHOT_REF) {
    field->magLevel = MAG_LEVEL_CAT;
    // is this one required?
  }
  
  // look at the remaining words and assign properties as approrpiate
  char *ptr = skipword(fieldCopy);
  while (ptr) {

    char *word = getword(ptr);
    if (!word) break;
    
    dvoMagSourceType source = GetMagSource (word);
    if (source != MAG_SRC_NONE) {
      // if mySource is already set, then we have two 'source' elements (an error)
      if (mySource != MAG_SRC_NONE) { fprintf (stderr, "invalid selection %s in %s\n", word, fieldName); free (word); return FALSE; } 
      mySource = source;
      ptr = skipword (ptr);
      continue;
    }

    dvoMagLevelType level = GetMagLevel (word);
    if (level != MAG_LEVEL_NONE) {
      // if myLevel is already set, then we have two 'level' elements (an error)
      if (myLevel != MAG_LEVEL_NONE) { fprintf (stderr, "invalid selection %s in %s\n", word, fieldName); free (word); return FALSE; } 
      myLevel = level;
      ptr = skipword (ptr);
      continue;
    }

    dvoMagClassType class = GetMagClass (word);
    if (class != MAG_CLASS_NONE) {
      // if myClass is already set, then we have two 'class' elements (an error)
      if (myClass != MAG_CLASS_NONE) { fprintf (stderr, "invalid selection %s in %s\n", word, fieldName); free (word); return FALSE; } 
      myClass = class;
      ptr = skipword (ptr);
      continue;
    }

    dvoMagOptionType option = GetMagOption (word);
    if (option != MAG_OPTION_NONE) {
      // if myClass is already set, then we have two 'class' elements
      // this is an error unless the request was of the form mag:value
      // if so, then code->type will be PHOT_MAG

      if (code->type == PHOT_MAG) {
	// mag:err -> magerr
	if ((myOption == MAG_OPTION_MAG) && (option == MAG_OPTION_ERR)) {
	  myOption = MAG_OPTION_ERR;
	  ptr = skipword (ptr);
	  continue;
	} 
	// flux:err -> fluxerr
	if ((myOption == MAG_OPTION_FLUX) && (option == MAG_OPTION_ERR)) {
	  myOption = MAG_OPTION_FLUX_ERR;
	  ptr = skipword (ptr);
	  continue;
	} 
	myOption = option;
	ptr = skipword (ptr);
	continue;
      }
      if (myOption != MAG_OPTION_NONE) { fprintf (stderr, "invalid selection %s in %s\n", word, fieldName); free (word); return FALSE; } 
      myOption = option;
      ptr = skipword (ptr);
      continue;
    }

    fprintf (stderr, "ERROR: unknown mag/photocode argument %s\n", word);
    free (word);
    return FALSE;
  }

  // check if we are asking for a galphot value:
  if (OptionNeedGalphot (myOption)) {
    field->magClass = MAG_CLASS_SER; // for galphot, we need to set a default model class (SERSIC)
  }

  // if nothing is provided, use the defaults
  field->magSource = (mySource == MAG_SRC_NONE)    ? field->magSource : mySource;
  field->magLevel  = (myLevel  == MAG_LEVEL_NONE)  ? field->magLevel  : myLevel;
  field->magClass  = (myClass  == MAG_CLASS_NONE)  ? field->magClass  : myClass;
  field->magOption = (myOption == MAG_OPTION_NONE) ? field->magOption : myOption;

  switch (field->magOption) {
    case MAG_OPTION_NWARP:
    case MAG_OPTION_NWARP_GOOD:
    case MAG_OPTION_NSTACK:
    case MAG_OPTION_NSTACK_DET:
    case MAG_OPTION_NCODE:
    case MAG_OPTION_NPHOT:
    case MAG_OPTION_FLAGS:
      field->type = OPIHI_INT;
      break;
    default:
      field->type = OPIHI_FLT;
      break;
  }
  field->photcode = code;
  return TRUE;
}

// field needs to be initialized with dbInitField()
# define ESCAPE(F,T) { \
  field->ID = (F); \
  field->type = (T); \
  return (TRUE); }

int ParseMeasureField (dbField *field, char *fieldName) {

  field->table = DVO_TABLE_MEASURE;
  field->name  = strcreate (fieldName);

  if (!strcasecmp (fieldName, "GLON"))       {
    dbExtractMeasuresInitTransform (COORD_GALACTIC);
    ESCAPE (MEAS_GLON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "GLAT")) {
    dbExtractMeasuresInitTransform (COORD_GALACTIC);
    ESCAPE (MEAS_GLAT, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "GLON:AVE")) {
    dbExtractMeasuresInitTransform (COORD_GALACTIC);
    ESCAPE (MEAS_GLON_AVE, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "GLAT:AVE")) {
    dbExtractMeasuresInitTransform (COORD_GALACTIC);
    ESCAPE (MEAS_GLAT_AVE, OPIHI_FLT);
  }

  if (!strcasecmp (fieldName, "ELON"))       {
    dbExtractMeasuresInitTransform (COORD_ECLIPTIC);
    ESCAPE (MEAS_ELON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "ELAT")) {
    dbExtractMeasuresInitTransform (COORD_ECLIPTIC);
    ESCAPE (MEAS_ELAT, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "ELON:AVE")) {
    dbExtractMeasuresInitTransform (COORD_ECLIPTIC);
    ESCAPE (MEAS_ELON_AVE, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "ELAT:AVE")) {
    dbExtractMeasuresInitTransform (COORD_ECLIPTIC);
    ESCAPE (MEAS_ELAT_AVE, OPIHI_FLT);
  }

  if (!strcasecmp (fieldName, "RA"))         	 ESCAPE (MEAS_RA,             OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC"))        	 ESCAPE (MEAS_DEC,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "RA:AVE"))     	 ESCAPE (MEAS_RA_AVE,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC:AVE"))    	 ESCAPE (MEAS_DEC_AVE,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "RA:ERR"))     	 ESCAPE (MEAS_RA_AVE_ERR,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC:ERR"))    	 ESCAPE (MEAS_DEC_AVE_ERR,    OPIHI_FLT);
  if (!strcasecmp (fieldName, "uRA"))        	 ESCAPE (MEAS_U_RA,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "uDEC"))       	 ESCAPE (MEAS_U_DEC,          OPIHI_FLT);
  if (!strcasecmp (fieldName, "duRA"))       	 ESCAPE (MEAS_U_RA_ERR,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "duDEC"))      	 ESCAPE (MEAS_U_DEC_ERR,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "PAR"))        	 ESCAPE (MEAS_PAR,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "dPAR"))       	 ESCAPE (MEAS_PAR_ERR,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "dR"))         	 ESCAPE (MEAS_RA_OFFSET,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "dD"))         	 ESCAPE (MEAS_DEC_OFFSET,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "dR:FIT"))     	 ESCAPE (MEAS_RA_FIT_OFFSET,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "dD:FIT"))     	 ESCAPE (MEAS_DEC_FIT_OFFSET, OPIHI_FLT);
  if (!strcasecmp (fieldName, "dR:ERR"))     	 ESCAPE (MEAS_RA_OFFSET_ERR,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "dD:ERR"))     	 ESCAPE (MEAS_DEC_OFFSET_ERR, OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPos"))       ESCAPE (MEAS_CHISQ_POS,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPM"))        ESCAPE (MEAS_CHISQ_PM,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPar"))       ESCAPE (MEAS_CHISQ_PAR,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "Tmean"))          ESCAPE (MEAS_TMEAN,          OPIHI_FLT);
  if (!strcasecmp (fieldName, "Trange"))         ESCAPE (MEAS_TRANGE,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "nmeas"))      	 ESCAPE (MEAS_NMEAS,          OPIHI_INT);
  if (!strcasecmp (fieldName, "nmiss"))      	 ESCAPE (MEAS_NMISS,          OPIHI_INT);
  if (!strcasecmp (fieldName, "npos"))      	 ESCAPE (MEAS_NPOS,           OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJFLAGS"))   	 ESCAPE (MEAS_OBJ_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJ_FLAGS"))   	 ESCAPE (MEAS_OBJ_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJ_PHOT_FLAGS")) ESCAPE (MEAS_SECFILT_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "SECFLAGS"))   	 ESCAPE (MEAS_SECFILT_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "SEC_FLAGS"))   	 ESCAPE (MEAS_SECFILT_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "SECFILT_FLAGS"))  ESCAPE (MEAS_SECFILT_FLAGS,      OPIHI_INT);
  if (!strcasecmp (fieldName, "DB_FLAGS"))   	 ESCAPE (MEAS_DB_FLAGS,       OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS")) 	 ESCAPE (MEAS_PHOT_FLAGS,     OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS2")) 	 ESCAPE (MEAS_PHOT_FLAGS2,     OPIHI_INT);
  if (!strcasecmp (fieldName, "DBFLAGS"))   	 ESCAPE (MEAS_DB_FLAGS,       OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS")) 	 ESCAPE (MEAS_PHOT_FLAGS,     OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS2")) 	 ESCAPE (MEAS_PHOT_FLAGS2,    OPIHI_INT);
  if (!strcasecmp (fieldName, "AIRMASS"))    	 ESCAPE (MEAS_AIRMASS,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "MEAN_AIRMASS"))   ESCAPE (MEAS_MEAN_AIRMASS,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "ALT"))        	 ESCAPE (MEAS_ALT,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "AZ"))         	 ESCAPE (MEAS_AZ,             OPIHI_FLT);
  if (!strcasecmp (fieldName, "EXPTIME"))    	 ESCAPE (MEAS_EXPTIME,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "PHOTCODE"))   	 ESCAPE (MEAS_PHOTCODE,       OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTCODE:KLAM"))  ESCAPE (MEAS_PHOTCODE_KLAM,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "PHOTCODE:C"))     ESCAPE (MEAS_PHOTCODE_C,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "PHOTCODE:EQUIV")) ESCAPE (MEAS_PHOTCODE_EQUIV, OPIHI_INT);
  if (!strcasecmp (fieldName, "TIME"))       	 ESCAPE (MEAS_TIME,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM"))       	 ESCAPE (MEAS_FWHM,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MAJ"))   	 ESCAPE (MEAS_FWHM_MAJ,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MIN"))   	 ESCAPE (MEAS_FWHM_MIN,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "THETA"))      	 ESCAPE (MEAS_THETA,          OPIHI_FLT);
  if (!strcasecmp (fieldName, "POSANGLE"))     	 ESCAPE (MEAS_POSANGLE,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "PLATESCALE"))   	 ESCAPE (MEAS_PLATESCALE,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "MXX"))       	 ESCAPE (MEAS_MXX,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "MYY"))       	 ESCAPE (MEAS_MYY,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "MXY"))       	 ESCAPE (MEAS_MXY,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "DOPHOT"))     	 ESCAPE (MEAS_DOPHOT,         OPIHI_INT);
  if (!strcasecmp (fieldName, "XCCD"))       	 ESCAPE (MEAS_XCCD,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "YCCD"))       	 ESCAPE (MEAS_YCCD,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "XCCD:ERR"))   	 ESCAPE (MEAS_XCCD_ERR,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "YCCD:ERR"))   	 ESCAPE (MEAS_YCCD_ERR,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "XFIX"))       	 ESCAPE (MEAS_XFIX,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "YFIX"))       	 ESCAPE (MEAS_YFIX,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "XOFF_KH"))      	 ESCAPE (MEAS_XOFF_KH,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "YOFF_KH"))      	 ESCAPE (MEAS_YOFF_KH,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "XOFF_DCR"))     	 ESCAPE (MEAS_XOFF_DCR,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "YOFF_DCR"))     	 ESCAPE (MEAS_YOFF_DCR,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "XOFF_CAM"))     	 ESCAPE (MEAS_XOFF_CAM,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "YOFF_CAM"))     	 ESCAPE (MEAS_YOFF_CAM,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "ROFF_GAL"))       ESCAPE (MEAS_ROFF_GAL,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "DOFF_GAL"))       ESCAPE (MEAS_DOFF_GAL,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "POS_SYS_ERR"))    ESCAPE (MEAS_POS_SYS_ERR,    OPIHI_FLT);
  if (!strcasecmp (fieldName, "XFIELD"))    	 ESCAPE (MEAS_XFIELD,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "YFIELD"))    	 ESCAPE (MEAS_YFIELD,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "XMOSAIC"))    	 ESCAPE (MEAS_XMOSAIC,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "YMOSAIC"))    	 ESCAPE (MEAS_YMOSAIC,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "XCHIP"))      	 ESCAPE (MEAS_XCCD,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "YCHIP"))      	 ESCAPE (MEAS_YCCD,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "XFPA"))       	 ESCAPE (MEAS_XMOSAIC,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "YFPA"))       	 ESCAPE (MEAS_YMOSAIC,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "DETID"))      	 ESCAPE (MEAS_DET_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJID"))      	 ESCAPE (MEAS_OBJ_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "CATID"))      	 ESCAPE (MEAS_CAT_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "IMAGEID"))    	 ESCAPE (MEAS_IMAGE_ID,       OPIHI_INT);
  if (!strcasecmp (fieldName, "IMAGE_EXTERN_ID")) ESCAPE (MEAS_IMAGE_EXTERN_ID, OPIHI_INT);
  if (!strcasecmp (fieldName, "IMAGE_EXT_ID"))    ESCAPE (MEAS_IMAGE_EXTERN_ID, OPIHI_INT);
  if (!strcasecmp (fieldName, "EXTERNID"))    	 ESCAPE (MEAS_EXT_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "EXT_ID"))    	 ESCAPE (MEAS_EXT_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "EXTID"))    	 ESCAPE (MEAS_EXT_ID,         OPIHI_INT);
  if (!strcasecmp (fieldName, "EXPNAME"))    	 ESCAPE (MEAS_EXPNAME_AS_INT, OPIHI_INT);
  if (!strcasecmp (fieldName, "PSF_QF"))     	 ESCAPE (MEAS_PSF_QF,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSF_QF_PERFECT")) ESCAPE (MEAS_PSF_QF_PERFECT, OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSFQF"))     	 ESCAPE (MEAS_PSF_QF,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSFQF_PERFECT"))  ESCAPE (MEAS_PSF_QF_PERFECT, OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSFQFPERFECT"))   ESCAPE (MEAS_PSF_QF_PERFECT, OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSF_CHISQ"))  	 ESCAPE (MEAS_PSF_CHISQ,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "PSF_NDOF"))  	 ESCAPE (MEAS_PSF_NDOF,       OPIHI_INT);
  if (!strcasecmp (fieldName, "PSF_NPIX"))  	 ESCAPE (MEAS_PSF_NPIX,       OPIHI_INT);
  if (!strcasecmp (fieldName, "CR_NSIGMA"))  	 ESCAPE (MEAS_CR_NSIGMA,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "EXT_NSIGMA")) 	 ESCAPE (MEAS_EXT_NSIGMA,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "SKY"))        	 ESCAPE (MEAS_SKY,            OPIHI_FLT);
  if (!strcasecmp (fieldName, "SKY_ERR"))    	 ESCAPE (MEAS_dSKY,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "MCAL_OFFSET"))    ESCAPE (MEAS_MCAL_OFFSET_PSF,    OPIHI_FLT);
  if (!strcasecmp (fieldName, "MCAL_OFFSET_PSF"))  ESCAPE (MEAS_MCAL_OFFSET_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "MCAL_OFFSET_APER")) ESCAPE (MEAS_MCAL_OFFSET_APER, OPIHI_FLT);
  if (!strcasecmp (fieldName, "FLAT"))    	 ESCAPE (MEAS_FLAT,           OPIHI_FLT);
  if (!strcasecmp (fieldName, "CENTER_OFFSET"))  ESCAPE (MEAS_CENTER_OFFSET,  OPIHI_FLT);

  // individual lensing measurements are not grouped by photcode:
  if (!strcasecmp (fieldName, "X11_SM_OBJ"))     ESCAPE (MEAS_X11_SM_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X12_SM_OBJ"))     ESCAPE (MEAS_X12_SM_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X22_SM_OBJ"))     ESCAPE (MEAS_X22_SM_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E1_SM_OBJ"))      ESCAPE (MEAS_E1_SM_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E2_SM_OBJ"))      ESCAPE (MEAS_E2_SM_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X11_SH_OBJ"))     ESCAPE (MEAS_X11_SH_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X12_SH_OBJ"))     ESCAPE (MEAS_X12_SH_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X22_SH_OBJ"))     ESCAPE (MEAS_X22_SH_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E1_SH_OBJ"))      ESCAPE (MEAS_E1_SH_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E2_SH_OBJ"))      ESCAPE (MEAS_E2_SH_OBJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X11_SM_PSF"))     ESCAPE (MEAS_X11_SM_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X12_SM_PSF"))     ESCAPE (MEAS_X12_SM_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X22_SM_PSF"))     ESCAPE (MEAS_X22_SM_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E1_SM_PSF"))      ESCAPE (MEAS_E1_SM_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E2_SM_PSF"))      ESCAPE (MEAS_E2_SM_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X11_SH_PSF"))     ESCAPE (MEAS_X11_SH_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X12_SH_PSF"))     ESCAPE (MEAS_X12_SH_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "X22_SH_PSF"))     ESCAPE (MEAS_X22_SH_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E1_SH_PSF"))      ESCAPE (MEAS_E1_SH_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E2_SH_PSF"))      ESCAPE (MEAS_E2_SH_PSF,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "E1_PSF"))         ESCAPE (MEAS_E1_PSF,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "E2_PSF"))         ESCAPE (MEAS_E2_PSF,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "F_AP_R5"))        ESCAPE (MEAS_F_AP_R5,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R5"))    ESCAPE (MEAS_F_ERR_AP_R5,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R5"))  ESCAPE (MEAS_F_STDEV_AP_R5, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R5"))   ESCAPE (MEAS_F_FILL_AP_R5,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R6"))        ESCAPE (MEAS_F_AP_R6,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R6"))    ESCAPE (MEAS_F_ERR_AP_R6,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R6"))  ESCAPE (MEAS_F_STDEV_AP_R6, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R6"))   ESCAPE (MEAS_F_FILL_AP_R6,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R7"))        ESCAPE (MEAS_F_AP_R7,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R7"))    ESCAPE (MEAS_F_ERR_AP_R7,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R7"))  ESCAPE (MEAS_F_STDEV_AP_R7, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R7"))   ESCAPE (MEAS_F_FILL_AP_R7,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "F_AP_R5_C0"))        ESCAPE (MEAS_F_AP_R5,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R5_C0"))    ESCAPE (MEAS_F_ERR_AP_R5,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R5_C0"))  ESCAPE (MEAS_F_STDEV_AP_R5, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R5_C0"))   ESCAPE (MEAS_F_FILL_AP_R5,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R6_C0"))        ESCAPE (MEAS_F_AP_R6,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R6_C0"))    ESCAPE (MEAS_F_ERR_AP_R6,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R6_C0"))  ESCAPE (MEAS_F_STDEV_AP_R6, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R6_C0"))   ESCAPE (MEAS_F_FILL_AP_R6,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R7_C0"))        ESCAPE (MEAS_F_AP_R7,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R7_C0"))    ESCAPE (MEAS_F_ERR_AP_R7,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_STDEV_AP_R7_C0"))  ESCAPE (MEAS_F_STDEV_AP_R7, OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_FILL_AP_R7_C0"))   ESCAPE (MEAS_F_FILL_AP_R7,  OPIHI_FLT);

  // these names are overloaded (used for e.g., UNIONS DR3)
  if (!strcasecmp (fieldName, "F_AP_R5_C1"))        ESCAPE (MEAS_X11_SM_OBJ,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R5_C1"))    ESCAPE (MEAS_E1_SM_OBJ,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R6_C1"))        ESCAPE (MEAS_X22_SM_OBJ,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R6_C1"))    ESCAPE (MEAS_E2_SM_OBJ,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R7_C1"))        ESCAPE (MEAS_X11_SH_OBJ,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R7_C1"))    ESCAPE (MEAS_E2_SH_OBJ,       OPIHI_FLT);

  // these names are overloaded (used for e.g., UNIONS DR3)
  if (!strcasecmp (fieldName, "F_AP_R5_C2"))        ESCAPE (MEAS_X11_SM_PSF,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R5_C2"))    ESCAPE (MEAS_E1_SM_PSF,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R6_C2"))        ESCAPE (MEAS_X22_SM_PSF,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R6_C2"))    ESCAPE (MEAS_E2_SM_PSF,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_AP_R7_C2"))        ESCAPE (MEAS_X11_SH_PSF,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "F_ERR_AP_R7_C2"))    ESCAPE (MEAS_E2_SH_PSF,       OPIHI_FLT);

  if (!strcasecmp (fieldName, "E_BV"))             ESCAPE (MEAS_E_BV          ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "E_BV_ERR"))         ESCAPE (MEAS_E_BV_ERR      ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DISTANCE_MOD"))     ESCAPE (MEAS_DISTANCE_MOD  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DISTANCE_MOD_ERR")) ESCAPE (MEAS_DISTANCE_MOD_ERR, OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_M"))     	   ESCAPE (MEAS_DISTANCE_MOD  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_M_ERR")) 	   ESCAPE (MEAS_DISTANCE_MOD_ERR, OPIHI_FLT);
  if (!strcasecmp (fieldName, "M_R"))              ESCAPE (MEAS_M_R           ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "M_R_ERR"))          ESCAPE (MEAS_M_R_ERR       ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "FEH"))              ESCAPE (MEAS_FEH           ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "FEH_ERR"))          ESCAPE (MEAS_FEH_ERR       ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "URA_GALMODEL"))     ESCAPE (MEAS_URA_GALMODEL  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "UDEC_GALMODEL"))    ESCAPE (MEAS_UDEC_GALMODEL ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "RA_GALMODEL"))      ESCAPE (MEAS_RA_GALMODEL   ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC_GALMODEL"))     ESCAPE (MEAS_DEC_GALMODEL  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "URA_GAL"))     	   ESCAPE (MEAS_URA_GALMODEL  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "UDEC_GAL"))    	   ESCAPE (MEAS_UDEC_GALMODEL ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "RA_GAL"))      	   ESCAPE (MEAS_RA_GALMODEL   ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC_GAL"))     	   ESCAPE (MEAS_DEC_GALMODEL  ,   OPIHI_FLT);

  // if (!strcasecmp (fieldName, "FLUX"))           ESCAPE (MEAS_FLUX_PSF,       OPIHI_FLT);
  // if (!strcasecmp (fieldName, "FLUX_ERR"))       ESCAPE (MEAS_FLUX_PSF_ERR,   OPIHI_FLT);
  // if (!strcasecmp (fieldName, "FLUX_PSF"))       ESCAPE (MEAS_FLUX_PSF,       OPIHI_FLT);
  // if (!strcasecmp (fieldName, "FLUX_PSF_ERR"))   ESCAPE (MEAS_FLUX_PSF_ERR,   OPIHI_FLT);
  // if (!strcasecmp (fieldName, "FLUX_KRON"))      ESCAPE (MEAS_FLUX_KRON,      OPIHI_FLT);
  // if (!strcasecmp (fieldName, "FLUX_KRON_ERR"))  ESCAPE (MEAS_FLUX_KRON_ERR,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "REF_COLOR_BLUE"))    ESCAPE (MEAS_REF_COLOR_BLUE, OPIHI_FLT);
  if (!strcasecmp (fieldName, "REF_COLOR_RED"))     ESCAPE (MEAS_REF_COLOR_RED,  OPIHI_FLT);

  // for words that don't parse, try a photcode

  // check for code:mode in photcode name 
  if (!ParsePhotcodeField (field, fieldName, MEAS_PHOT)) {
    gprint (GP_ERR, "unknown field '%s' for measurement table in DVO database\n", fieldName);
    return (FALSE);
  }

  return (TRUE);
}

int ParseAverageField (dbField *field, char *fieldName) {

  field->table = DVO_TABLE_AVERAGE;
  field->name  = strcreate (fieldName);

  // if either GLON or GLAT is requested, we set up a static tranformation 
  // at prepare to calculate the values only once for each row
  if (!strcasecmp (fieldName, "GLON"))  { 
    dbExtractAveragesInitTransform (COORD_GALACTIC);
    ESCAPE (AVE_GLON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "GLAT"))  {
    dbExtractAveragesInitTransform (COORD_GALACTIC);
    ESCAPE (AVE_GLAT, OPIHI_FLT);
  }

  // if either ELON or ELAT is requested, we set up a static tranformation 
  // at prepare to calculate the values only once for each row
  if (!strcasecmp (fieldName, "ELON"))  { 
    dbExtractAveragesInitTransform (COORD_ECLIPTIC);
    ESCAPE (AVE_ELON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "ELAT"))  {
    dbExtractAveragesInitTransform (COORD_ECLIPTIC);
    ESCAPE (AVE_ELAT, OPIHI_FLT);
  }

  if (!strcasecmp (fieldName, "RA"))          ESCAPE (AVE_RA,          OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC"))         ESCAPE (AVE_DEC,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "GLON"))        ESCAPE (AVE_GLON,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "GLAT"))        ESCAPE (AVE_GLAT,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "ELON"))        ESCAPE (AVE_ELON,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "ELAT"))        ESCAPE (AVE_ELAT,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "dRA"))         ESCAPE (AVE_RA_ERR,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "dDEC"))        ESCAPE (AVE_DEC_ERR,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "uRA"))         ESCAPE (AVE_U_RA,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "uDEC"))        ESCAPE (AVE_U_DEC,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "duRA"))        ESCAPE (AVE_U_RA_ERR,    OPIHI_FLT);
  if (!strcasecmp (fieldName, "duDEC"))       ESCAPE (AVE_U_DEC_ERR,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "PAR"))         ESCAPE (AVE_PAR,         OPIHI_FLT);
  if (!strcasecmp (fieldName, "dPAR"))        ESCAPE (AVE_PAR_ERR,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPos"))    ESCAPE (AVE_CHISQ_POS,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPM"))     ESCAPE (AVE_CHISQ_PM,    OPIHI_FLT);
  if (!strcasecmp (fieldName, "ChiSqPar"))    ESCAPE (AVE_CHISQ_PAR,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "Tmean"))       ESCAPE (AVE_TMEAN,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "Trange"))      ESCAPE (AVE_TRANGE,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "psfqf"))       ESCAPE (AVE_PSF_QF,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "psfqfperf"))   ESCAPE (AVE_PSF_QF_PERF, OPIHI_FLT);
  if (!strcasecmp (fieldName, "psf_qf"))      ESCAPE (AVE_PSF_QF,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "psf_qf_perf")) ESCAPE (AVE_PSF_QF_PERF, OPIHI_FLT);
  if (!strcasecmp (fieldName, "stargal"))     ESCAPE (AVE_STARGAL,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "NMEAS"))       ESCAPE (AVE_NMEAS,       OPIHI_INT);
  if (!strcasecmp (fieldName, "NMISS"))       ESCAPE (AVE_NMISS,       OPIHI_INT);
  if (!strcasecmp (fieldName, "NLENSMEAS"))   ESCAPE (AVE_NLENSING,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NLENSING"))    ESCAPE (AVE_NLENSING,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NLENSOBJ"))    ESCAPE (AVE_NLENSOBJ,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NSTARPAR"))    ESCAPE (AVE_NSTARPAR,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NGALPHOT"))    ESCAPE (AVE_NGALPHOT,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NPOS"))        ESCAPE (AVE_NPOS,        OPIHI_INT);
  if (!strcasecmp (fieldName, "NASTROM"))     ESCAPE (AVE_NPOS,        OPIHI_INT);
  if (!strcasecmp (fieldName, "NWARP_OK"))    ESCAPE (AVE_NWARP_OK,    OPIHI_INT);
  if (!strcasecmp (fieldName, "FLAGS"))       ESCAPE (AVE_OBJ_FLAGS,   OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJ_FLAGS"))   ESCAPE (AVE_OBJ_FLAGS,   OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJFLAGS"))    ESCAPE (AVE_OBJ_FLAGS,   OPIHI_INT);
  if (!strcasecmp (fieldName, "OBJID"))       ESCAPE (AVE_OBJID,       OPIHI_INT);
  if (!strcasecmp (fieldName, "CATID"))       ESCAPE (AVE_CATID,       OPIHI_INT);
  if (!strcasecmp (fieldName, "EXTID_HI"))    ESCAPE (AVE_EXTID_HI,    OPIHI_INT);
  if (!strcasecmp (fieldName, "EXTID_LO"))    ESCAPE (AVE_EXTID_LO,    OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS_HI"))     ESCAPE (AVE_PHOT_FLAGS_HI, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS_LO"))     ESCAPE (AVE_PHOT_FLAGS_LO, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS_HI"))    ESCAPE (AVE_PHOT_FLAGS_HI, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS_LO"))    ESCAPE (AVE_PHOT_FLAGS_LO, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS_UPPER"))  ESCAPE (AVE_PHOT_FLAGS_HI, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOTFLAGS_LOWER"))  ESCAPE (AVE_PHOT_FLAGS_LO, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS_UPPER")) ESCAPE (AVE_PHOT_FLAGS_HI, OPIHI_INT);
  if (!strcasecmp (fieldName, "PHOT_FLAGS_LOWER")) ESCAPE (AVE_PHOT_FLAGS_LO, OPIHI_INT);
  if (!strcasecmp (fieldName, "REF_COLOR_BLUE"))   ESCAPE (AVE_REF_COLOR_BLUE, OPIHI_FLT);
  if (!strcasecmp (fieldName, "REF_COLOR_RED"))    ESCAPE (AVE_REF_COLOR_RED,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "E_BV"))             ESCAPE (AVE_E_BV          ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "E_BV_ERR"))         ESCAPE (AVE_E_BV_ERR      ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DISTANCE_MOD"))     ESCAPE (AVE_DISTANCE_MOD  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DISTANCE_MOD_ERR")) ESCAPE (AVE_DISTANCE_MOD_ERR, OPIHI_FLT);
  if (!strcasecmp (fieldName, "M_R"))              ESCAPE (AVE_M_R           ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "M_R_ERR"))          ESCAPE (AVE_M_R_ERR       ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "FEH"))              ESCAPE (AVE_FEH           ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "FEH_ERR"))          ESCAPE (AVE_FEH_ERR       ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "URA_GALMODEL"))     ESCAPE (AVE_URA_GALMODEL  ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "UDEC_GALMODEL"))    ESCAPE (AVE_UDEC_GALMODEL ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "RA_GALMODEL"))      ESCAPE (AVE_RA_GALMODEL   ,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC_GALMODEL"))     ESCAPE (AVE_DEC_GALMODEL  ,   OPIHI_FLT);


  // check for code:mode in photcode name 
  if (!ParsePhotcodeField (field, fieldName, AVE_PHOT)) {
    gprint (GP_ERR, "unknown field '%s' for average table in DVO database\n", fieldName);
    return (FALSE);
  }

  if (field->photcode->type == PHOT_MAG) {
    gprint (GP_ERR, "'mag' is ambiguous for avextract\n");
    return (FALSE);
  }

  return (TRUE);
}


int ParseImageField (dbField *field, char *fieldName) {

  field->table = DVO_TABLE_IMAGE;
  field->name  = strcreate (fieldName);

  // if either GLON or GLAT is requested, we set up a static tranformation 
  // at prepare to calculate the values only once for each row
  if (!strcasecmp (fieldName, "GLON"))  { 
    dbExtractImagesInitTransform (COORD_GALACTIC);
    ESCAPE (IMAGE_GLON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "GLAT"))  {
    dbExtractImagesInitTransform (COORD_GALACTIC);
    ESCAPE (IMAGE_GLAT, OPIHI_FLT);
  }

  // if either ELON or ELAT is requested, we set up a static tranformation 
  // at prepare to calculate the values only once for each row
  if (!strcasecmp (fieldName, "ELON"))  { 
    dbExtractImagesInitTransform (COORD_ECLIPTIC);
    ESCAPE (IMAGE_ELON, OPIHI_FLT);
  }
  if (!strcasecmp (fieldName, "ELAT"))  {
    dbExtractImagesInitTransform (COORD_ECLIPTIC);
    ESCAPE (IMAGE_ELAT, OPIHI_FLT);
  }

  if (!strcasecmp (fieldName, "RA"       )) ESCAPE (IMAGE_RA,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "DEC"      )) ESCAPE (IMAGE_DEC,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "GLON"     )) ESCAPE (IMAGE_GLON,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "GLAT"     )) ESCAPE (IMAGE_GLAT,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "ELON"     )) ESCAPE (IMAGE_ELON,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "ELAT"     )) ESCAPE (IMAGE_ELAT,      OPIHI_FLT);

  if (!strcasecmp (fieldName, "theta"    )) ESCAPE (IMAGE_THETA,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "skew"     )) ESCAPE (IMAGE_SKEW,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "scale"    )) ESCAPE (IMAGE_SCALE,     OPIHI_FLT);
  if (!strcasecmp (fieldName, "dscale"   )) ESCAPE (IMAGE_DSCALE,    OPIHI_FLT);

  if (!strcasecmp (fieldName, "time"     )) ESCAPE (IMAGE_TIME,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "nstar"    )) ESCAPE (IMAGE_NSTAR,     OPIHI_INT);
  if (!strcasecmp (fieldName, "airmass"  )) ESCAPE (IMAGE_AIRMASS,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "NX"       )) ESCAPE (IMAGE_NX_PIX,    OPIHI_INT);
  if (!strcasecmp (fieldName, "NY"       )) ESCAPE (IMAGE_NY_PIX,    OPIHI_INT);
  if (!strcasecmp (fieldName, "apresid"  )) ESCAPE (IMAGE_APRESID,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "dapresid" )) ESCAPE (IMAGE_DAPRESID,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "Mcal"        )) ESCAPE (IMAGE_MCAL_PSF,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "McalPSF"     )) ESCAPE (IMAGE_MCAL_PSF,       OPIHI_FLT);
  if (!strcasecmp (fieldName, "McalAPER"    )) ESCAPE (IMAGE_MCAL_APER,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "McalAPERTURE")) ESCAPE (IMAGE_MCAL_APER,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "dMcal"       )) ESCAPE (IMAGE_dMCAL,          OPIHI_FLT);
  if (!strcasecmp (fieldName, "Xm"          )) ESCAPE (IMAGE_XM,             OPIHI_FLT);
  if (!strcasecmp (fieldName, "photcode"    )) ESCAPE (IMAGE_PHOTCODE,       OPIHI_INT);
  if (!strcasecmp (fieldName, "exptime"     )) ESCAPE (IMAGE_EXPTIME,        OPIHI_FLT);
  if (!strcasecmp (fieldName, "expname"     )) ESCAPE (IMAGE_EXPNAME_AS_INT, OPIHI_INT);
  if (!strcasecmp (fieldName, "sidtime"     )) ESCAPE (IMAGE_SIDTIME,        OPIHI_FLT);

  if (!strcasecmp (fieldName, "latitude" )) ESCAPE (IMAGE_LATITUDE,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "detlimit" )) ESCAPE (IMAGE_DET_LIMIT, OPIHI_FLT);
  if (!strcasecmp (fieldName, "satlimit" )) ESCAPE (IMAGE_SAT_LIMIT, OPIHI_FLT);
  if (!strcasecmp (fieldName, "cerror"   )) ESCAPE (IMAGE_CERROR,    OPIHI_FLT);

  if (!strcasecmp (fieldName, "FWHM"       )) ESCAPE (IMAGE_FWHM,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MAJ"   )) ESCAPE (IMAGE_FWHM_MAJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MIN"   )) ESCAPE (IMAGE_FWHM_MIN,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MAJOR" )) ESCAPE (IMAGE_FWHM_MAJ,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MINOR" )) ESCAPE (IMAGE_FWHM_MIN,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "FWHM_MEDIAN"    ))   ESCAPE (IMAGE_FWHM_MEDIAN,      OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MAJ_MEDIAN"))   ESCAPE (IMAGE_FWHM_MAJ_MEDIAN,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MIN_MEDIAN"))   ESCAPE (IMAGE_FWHM_MIN_MEDIAN,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MAJOR_MEDIAN")) ESCAPE (IMAGE_FWHM_MAJ_MEDIAN,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "FWHM_MINOR_MEDIAN")) ESCAPE (IMAGE_FWHM_MIN_MEDIAN,  OPIHI_FLT);

  if (!strcasecmp (fieldName, "trate"    )) ESCAPE (IMAGE_TRATE,     OPIHI_FLT);

  if (!strcasecmp (fieldName, "ncal"     )) ESCAPE (IMAGE_NCAL,      OPIHI_INT);
  if (!strcasecmp (fieldName, "sky"      )) ESCAPE (IMAGE_SKY,       OPIHI_FLT); // deprecated for now

  if (!strcasecmp (fieldName, "imflags"  )) ESCAPE (IMAGE_FLAGS,     OPIHI_INT);
  if (!strcasecmp (fieldName, "flags"    )) ESCAPE (IMAGE_FLAGS,     OPIHI_INT);
  if (!strcasecmp (fieldName, "ccdnum"   )) ESCAPE (IMAGE_CCDNUM,    OPIHI_INT);

  if (!strcasecmp (fieldName, "imageID"  )) ESCAPE (IMAGE_IMAGE_ID,  OPIHI_INT);
  if (!strcasecmp (fieldName, "externID" )) ESCAPE (IMAGE_EXTERN_ID, OPIHI_INT);
  if (!strcasecmp (fieldName, "sourceID" )) ESCAPE (IMAGE_SOURCE_ID, OPIHI_INT);

  if (!strcasecmp (fieldName, "X_LL_CHIP")) ESCAPE (IMAGE_X_LL_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_LR_CHIP")) ESCAPE (IMAGE_X_LR_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_UL_CHIP")) ESCAPE (IMAGE_X_UL_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_UR_CHIP")) ESCAPE (IMAGE_X_UR_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_LL_CHIP")) ESCAPE (IMAGE_Y_LL_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_LR_CHIP")) ESCAPE (IMAGE_Y_LR_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_UL_CHIP")) ESCAPE (IMAGE_Y_UL_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_UR_CHIP")) ESCAPE (IMAGE_Y_UR_CHIP, OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_LL_FP"  )) ESCAPE (IMAGE_X_LL_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_LR_FP"  )) ESCAPE (IMAGE_X_LR_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_UL_FP"  )) ESCAPE (IMAGE_X_UL_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "X_UR_FP"  )) ESCAPE (IMAGE_X_UR_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_LL_FP"  )) ESCAPE (IMAGE_Y_LL_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_LR_FP"  )) ESCAPE (IMAGE_Y_LR_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_UL_FP"  )) ESCAPE (IMAGE_Y_UL_FP,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "Y_UR_FP"  )) ESCAPE (IMAGE_Y_UR_FP,   OPIHI_FLT);

  if (!strcasecmp (fieldName, "R_LL"  )) ESCAPE (IMAGE_R_LL,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "R_LR"  )) ESCAPE (IMAGE_R_LR,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "R_UL"  )) ESCAPE (IMAGE_R_UL,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "R_UR"  )) ESCAPE (IMAGE_R_UR,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_LL"  )) ESCAPE (IMAGE_D_LL,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_LR"  )) ESCAPE (IMAGE_D_LR,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_UL"  )) ESCAPE (IMAGE_D_UL,   OPIHI_FLT);
  if (!strcasecmp (fieldName, "D_UR"  )) ESCAPE (IMAGE_D_UR,   OPIHI_FLT);

  if (!strcasecmp (fieldName, "dX_SYS"  )) ESCAPE (IMAGE_X_ERR_SYS,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "dY_SYS"  )) ESCAPE (IMAGE_Y_ERR_SYS,  OPIHI_FLT);
  if (!strcasecmp (fieldName, "dM_SYS"  )) ESCAPE (IMAGE_MAG_ERR_SYS,OPIHI_FLT);

  if (!strcasecmp (fieldName, "UBERCAL_DIST")) ESCAPE (IMAGE_UBERCAL_DIST,OPIHI_INT);
  if (!strcasecmp (fieldName, "UCDIST")) ESCAPE (IMAGE_UBERCAL_DIST,OPIHI_INT);

  if (!strcasecmp (fieldName, "NFIT_PHOTOM"))  ESCAPE (IMAGE_NFIT_PHOTOM,  OPIHI_INT);
  if (!strcasecmp (fieldName, "NFIT_ASTROM"))  ESCAPE (IMAGE_NFIT_ASTROM,  OPIHI_INT);
  if (!strcasecmp (fieldName, "NLINK_PHOTOM")) ESCAPE (IMAGE_NLINK_PHOTOM, OPIHI_INT);
  if (!strcasecmp (fieldName, "NLINK_ASTROM")) ESCAPE (IMAGE_NLINK_ASTROM, OPIHI_INT);
  if (!strcasecmp (fieldName, "REF_COLOR_BLUE")) ESCAPE (IMAGE_REF_COLOR_BLUE, OPIHI_FLT);
  if (!strcasecmp (fieldName, "REF_COLOR_RED"))  ESCAPE (IMAGE_REF_COLOR_RED,  OPIHI_FLT);

  // for words that don't parse, try a photcode
  gprint (GP_ERR, "unknown field '%s' for image table in DVO database\n", fieldName);
  return (FALSE);
}
