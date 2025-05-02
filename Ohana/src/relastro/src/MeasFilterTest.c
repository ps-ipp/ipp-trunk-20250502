# include "relastro.h"

/** lifted from relphot/StarOps.clean_measures */
void FlagOutliers2D(Catalog *catalog);

// operates on Full values (not tiny)
void FlagOutliers (Catalog *catalog) {

  // XXX FlagOutliers is now just using FlagOutliers2D
  FlagOutliers2D(catalog);
  return;

  int Ndel, Nave;
  off_t i, j, k, m, N, Nmax, TOOFEW, Nsecfilt;
  double Ns, theta, x, y;
  double *R, *D, *dR, *dD;
  StatType statsR, statsD;

  Nsecfilt = GetPhotcodeNsecfilt();
  assert(catalog[0].Nsecfilt == Nsecfilt);

  if (VERBOSE2) fprintf (stderr, "marking poor measures\n");
  Nmax = 0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Nmax = MAX (Nmax, catalog[0].average[i].Nmeasure);
  }

  ALLOCATE (R, double, Nmax);
  ALLOCATE (D, double, Nmax);
  ALLOCATE (dR, double, Nmax);
  ALLOCATE (dD, double, Nmax);

  /* it makes no sense to mark 3-sigma outliers with <5 measurements */
  TOOFEW = MAX (5, SRC_MEAS_TOOFEW);

  Ns = CLIP_THRESH;
  Ndel = Nave = 0;
      
  /* loop over each object in the catalog */
  for (j = 0; j < catalog[0].Naverage; j++) {
    
    // pointer to this set of measurements
    m = catalog[0].average[j].measureOffset;
    Measure *measure = &catalog[0].measure[m];

    /* accumulate list of valid measurements */
    N = 0;
    for (k = 0; k < catalog[0].average[j].Nmeasure; k++) {
      // skip measurements based on user selected criteria
      if (!MeasFilterTest(&measure[k], FALSE)) continue;
      R[N] = measure[k].R;
      D[N] = measure[k].D;
      dR[N] = GetAstromError (&measure[k], ERROR_MODE_RA);
      dD[N] = GetAstromError (&measure[k], ERROR_MODE_DEC);
      if (isnan(R[N]) || isnan(D[N])) continue;
      N++;
    }
    if (N <= TOOFEW) continue;
    
    /* 3-sigma clip based on stats of inner 50% */
    initstats ("MEAN");
    liststats (R, dR, N, &statsR);
    liststats (D, dD, N, &statsD);
    
    statsR.sigma = MAX (MIN_ERROR, statsR.sigma);
    statsD.sigma = MAX (MIN_ERROR, statsD.sigma);
    
    /* compare per-object distance to this standard deviation, and flag outliers*/
    N = 0;
    for (k = 0; k < catalog[0].average[j].Nmeasure; k++) {
      // reset flag on each invocation
      measure[k].dbFlags &= ~ID_MEAS_POOR_ASTROM;

      // skip measurements based on user selected criteria
      if (!MeasFilterTest(&measure[k], FALSE)) continue;
      
      x = measure[k].R - statsR.median;
      y = measure[k].D - statsD.median;
      theta = atan2(y,x);
      if ((x*x + y*y) > (SQR(statsR.sigma * Ns * cos(theta)) + 
			 SQR(statsD.sigma * Ns * sin(theta)))) {   
	measure[k].dbFlags |= ID_MEAS_POOR_ASTROM;
	Ndel++;
      }
      N++;
      Nave ++;
    }

    // examine results
    // relastroVisualPlotOutliers(catalog, catalog[0].average[j].measureOffset, catalog[0].average[j].Nmeasure, statsR, statsD, Ns);
  }
  
  if (VERBOSE) fprintf (stderr, "%d measures marked poor, %d total\n", Ndel, Nave);
  free (R);
  free (dR);
  free (D);
  free (dD); 
}


/** an alternative outlier rejection scheme */
void FlagOutliers2D (Catalog *catalog) {

  int Ndel, Nave;
  off_t i, j, k, m, N, Nmax, TOOFEW, Nsecfilt;
  double *index;
  double Ns, theta, x, y;
  double *R, *D, *dR, *dD, *d2;
  StatType statsR, statsD;

  // XXX we are not going to use this for now
  return;

  Nsecfilt = GetPhotcodeNsecfilt();
  assert(catalog[0].Nsecfilt == Nsecfilt);

  if (VERBOSE2) fprintf (stderr, "marking poor measures\n");
  Nmax = 0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Nmax = MAX (Nmax, catalog[0].average[i].Nmeasure);
  }

  ALLOCATE (R, double, Nmax);
  ALLOCATE (D, double, Nmax);
  ALLOCATE (dR, double, Nmax);
  ALLOCATE (dD, double, Nmax);
  ALLOCATE (d2, double, Nmax);
  ALLOCATE (index, double, Nmax);

  /* it makes no sense to mark 3-sigma outliers with <5 measurements */
  TOOFEW = MAX (5, SRC_MEAS_TOOFEW);

  Ns = CLIP_THRESH;
  Ndel = Nave = 0;
      
  /* loop over each object in the catalog */
  for (j = 0; j < catalog[0].Naverage; j++) {
    
    // pointer to this set of measurements
    m = catalog[0].average[j].measureOffset;
    Measure *measure = &catalog[0].measure[m];

    /* accumulate list of valid measurements */
    N = 0;
    for (k = 0; k < catalog[0].average[j].Nmeasure; k++) {

      // reset flag on each invocation
      measure[k].dbFlags &= ~ID_MEAS_POOR_ASTROM;
      
      // skip measurements based on user selected criteria
      if (!MeasFilterTest(&measure[k], FALSE)) continue;
      R[N] = measure[k].R;
      D[N] = measure[k].D;
      dR[N] = GetAstromError(&measure[k], ERROR_MODE_RA);
      dD[N] = GetAstromError(&measure[k], ERROR_MODE_DEC);
      if (isnan(R[N]) || isnan(D[N])) continue;
      N++;
    }
    if (N <= TOOFEW) continue;
    
    /* calculate mean of all points*/
    initstats ("MEAN");
    liststats (R, dR, N, &statsR);
    liststats (D, dD, N, &statsD);
    statsR.sigma = MAX (MIN_ERROR, statsR.sigma);
    statsD.sigma = MAX (MIN_ERROR, statsD.sigma);
    
    /* calculate deviations of all points*/
    N = 0;
    for (k = 0; k < catalog[0].average[j].Nmeasure; k++) {
      // skip bad measurements
      if (!MeasFilterTest(&measure[k], FALSE)) continue;  
      x = measure[k].R - statsR.median;
      y = measure[k].D - statsD.median;
      theta = atan2(y,x);
      d2[N] = (x*x + y*y) / (SQR(statsR.sigma * Ns * cos(theta)) + 
			     SQR(statsD.sigma * Ns * sin(theta)));      
      index[N] = k;
      N++;
    }
    
    // sort d2
    dsortpair(d2, index, N);
    N = (N/2 > (N-1)) ? N/2 : N-1;

    // recalculate image center, sigma based on closest 50% of points
    for (k = 0;  k < N; k++) {
      off_t ind = (off_t) index[k];
      R[k] = measure[ind].R;
      D[k] = measure[ind].D;
      dR[k] = GetAstromError(&measure[ind], ERROR_MODE_RA);
      dD[k] = GetAstromError(&measure[ind], ERROR_MODE_DEC);
    }
    liststats (R, dR, N, &statsR);
    liststats (D, dD, N, &statsD);
    statsR.sigma = MAX (MIN_ERROR, statsR.sigma);
    statsD.sigma = MAX (MIN_ERROR, statsD.sigma);
    
    // use these new statistics to flag outliers 
    N = 0;
    for (k = 0; k < catalog[0].average[j].Nmeasure; k++) {
      //skip bad measurements
      if (!MeasFilterTest(&measure[k], FALSE)) continue;  
      x = measure[k].R - statsR.median;
      y = measure[k].D - statsD.median;
      theta = atan2(y,x);
      d2[N] = (x*x + y*y) / (SQR(statsR.sigma * Ns * cos(theta)) + 
			     SQR(statsD.sigma * Ns * sin(theta)));      
      if ((d2[N]) > 1) {
	measure[k].dbFlags |= ID_MEAS_POOR_ASTROM;
	Ndel ++;
      }
      N++;
      Nave++;
    }  // done rejecting outliers

    // examine results
    // relastroVisualPlotOutliers(catalog, catalog[0].average[j].measureOffset, catalog[0].average[j].Nmeasure, statsR, statsD, Ns);
    
  } // done looping over objects
  
  if (VERBOSE) fprintf (stderr, "%d measures marked poor, %d total\n", Ndel, Nave);
  free (R);
  free (dR);
  free (D);
  free (dD); 
  free (d2);
  free (index);
}

/** Determine whether a measurement should be included in the analysis, based on supplied filter criteria */ 
// we only optionally apply the sigma limit: for object averages, this should not be used
int MeasFilterTestTiny(MeasureTiny *measure, int applySigmaLim) {
  int found, k;
  long mask;
  PhotCode *code;
  float mag;

  if (!finite(measure[0].R) || !finite(measure[0].D)) return FALSE;
  if (!finite(measure[0].M)) return FALSE; //XXX is this necessary for all relastro tasks?
  if (!finite(measure[0].dM)) return FALSE; //XXX is this necessary for all relastro tasks?
  
  if ((MinBadQF > 0.0) && (isGPC1chip(measure[0].photcode) || isGPC1stack(measure[0].photcode))) {
    if (!isfinite(measure[0].psfQF)) { return FALSE; };
    if (measure[0].psfQF < MinBadQF) { return FALSE; };
  }

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
      if (!code) return FALSE;
      mask = code[0].astromBadMask;
    }
    if (mask & measure[0].photFlags) return FALSE;
  }

  /* select measurements by measurement error */
  // this is a bit convoluted: applySigmaLim is only TRUE when this function is
  // called by bcatalog.  for UpdateObjects, it is FALSE
  if (applySigmaLim && (SIGMA_LIM > 0) && (measure[0].dM > SIGMA_LIM)) {
    return FALSE;
  }
  
  /* select measurements by mag limit */
  if (ImagSelect) {
    mag = PhotInstTiny (measure, MAG_CLASS_PSF);
    if (mag < ImagMin || mag > ImagMax) return FALSE;
  }
  
  return TRUE;
}

# define SUPER_VERBOSE 0

/** Determine whether a measurement should be included in the analysis, based on supplied filter criteria */ 
// we only optionally apply the sigma limit: for object averages, this should not be used (should it?)
int MeasFilterTest(Measure *measure, int applySigmaLim) {
  int found, k;
  long mask;
  PhotCode *code;
  float mag;

  if (!finite(measure[0].R) || !finite(measure[0].D)) { if (SUPER_VERBOSE) fprintf (stderr, "filter 1\n"); return FALSE; };
  if (!finite(measure[0].M)) { if (SUPER_VERBOSE) fprintf (stderr, "filter 2\n"); return FALSE; }; //XXX is this necessary for all relastro tasks?
  if (!finite(measure[0].dM)) { if (SUPER_VERBOSE) fprintf (stderr, "filter 3\n"); return FALSE; }; //XXX is this necessary for all relastro tasks?
  
  /* select measurements by photcode, or equiv photcode, if specified */
  if (NphotcodesKeep > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesKeep) && !found; k++) {
      if (photcodesKeep[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesKeep[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (!found) { if (SUPER_VERBOSE) fprintf (stderr, "filter 4\n"); return FALSE; };
  }
  
  if (NphotcodesSkip > 0) {
    found = FALSE;
    for (k = 0; (k < NphotcodesSkip) && !found; k++) {
      if (photcodesSkip[k][0].code == measure[0].photcode) found = TRUE;
      if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
    }
    if (found) { if (SUPER_VERBOSE) fprintf (stderr, "filter 5\n"); return FALSE; };
  }  
  
  if ((MinBadQF > 0.0) && isGPC1chip(measure[0].photcode)) {
    if (!isfinite(measure[0].psfQF)) { if (SUPER_VERBOSE) fprintf (stderr, "filter 6\n"); return FALSE; };
    if (measure[0].psfQF < MinBadQF) { if (SUPER_VERBOSE) fprintf (stderr, "filter 7\n"); return FALSE; };
  }

  /* select measurements by time */
  if (TimeSelect) {
    if (measure[0].t < TSTART) { if (SUPER_VERBOSE) fprintf (stderr, "filter 8\n"); return FALSE; };
    if (measure[0].t > TSTOP) { if (SUPER_VERBOSE) fprintf (stderr, "filter 9\n"); return FALSE; };
  }
  
  /* select measurements by quality */
  if (PhotFlagSelect) {
    if (PhotFlagBad) {
      mask = PhotFlagBad;
    } else {
      code = GetPhotcodebyCode (measure[0].photcode);
      if (!code) { if (SUPER_VERBOSE) fprintf (stderr, "filter 10\n"); return FALSE; };
      mask = code[0].astromBadMask;
    }
    if (mask & measure[0].photFlags) { if (SUPER_VERBOSE) fprintf (stderr, "filter 11\n"); return FALSE; };
  }

  /* select measurements by measurement error */
  if (applySigmaLim && (SIGMA_LIM > 0) && (measure[0].dM > SIGMA_LIM)) {
    { if (SUPER_VERBOSE) fprintf (stderr, "filter 12\n"); return FALSE; };
  }
  
  /* select measurements by mag limit */
  if (ImagSelect) {
    mag = PhotInst (measure, MAG_CLASS_PSF);
    if (mag < ImagMin || mag > ImagMax) { if (SUPER_VERBOSE) fprintf (stderr, "filter 13\n"); return FALSE; };
  }
  
  return TRUE;
}
