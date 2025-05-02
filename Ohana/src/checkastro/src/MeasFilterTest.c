# include "checkastro.h"

/** Determine whether a measurement should be included in the analysis, based on supplied filter criteria */ 
// we only optionally apply the sigma limit: for object averages, this should not be used (should it?)
int MeasFilterTestTiny(MeasureTiny *measure, int applySigmaLim) {
  int found, k;
  long mask;
  PhotCode *code;
  float mag;

  if (!finite(measure[0].dR) || !finite(measure[0].dD)) return FALSE;
  if (!finite(measure[0].M)) return FALSE; //XXX is this necessary for all checkastro tasks?
  if (!finite(measure[0].dM)) return FALSE; //XXX is this necessary for all checkastro tasks?
  
  /* select measurements by photcode, or equiv photcode, if specified */
  if (NphotcodesKeep > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesKeep) && !found; k++) {
      if (photcodesKeep[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesKeep[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (!found) return FALSE;
  }
  
  if (NphotcodesSkip > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesSkip) && !found; k++) {
      if (photcodesSkip[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (found) return FALSE;
  }  
  
  // if (MinBadQF > 0.0) {
  //   if (measure[0].psfQF < MinBadQF) return FALSE;
  // }

  /* select measurements by time */
  if (TimeSelect) {
    if (measure[0].t < TSTART) return FALSE;
    if (measure[0].t > TSTOP) return FALSE;
  }
  
  /* select measurements by quality */
  if (PhotFlagSelect) {
    if (PhotFlagBad) {
      mask = PhotFlagBad;
    } else {
      code = GetPhotcodebyCode (measure[0].photcode);
      mask = code[0].astromBadMask;
    }
    if (mask & measure[0].photFlags) return FALSE;
  }

  /* select measurements by measurement error */
  if (applySigmaLim && (SIGMA_LIM > 0) && (measure[0].dM > SIGMA_LIM)) {
    return FALSE;
  }
  
  /* select measurements by mag limit */
  if (ImagSelect) {
    mag = PhotInstTiny (measure);
    if (mag < ImagMin || mag > ImagMax) return FALSE;
  }
  
  return TRUE;
}

/** Determine whether a measurement should be included in the analysis, based on supplied filter criteria */ 
// we only optionally apply the sigma limit: for object averages, this should not be used (should it?)
int MeasFilterTest(Measure *measure, int applySigmaLim) {
  int found, k;
  long mask;
  PhotCode *code;
  float mag;

  if (!finite(measure[0].dR) || !finite(measure[0].dD)) return FALSE;
  if (!finite(measure[0].M)) return FALSE; //XXX is this necessary for all checkastro tasks?
  if (!finite(measure[0].dM)) return FALSE; //XXX is this necessary for all checkastro tasks?
  
  /* select measurements by photcode, or equiv photcode, if specified */
  if (NphotcodesKeep > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesKeep) && !found; k++) {
      if (photcodesKeep[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesKeep[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (!found) return FALSE;
  }
  
  if (NphotcodesSkip > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesSkip) && !found; k++) {
      if (photcodesSkip[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (found) return FALSE;
  }  
  
  if (MinBadQF > 0.0) {
    if (measure[0].psfQF < MinBadQF) return FALSE;
  }

  /* select measurements by time */
  if (TimeSelect) {
    if (measure[0].t < TSTART) return FALSE;
    if (measure[0].t > TSTOP) return FALSE;
  }
  
  /* select measurements by quality */
  if (PhotFlagSelect) {
    if (PhotFlagBad) {
      mask = PhotFlagBad;
    } else {
      code = GetPhotcodebyCode (measure[0].photcode);
      mask = code[0].astromBadMask;
    }
    if (mask & measure[0].photFlags) return FALSE;
  }

  /* select measurements by measurement error */
  if (applySigmaLim && (SIGMA_LIM > 0) && (measure[0].dM > SIGMA_LIM)) {
    return FALSE;
  }
  
  /* select measurements by mag limit */
  if (ImagSelect) {
    mag = PhotInst (measure);
    if (mag < ImagMin || mag > ImagMax) return FALSE;
  }
  
  return TRUE;
}
