# include "relastro.h"

// for good time sampling, these values are probably conservative
// but with tti pairs, these may not be sufficient...
# define PAR_MIN_NPTS 7
# define PAR_MIN_NPTS_BOOT 6
# define PM_MIN_NPTS 5
# define PM_MIN_NPTS_BOOT 4
# define POS_MIN_NPTS 4
# define POS_MIN_NPTS_BOOT 4

typedef enum {
  SELECT_MEAS_HAS_DATA  = 0x01,
  SELECT_MEAS_HAS_STACK = 0x02,
  SELECT_MEAS_HAS_2MASS = 0x04,
  SELECT_MEAS_HAS_GAIA  = 0x08,
  SELECT_MEAS_HAS_TYCHO = 0x10,
} SelectMeasureStatus;

int DumpObjectsWith2MASS (Catalog *catalog, int Ncatalog);

static DVOAverageFlags astromBits = 
  ID_OBJ_LARGE_PM        | // star with large proper motion
  ID_OBJ_RAW_AVE     	 | // simple weighted average position was used (no IRLS fitting)
  ID_OBJ_FIT_AVE         | // average position was fitted
  ID_OBJ_FIT_PM          | // proper motion model was fitted
  ID_OBJ_FIT_PAR         | // parallax model was fitted
  ID_OBJ_USE_AVE         | // average position used (not PM or PAR)
  ID_OBJ_USE_PM          | // proper motion used (not AVE or PAR)
  ID_OBJ_USE_PAR         | // parallax used (not AVE or PM)
  ID_OBJ_NO_MEAN_ASTROM  | // mean astrometry could not be measured
  ID_OBJ_STACK_FOR_MEAN  | // stack position used for mean astrometry
  ID_OBJ_MEAN_FOR_STACK  | // mean astrometry could not be measured
  ID_OBJ_BAD_PM;           // failure to measure proper-motion model

// This function operates on both Measure and MeasureTiny.  In the big stages, this should
// be called with just MeasureTiny set and Measure == NULL
int UpdateObjects (Catalog *catalog, int Ncatalog, int Nloop) {

  // XXX in the future, use catalog[0].Nsecfilt only?  allow catalogs to have variable Nsecfilt?
  int Nsecfilt = GetPhotcodeNsecfilt ();
  if (Ncatalog) {
    assert (catalog[0].Nsecfilt == Nsecfilt);
  }

  int NmeasureMax = CatalogMaxNmeasure (catalog, Ncatalog);

  // allocate summary stats with Nmax = 0, Nboot = 0
  FitStats *sumStatsChips = FitStatsInit (0, 0);
  FitStats *sumStatsStack = FitStatsInit (0, 0);

  FitStats *fitStatsChips = FitStatsInit (NmeasureMax, N_BOOTSTRAP_SAMPLES);
  FitStats *fitStatsStack = FitStatsInit (NmeasureMax, N_BOOTSTRAP_SAMPLES);

  AstromErrorSetLoop (Nloop, FALSE);

  int i;
  for (i = 0; i < Ncatalog; i++) {

    if (VERBOSE2) fprintf (stderr, "astrometrize catalog %d : "OFF_T_FMT" ave, "OFF_T_FMT" meas\n", i,  catalog[i].Naverage,  catalog[i].Nmeasure);

    FitStatsReset (fitStatsChips);
    FitStatsReset (fitStatsStack);

    off_t j;
    for (j = 0; j < catalog[i].Naverage; j++) {
      /* calculate the average value of R,D for a single star */
      off_t m = catalog[i].average[j].measureOffset;
      MeasureTiny *measure = &catalog[i].measureT[m];
      Measure *measureBig = catalog[i].measure ? &catalog[i].measure[m] : NULL;
      Average *average = &catalog[i].average[j];
      SecFilt *secfilt = &catalog[i].secfilt[j*Nsecfilt];

      if (RESET) { average[0].flags &= ~astromBits; }

      UpdateObjects_Stack(average, secfilt, measure, measureBig, Nsecfilt, fitStatsStack);
      UpdateObjects_Chips(average, secfilt, measure, measureBig, Nsecfilt, fitStatsChips, i, m);
    }
    if (VERBOSE2) fprintf (stderr, "catalog %d : chips "OFF_T_FMT" ave, "OFF_T_FMT" pm, "OFF_T_FMT" par : Nskip "OFF_T_FMT", Noffset "OFF_T_FMT"\n",  i,  fitStatsChips->Nave,  fitStatsChips->Npm,  fitStatsChips->Npar,  fitStatsChips->Nskip, fitStatsChips->Noffset);
    if (VERBOSE2) fprintf (stderr, "catalog %d : stack "OFF_T_FMT" ave, "OFF_T_FMT" pm, "OFF_T_FMT" par : Nskip "OFF_T_FMT", Noffset "OFF_T_FMT"\n",  i,  fitStatsStack->Nave,  fitStatsStack->Npm,  fitStatsStack->Npar,  fitStatsStack->Nskip, fitStatsStack->Noffset);
    FitStatsSum (fitStatsChips, sumStatsChips);
    FitStatsSum (fitStatsStack, sumStatsStack);
  }

  // DumpObjectsWith2MASS (catalog, Ncatalog);

  FitStatsFree (fitStatsChips);
  FitStatsFree (fitStatsStack);

  if (VERBOSE && (Ncatalog > 1)) fprintf (stderr, "fitted "OFF_T_FMT" objects ("OFF_T_FMT" ave, "OFF_T_FMT" pm, "OFF_T_FMT" par), skipped "OFF_T_FMT", "OFF_T_FMT" have too large an offset\n",  (sumStatsChips->Nave + sumStatsChips->Npm + sumStatsChips->Npar),  sumStatsChips->Nave,  sumStatsChips->Npm,  sumStatsChips->Npar,  sumStatsChips->Nskip, sumStatsChips->Noffset);
  if (VERBOSE && (Ncatalog > 1)) fprintf (stderr, "fitted "OFF_T_FMT" objects ("OFF_T_FMT" ave, "OFF_T_FMT" pm, "OFF_T_FMT" par), skipped "OFF_T_FMT", "OFF_T_FMT" have too large an offset\n",  (sumStatsStack->Nave + sumStatsStack->Npm + sumStatsStack->Npar),  sumStatsStack->Nave,  sumStatsStack->Npm,  sumStatsStack->Npar,  sumStatsStack->Nskip, sumStatsStack->Noffset);

  FitStatsFree (sumStatsChips);
  FitStatsFree (sumStatsStack);

  return (TRUE);
}

// This function operates on both Measure and MeasureTiny.  In the big stages, this should
// be called with just MeasureTiny set and Measure == NULL
int DumpObjectsWith2MASS (Catalog *catalog, int Ncatalog) {

  int i;
  for (i = 0; i < Ncatalog; i++) {
    off_t j;
    for (j = 0; j < catalog[i].Naverage; j++) {
      /* calculate the average value of R,D for a single star */
      off_t m = catalog[i].average[j].measureOffset;

      off_t k;
      for (k = 0; k < catalog[i].average[j].Nmeasure; k++) {
	MeasureTiny *measure = &catalog[i].measureT[m+k];
	if (measure->dbFlags & ID_MEAS_OBJECT_HAS_2MASS) {
	  fprintf (stderr, "0x%08x 0x%08x : %12.8f %12.8f %5d\n", catalog[i].average[j].objID, catalog[i].average[j].catID, measure->R, measure->D, measure->photcode);
	}
      }
    }
  }
  return (TRUE);
}

// This function operates on both Measure and MeasureTiny.  In the big stages, this should
// be called with just MeasureTiny set and Measure == NULL
int UpdateObjects_Chips (Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int Nsecfilt, FitStats *fitStats, int cat, off_t measOff) {
  OHANA_UNUSED_PARAM(Nsecfilt);

  int k;

  // we need to chose to do something here...
  int valid = UPDATE_ALL_MEASURE;
  valid |= UPDATE_PS1_CHIP_MEASURE;
  valid |= UPDATE_HSC_MEASURE;
  valid |= UPDATE_CFH_MEASURE;
  if (!valid) return FALSE;

  /* calculate the average value of R,D for a single star */
  FitAstromResult fitPos, fitPM, fitPar;
  FitAstromResultInit (&fitPos);
  FitAstromResultInit (&fitPM);
  FitAstromResultInit (&fitPar);

  // if we fail to fit the astrometry for some reason, we need to set/reset these
  average[0].flags |= ID_OBJ_NO_MEAN_ASTROM;
  average[0].ChiSqAve  = NAN;
  average[0].ChiSqPM   = NAN;
  average[0].ChiSqPar  = NAN;
  average[0].Npos = 0;

  // an object with no measurements is externally supplied
  if (average[0].Nmeasure == 0) return TRUE;

  int mode = FIT_MODE; // start with the globally-defined fit mode
  // mode may be one of FIT_AVERAGE (position only), FIT_PM_ONLY, FIT_PM_AND_PAR

  int XVERB = FALSE;
  XVERB |= (average[0].objID == OBJ_ID_SRC) && (average[0].catID == CAT_ID_SRC);
  XVERB |= (average[0].objID == OBJ_ID_DST) && (average[0].catID == CAT_ID_DST);

  if (XVERB) {
    fprintf (stderr, "found test object\n");
  }

  // select the measurements to be used in this analysis
  UpdateObjects_SelectMeasures (fitStats, average, secfilt, measure, measureBig, FALSE, NULL);

  // if there are no exposure detections, use the stack position
  if (fitStats->Npoints < 1) { 
    if (isfinite(average[0].Rstk) && isfinite(average[0].Dstk)) {
      average[0].R  = average[0].Rstk;
      average[0].D  = average[0].Dstk;
      average[0].dR = average[0].dRstk;
      average[0].dD = average[0].dDstk;
      average[0].flags |= ID_OBJ_STACK_FOR_MEAN;
    }
    return FALSE;
  }

  double Tmean, Trange, parRange;
  FitAstromPoints_Project (fitStats, &Tmean, &Trange, &parRange);

  // To judge the quality of the PM and PAR fits, we need to fit all three models and compare Chisq.
  // we start with the parallax fits, and step down to fewer and fewer parameters

  // if we have too few good detections for the desired fit, or too limited a baseline,
  // use a fit with fewer parameters.

  // fit the parallax + proper-motion model
  if (mode == FIT_PM_AND_PAR) {
    if (Trange < PM_DT_MIN) {
      // not enough baseline for proper motion, only set mean position
      goto justPosition;
    }
    if (parRange < PAR_FACTOR_MIN) {
      // not enough parallax factor range, skip parallax
      goto skipParallax;
    }
    if (fitStats->Npoints < PAR_MIN_NPTS) {
      // not enough data, skip parallax
      goto skipParallax;
    }

    // we are going to use the IRLS analysis to calculate the mean solution and the masking
    // then run N_BOOTSTRAP_SAMPLES to measure the errors.  We first measure the OLS error,
    // and choose the max of the OLS and bootstrap errors
    fitStats->fitdataPar->getError = TRUE;
    if (USE_IRLS) {
      if (!FitPMandPar_IRLS (&fitPar, fitStats->fitdataPar, fitStats->points, fitStats->Npoints)) {
	goto skipParallax;
      }
    } else {
      if (!FitPMandPar_Basic (&fitPar, fitStats->fitdataPar, fitStats->points, fitStats->Npoints)) {
	goto skipParallax;
      }
    }

    // in the fits above, we have saved the formal error for the unmasked points.  
    // if we do not have enough points for bootstrap, we will keep those errors
    if (N_BOOTSTRAP_SAMPLES && (fitPar.Nfit >= PAR_MIN_NPTS_BOOT)) {
      fitStats->Nfit = 0;
      int Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);

      // if we do not have enough points to assess parallax error, skip the bootstrap analysis.
      // (I think the test above means we never do this skip)
      if (Nnomask < PAR_MIN_NPTS_BOOT) goto skipParallaxBootstrap;

      for (k = 0; k < fitStats->NfitAlloc; k++) {
	BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	if (!FitPMandPar_Basic (&fitStats->fit[k], fitStats->fitdataPar, fitStats->sample, Nnomask)) continue;
	fitStats->Nfit ++;
      }

      // These calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
      // this call expects the fitted parameters to have the formal error set: it will apply the 
      // max of the formal and bootstrap errors
      BootstrapRobustStats (&fitPar, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
      BootstrapRobustStats (&fitPar, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
      BootstrapRobustStats (&fitPar, fitStats->fit, fitStats->Nfit, FIT_RESULT_uR);
      BootstrapRobustStats (&fitPar, fitStats->fit, fitStats->Nfit, FIT_RESULT_uD);
      BootstrapRobustStats (&fitPar, fitStats->fit, fitStats->Nfit, FIT_RESULT_PLX);
    }

  skipParallaxBootstrap:
    // project Ro, Do back to RA,DEC
    XY_to_RD (&fitPar.Ro, &fitPar.Do, fitPar.Ro, fitPar.Do, &fitStats->coords);

    average[0].flags |= ID_OBJ_FIT_PAR;
    fitStats->Npar ++;

    // XXX a hard-wired hack...
    // unless there is a clear problems (below) with the parallax fit, we will use it
    int valid = TRUE;
    valid = valid && isfinite(fitPar.uR);
    valid = valid && isfinite(fitPar.uD);
    valid = valid && isfinite(fitPar.p);
    valid = valid && isfinite(fitPar.duR);
    valid = valid && isfinite(fitPar.duD);
    valid = valid && isfinite(fitPar.dp);
    valid = valid && (fabs(fitPar.uR) < 4.0);
    valid = valid && (fabs(fitPar.uD) < 4.0);
    valid = valid && (fabs(fitPar.p) < 2.0);
    if (valid) {
      average[0].flags |= ID_OBJ_USE_PAR;
    }
  }	  

skipParallax:

  // *** first fit for the proper motion (skip fit if Trange or Npts is too small) ***
  if ((mode == FIT_PM_ONLY) || (mode == FIT_PM_AND_PAR)) {
    if (Trange < PM_DT_MIN) {
      goto justPosition;
    }
    if (fitStats->Npoints < PM_MIN_NPTS) {
      goto justPosition;
    }

    // if we have fitted (and accepted) a parallax model, get the best pm fit and chisq
    // given the set of points (mask is respected).  Alternatively, if we do not request
    // IRLS fitting, the just use OLS fitting (skip bootstrap)
    if ((average[0].flags & ID_OBJ_USE_PAR) || !USE_IRLS) {
      if (!FitPM_Basic (&fitPM, fitStats->fitdataPM, fitStats->points, fitStats->Npoints)) {
	average[0].flags |= ID_OBJ_BAD_PM;
	goto justPosition;
      }
      goto skipProperMotionBootstrap;
    } 

    // we are going to use the IRLS analysis to calculate the mean solution and the masking
    // then run N_BOOTSTRAP_SAMPLES to measure the errors.  We first measure the OLS error,
    // and choose the max of the OLS and bootstrap errors
    fitStats->fitdataPM->getError = TRUE;
    if (!FitPM_IRLS (&fitPM, fitStats->fitdataPM, fitStats->points, fitStats->Npoints)) {
      goto justPosition;
    }

    // in the fits above, we have saved the formal error for the unmasked points.  
    // if we do not have enough points for bootstrap, we will keep those errors
    if (N_BOOTSTRAP_SAMPLES && (fitPM.Nfit >= PM_MIN_NPTS_BOOT)) {
      fitStats->Nfit = 0;
      int Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);
	
      // if we do not have enough points to assess p.m. error, skip the bootstrap analysis.
      // (I think the test above means we never do this skip)
      if (Nnomask < PM_MIN_NPTS_BOOT) goto skipProperMotionBootstrap;

      for (k = 0; k < fitStats->NfitAlloc; k++) {
	BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	if (!FitPM_Basic (&fitStats->fit[k], fitStats->fitdataPM, fitStats->sample, Nnomask)) continue;
	fitStats->Nfit ++;
      }

      // These calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
      // this call expects the fitted parameters to have the formal error set: it will apply the 
      // max of the formal and bootstrap errors
      BootstrapRobustStats (&fitPM, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
      BootstrapRobustStats (&fitPM, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
      BootstrapRobustStats (&fitPM, fitStats->fit, fitStats->Nfit, FIT_RESULT_uR);
      BootstrapRobustStats (&fitPM, fitStats->fit, fitStats->Nfit, FIT_RESULT_uD);
    }

skipProperMotionBootstrap:
    // project Ro, Do back to RA,DEC
    XY_to_RD (&fitPM.Ro, &fitPM.Do, fitPM.Ro, fitPM.Do, &fitStats->coords);

    average[0].flags |= ID_OBJ_FIT_PM;
    fitStats->Npm ++;

    // XXX a hard-wired hack...
    // unless there is a clear problems (below) with the proper-motion fit or we have a parallax fit, we will use pm fit
    int valid = TRUE;
    valid = valid && isfinite(fitPM.uR);
    valid = valid && isfinite(fitPM.uD);
    valid = valid && isfinite(fitPM.duR);
    valid = valid && isfinite(fitPM.duD);
    valid = valid && (fabs(fitPM.uR) < 4.0);
    valid = valid && (fabs(fitPM.uD) < 4.0);
    if (!valid) {
      average[0].flags |= ID_OBJ_BAD_PM;
    } else {
      if (!(average[0].flags & ID_OBJ_USE_PAR)) {
	average[0].flags |= ID_OBJ_USE_PM;
      }
    }
  }
  
justPosition:
  {
    // use bootstrap resampling to check the error distribution
    // if we only have one point, this is silly...

    // set the proper motion (to the galaxy model or average value, if desired; else to 0,0)
    FitAstromResultSetPM (&fitPos, 1, average);

    // if we already have a valid fit (pm or par), use OLS to fit the position
    // alternatively, if we do not request IRLS, use OLS
    // alternatively, if we do not have enough points, use OLS
    if ((average[0].flags & (ID_OBJ_USE_PAR | ID_OBJ_USE_PM)) || (fitStats->Npoints < POS_MIN_NPTS) || !USE_IRLS) {
      if (!FitPosPMfixed_Basic (&fitPos, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) { 
	// if this fails, stick with the PM and/or PAR fit from above, or use a single value
	goto doneWithFit;
      }
      // if we have not already gotten a good fit, use this fit
      if (!(average[0].flags & (ID_OBJ_USE_PAR | ID_OBJ_USE_PM))) {
	average[0].flags |= ID_OBJ_USE_AVE;
	average[0].flags |= ID_OBJ_RAW_AVE;
      }
      goto useBasic;
    } 
    
    // try the IRLS fitting, otherwise give up and use OLS
    if (!FitPosPMfixed_IRLS (&fitPos, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) {
      // if the above fails, we need to clear the masks and try again below
      FitPointsClearMasks (fitStats->points, fitStats->Npoints); 
      // just calculate the weighted average
      if (!FitPosPMfixed_Basic (&fitPos, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) { 
	// if this fails, find a single unmasked point below
	goto doneWithFit;
      }
      average[0].flags |= ID_OBJ_USE_AVE;
      average[0].flags |= ID_OBJ_RAW_AVE;
      goto useBasic;
    }
    average[0].flags |= ID_OBJ_USE_AVE;

    if (N_BOOTSTRAP_SAMPLES && (fitPos.Nfit >= POS_MIN_NPTS_BOOT)) {
      fitStats->Nfit = 0;
      int Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);
      
      if (Nnomask < POS_MIN_NPTS_BOOT) goto useBasic;
      
      FitAstromResultSetPM (fitStats->fit, fitStats->NfitAlloc, average);
      for (k = 0; k < fitStats->NfitAlloc; k++) {
	BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	if (!FitPosPMfixed_Basic (&fitStats->fit[k], fitStats->fitdataPos, fitStats->sample, Nnomask)) continue;
	fitStats->Nfit ++;
      }
      
      // These calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
      // this call expects the fitted parameters to have the formal error set: it will apply the 
      // max of the formal and bootstrap errors
      BootstrapRobustStats (&fitPos, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
      BootstrapRobustStats (&fitPos, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
    }

  useBasic:
    // project Ro, Do back to RA,DEC
    XY_to_RD (&fitPos.Ro, &fitPos.Do, fitPos.Ro, fitPos.Do, &fitStats->coords);

    average[0].flags |= ID_OBJ_FIT_AVE;
    fitStats->Nave ++;
  }

 doneWithFit:
  // if no valid fit has been found, try to use a single unmasked point:
  if (!(average[0].flags & (ID_OBJ_USE_PAR | ID_OBJ_USE_PM | ID_OBJ_USE_AVE))) {
    if (!FitPosPMfixed_Single (&fitPos, fitStats->points, fitStats->Npoints)) { 
      fitStats->Nskip ++;
      return FALSE;
    }
    // project Ro, Do back to RA,DEC
    XY_to_RD (&fitPos.Ro, &fitPos.Do, fitPos.Ro, fitPos.Do, &fitStats->coords);

    average[0].flags |= ID_OBJ_USE_AVE;
    average[0].flags |= ID_OBJ_FIT_AVE;
    fitStats->Nave ++;
  }

  // update the bit flags of which points were used
  for (k = 0; k < fitStats->Npoints; k++) {
    int Nm = fitStats->points[k].measure;
    myAssert (Nm >= 0, "oops");
    measure[Nm].dbFlags |= ID_MEAS_USED_OBJ;
    if (measureBig) { measureBig[Nm].dbFlags |= ID_MEAS_USED_OBJ; }
    if (!fitStats->points[k].mask) {
      measure[Nm].dbFlags |= ID_MEAS_UNMASKED_ASTRO;
      if (measureBig) { measureBig[Nm].dbFlags |= ID_MEAS_UNMASKED_ASTRO; }
    }
  }

  // we can set the star reference-image color only if we have loaded the image data
  int setRefColor = areImagesMatched();
  if (setRefColor) {
    float *C_blue = NULL;
    float *C_red = NULL;
    ALLOCATE (C_blue, float, fitStats->Npoints);
    ALLOCATE (C_red, float, fitStats->Npoints);

    int NcBlue = 0;
    int NcRed = 0;

    for (k = 0; k < fitStats->Npoints; k++) {
      int Nm = fitStats->points[k].measure;
      float colorBlue = getColorBlue (measOff + Nm, cat);
      if (!isnan(colorBlue)) {
	C_blue[NcBlue] = colorBlue;
	NcBlue++;
      }
      float colorRed = getColorRed (measOff + Nm, cat);
      if (!isnan(colorRed)) {
	C_red[NcRed] = colorRed;
	NcRed++;
      }
    }

    // need to reassign here if isfinite()
    float colorMedian;
    fsort (C_blue, NcBlue);
    colorMedian = (NcBlue > 0) ? C_blue[(int)(0.5*NcBlue)] : NAN;
    average[0].refColorBlue = colorMedian;
    fsort (C_red, NcRed);
    colorMedian = (NcRed > 0) ? C_red[(int)(0.5*NcRed)] : NAN;
    average[0].refColorRed = colorMedian;

    free (C_blue);
    free (C_red);
  }

  /* choose the result based on the chisq values */
  // XXXX for now, just use the mode as the result:
  FitAstromResult fit;
  FitAstromResultInit (&fit);

  if (average[0].flags & ID_OBJ_USE_PAR) {
    myAssert ((average[0].flags & (ID_OBJ_USE_PM | ID_OBJ_USE_AVE)) == 0, "programming error");
    fit = fitPar;
  }
  if (average[0].flags & ID_OBJ_USE_PM) {
    myAssert ((average[0].flags & (ID_OBJ_USE_PAR | ID_OBJ_USE_AVE)) == 0, "programming error");
    fit = fitPM;
  }
  if (average[0].flags & ID_OBJ_USE_AVE) {
    myAssert ((average[0].flags & (ID_OBJ_USE_PAR | ID_OBJ_USE_PM)) == 0, "programming error");
    fit = fitPos;
  }

  if (XVERB) {
    fprintf (stderr, "%f %f -> %f %f (%f,%f) pm=(%f %f) plx=(%f +/- %f)\n",
	     average[0].R, 
	     average[0].D, 
	     fit.Ro, fit.Do, 
	     3600*(average[0].R - fit.Ro), 
	     3600*(average[0].D - fit.Do),
	     fit.uR, fit.uD, fit.p, fit.dp);
    fprintf (stderr, "-----\n");
  }

  // make sure that the fit succeeded
  int status = TRUE;
  status &= finite(fit.Ro);
  status &= finite(fit.Do);
  status &= finite(fit.dRo);
  status &= finite(fit.dDo);
  status &= finite(fit.uR);
  status &= finite(fit.uD);
  status &= finite(fit.duR);
  status &= finite(fit.duD);
  status &= finite(fit.p);
  status &= finite(fit.dp);
  if (!status) {
    fitStats->Nskip ++;
    return FALSE;
  }

  // what is the offset relative to the mean fit position?
  fitStats->coords.crval1 = average[0].R;
  fitStats->coords.crval2 = average[0].D;
  if (isnan(fitStats->coords.crval1)) {
    return (FALSE);
  }
  if (isnan(fitStats->coords.crval2)) {
    return (FALSE);
  }

  double dXoff, dYoff;
  RD_to_XY (&dXoff, &dYoff, fit.Ro, fit.Do, &fitStats->coords);
  float dPos = hypot (dXoff, dYoff);
  if (dPos > MaxMeanOffset) {
    if (fitStats->Noffset < 100) {
      fprintf (stderr, "(%f,%f) -> (%f,%f) (%f,%f)\n", fitStats->coords.crval1, fitStats->coords.crval2, fit.Ro, fit.Do, dXoff, dYoff);
    }
    fitStats->Noffset ++;
    return FALSE; // XXX ??
  }

  if (XVERB) fprintf (stderr, "%f %f -> %f %f (%f,%f) pm=(%f %f) chisq=(%f, %f, %f)\n",
		      average[0].R,
		      average[0].D,
		      fit.Ro, fit.Do,
		      3600*(average[0].R - fit.Ro),
		      3600*(average[0].D - fit.Do),
		      average[0].uR,
		      average[0].uD,
		      fitPos.chisq, fitPM.chisq, fitPar.chisq);

  average[0].R = ohana_normalize_angle_to_midpoint(fit.Ro, 180.0); // RA in degrees
  average[0].D = ohana_normalize_angle_to_midpoint(fit.Do,   0.0); // DEC in degrees

  average[0].dR 	= fit.dRo; // RA scatter in arcsec
  average[0].dD 	= fit.dDo; // DEC scatter in arcsec

  average[0].uR         = fit.uR; // RA proper motion in arcsec/year
  average[0].uD         = fit.uD; // DEC proper motion in arcsec/year
  average[0].duR        = fit.duR; // RA proper motion error in arcsec/year
  average[0].duD        = fit.duD; // DEC proper motion error in arcsec/year

  average[0].P          = fit.p; // parallax in arcsec
  average[0].dP         = fit.dp; // parallax error in arcsec


  average[0].ChiSqAve   = fitPos.chisq;
  average[0].ChiSqPM    = fitPM.chisq;
  average[0].ChiSqPar   = fitPar.chisq;

  average[0].Tmean      = (Tmean * 86400 * 365.25) + fitStats->T2000;
  average[0].Trange     = (Trange * 86400 * 365.25);
  average[0].Npos       = fit.Nfit;

  // unset the NO_ASTROM bit (not(NO_ASTROM) == HAVE_ASTROM)
  average[0].flags &= ~ID_OBJ_NO_MEAN_ASTROM;

  return (TRUE);
}

// This function operates on both Measure and MeasureTiny.  In the big stages, this should
// be called with just MeasureTiny set and Measure == NULL
int UpdateObjects_Stack (Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int Nsecfilt, FitStats *fitStats) {
  OHANA_UNUSED_PARAM(Nsecfilt);

  int status;

  // we need to chose to do something here...
  if (!UPDATE_PS1_STACK_MEASURE) return FALSE;

  // set the default values
  average[0].Rstk  = NAN; // RA in degrees
  average[0].Dstk  = NAN; // DEC in degrees
  average[0].dRstk = NAN; // RA scatter in arcsec
  average[0].dDstk = NAN; // DEC scatter in arcsec

  /* calculate the average value of R,D for a single star */
  FitAstromResult fitPos;
  FitAstromResultInit (&fitPos);

  if (average[0].Nmeasure == 0) return TRUE;

  int XVERB = FALSE;
  XVERB |= (average[0].objID == OBJ_ID_SRC) && (average[0].catID == CAT_ID_SRC);
  XVERB |= (average[0].objID == OBJ_ID_DST) && (average[0].catID == CAT_ID_DST);

  // select the measurements to be used in this analysis
  int stackEntry = -1;
  status = UpdateObjects_SelectMeasures (fitStats, average, secfilt, measure, measureBig, TRUE, &stackEntry);
  if (status & SELECT_MEAS_HAS_STACK) {
    // if we have any stack measurements, set the default value to the mean position
    // this is set first, so the position may only be the original stack position
    // if we exit this function without updating the stack position, it means we 
    // have a stack measurement, but not a good stack measuremetn

    average[0].Rstk  = average[0].R; // RA in degrees
    average[0].Dstk  = average[0].D; // DEC in degrees
    average[0].dRstk = average[0].dR; // RA scatter in arcsec
    average[0].dDstk = average[0].dD; // DEC scatter in arcsec
    if (stackEntry > -1) {
      // save the epoch for one of the stacks
      average[0].Tmean = measure[stackEntry].t;
    }
    average[0].flags |= ID_OBJ_MEAN_FOR_STACK;
  }

  // no stack measurements of acceptable quality (defaults to mean position)
  if (fitStats->Npoints < 1) return FALSE;
  
  double Tmean, Trange, parRange;
  FitAstromPoints_Project (fitStats, &Tmean, &Trange, &parRange);

  // the stack positions are not statistically independent...
  FitPosStack (&fitPos, fitStats);

  // project Ro, Do back to RA,DEC
  XY_to_RD (&fitPos.Ro, &fitPos.Do, fitPos.Ro, fitPos.Do, &fitStats->coords);

  // XXX choose stack flag? average[0].flags |= ID_OBJ_FIT_AVE;
  fitStats->Nave ++;

  if (XVERB) fprintf (stderr, "%f %f -> %f %f (%f,%f)\n",
		      average[0].R, 
		      average[0].D, 
		      fitPos.Ro, fitPos.Do, 
		      3600*(average[0].R - fitPos.Ro), 
		      3600*(average[0].D - fitPos.Do));

  // check if the fit succeeded
  status = TRUE;
  status &= finite(fitPos.Ro);
  status &= finite(fitPos.Do);
  status &= finite(fitPos.dRo);
  status &= finite(fitPos.dDo);
  if (!status) {
    fitStats->Nskip ++;
    return FALSE;
  }

  // what is the offset relative to the mean fit position?
  fitStats->coords.crval1 = average[0].R;
  fitStats->coords.crval2 = average[0].D;

  double dXoff, dYoff;
  RD_to_XY (&dXoff, &dYoff, fitPos.Ro, fitPos.Do, &fitStats->coords);
  float dPos = hypot (dXoff, dYoff);
  if (dPos > MaxMeanOffset) {
    if (fitStats->Noffset < 100) {
      fprintf (stderr, "(%f,%f) -> (%f,%f) (%f,%f)\n", fitStats->coords.crval1, fitStats->coords.crval2, fitPos.Ro, fitPos.Do, dXoff, dYoff);
    }
    fitStats->Noffset ++;
    return FALSE;
  }

  // set the stack position values
  average[0].Rstk = ohana_normalize_angle_to_midpoint(fitPos.Ro, 180.0); // RA in degrees
  average[0].Dstk = ohana_normalize_angle_to_midpoint(fitPos.Do,   0.0); // DEC in degrees
  average[0].dRstk = fitPos.dRo; // RA error in arcsec
  average[0].dDstk = fitPos.dDo; // DEC error in arcsec
  average[0].Tmean = (Tmean * 86400 * 365.25) + fitStats->T2000;
  average[0].flags &= ~ID_OBJ_MEAN_FOR_STACK;

  return (TRUE);
}

int UpdateObjects_SelectMeasures (FitStats *fit, Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int isStack, int *stackEntry) {
  OHANA_UNUSED_PARAM(secfilt);

  // I've already allocated fit->points (and fit->sample) with space for fit->NpointsAlloc entries

  int has2MASS = FALSE;
  int hasGAIA  = FALSE;
  int hasTycho = FALSE;
  int hasStack = FALSE;
  if (stackEntry) *stackEntry = -1;

  int Npoints = fit->Npoints = 0;
  FitAstromPoint *points = fit->points;

  int TESTPT2 = FALSE;
  TESTPT2 |= CAT_ID_SRC && OBJ_ID_SRC && (average[0].catID == CAT_ID_SRC) && (average[0].objID == OBJ_ID_SRC);
  TESTPT2 |= CAT_ID_DST && OBJ_ID_DST && (average[0].catID == CAT_ID_DST) && (average[0].objID == OBJ_ID_DST);
  if (TESTPT2) {
    fprintf (stderr, "got test det\n");
  }

  // find the basic properties of the detections for this object (Tmin, Tmax, Tmean)
  off_t k;
  for (k = 0; k < average[0].Nmeasure; k++) {

    if (0) {
      char *date = ohana_sec_to_date (measure[k].t);
      int dbFlagsBig = measureBig ? measureBig[k].dbFlags : 0;
      fprintf (stderr, OFF_T_FMT" %f %f %s : 0x%08x : 0x%08x\n",  k, measure[k].R, measure[k].D, date, measure[k].dbFlags, dbFlagsBig);
      free (date);
    }

    // SKIP gpc1 forced-warp data
    if (isGPC1warp(measure[k].photcode)) continue;

    // SKIP gpc1 stack data
    if (isStack) {
      if (!isGPC1stack(measure[k].photcode)) continue;
      hasStack = TRUE;
      if (stackEntry && *stackEntry == -1) *stackEntry = k; // save a reference to one of the valid stack entries
    } else {
      if ( isGPC1stack(measure[k].photcode)) continue;
    }

    // reset the bit to note that a detection was used (or not)
    measure[k].dbFlags &= ~ID_MEAS_USED_OBJ;
    if (measureBig) { measureBig[k].dbFlags &= ~ID_MEAS_USED_OBJ; }

    // does the measurement pass the supplied filtering constraints?
    // MeasFilterTestTiny does not test psfQF
    // exclude bad detections based on: photcodes, psfQF, time range, photflags & astromBadMask, mag_inst
    int keepMeasure = measureBig ? MeasFilterTest(&measureBig[k], FALSE) : MeasFilterTestTiny(&measure[k], FALSE);
    if (!keepMeasure) {
      continue;
    }

    double Ri = measure[k].R;
    double Di = measure[k].D;

    // mark (as POOR) any measurements which are deviant from the mean by > ExcludeBogusRadius
    if (ExcludeBogus) {
      // this is used here, but reset in FitAstromPoints_Project, called after SelectMeasures
      fit->coords.crval1 = average[0].R;
      fit->coords.crval2 = average[0].D;
      double Xi, Yi;
      RD_to_XY (&Xi, &Yi, Ri, Di, &fit->coords);
      double radius = hypot(Xi, Yi);
      if (radius > ExcludeBogusRadius) {
	measure[k].dbFlags |= ID_MEAS_POOR_ASTROM;
	if (measureBig) { measureBig[k].dbFlags |= ID_MEAS_POOR_ASTROM; }
	continue;
      }
      measure[k].dbFlags &= ~ID_MEAS_POOR_ASTROM;
      if (measureBig) { measureBig[k].dbFlags &= ~ID_MEAS_POOR_ASTROM; }
    }

    // outlier rejection
    if (FALSE && FlagOutlier && (measure[k].dbFlags & ID_MEAS_POOR_ASTROM)) {
      continue;
    }

    FitAstromPointInit (&points[Npoints]);

    points[Npoints].R = Ri;
    points[Npoints].D = Di;

    // measure[k].t is UNIX seconds, T2000 is UNIX seconds for J2000.
    // T[] is time in years since J2000 (jd = 2451545)
    points[Npoints].T = (measure[k].t - fit->T2000) / (86400*365.25) ; // time relative to J2000 in years

    // add measured systematic error in quadrature?  only do this after the fit has
    // converged (or you will never improve the poor images)

    // dX,dY are the X and Y direction errors in arcseconds.  dR, dD are the errors in
    // those directions in degrees.  IF we have non-circular errors (different values for
    // X and Y), then dR and dD will be incorrect: they would need to be rotated to take
    // out the position angle

    // dX, dY : error in arcsec:
    points[Npoints].dX = GetAstromErrorTiny (&measure[k], ERROR_MODE_RA);
    points[Npoints].dY = GetAstromErrorTiny (&measure[k], ERROR_MODE_DEC);

    // allow a given photcode or measurement to be
    // ignored if the error is NAN (for photcode, set astromErrSys to NaN)
    if (isnan(points[Npoints].dX)) continue;
    if (isnan(points[Npoints].dY)) continue;

    // avoid dX,dY == 0.0 so we do not have to constantly test for it
    points[Npoints].dX = MAX(points[Npoints].dX, 0.001);
    points[Npoints].dY = MAX(points[Npoints].dY, 0.001);

    points[Npoints].dT = measure[k].dt;

    points[Npoints].measure = k;
    Npoints++;

    // NOTE: if 'isStack' is true, we will never see these photcode values
    hasGAIA  =  (measure[k].photcode == 1030);
    has2MASS = ((measure[k].photcode >= 2011) && (measure[k].photcode <= 2013));
    hasTycho = ((measure[k].photcode >= 2020) && (measure[k].photcode <= 2021));

    myAssert (Npoints <= fit->NpointsAlloc, "oops");
  } // loop over measurements : average[0].Nmeasure 

  int TESTPT = FALSE;
  TESTPT |= CAT_ID_SRC && OBJ_ID_SRC && (average[0].catID == CAT_ID_SRC) && (average[0].objID == OBJ_ID_SRC);
  TESTPT |= CAT_ID_DST && OBJ_ID_DST && (average[0].catID == CAT_ID_DST) && (average[0].objID == OBJ_ID_DST);
  if (TESTPT) {
    fprintf (stderr, "got test det\n");
  }
  
  // flag measurements from stars with 2MASS, GAIA, TYCHO
  // NOTE: if 'isStack' is TRUE, these will not be correctly set.
  // ONLY do this loop if not(isStack) 
  for (k = 0; !isStack && (k < average[0].Nmeasure); k++) {
    // reset the bit to note that an object does or does not have the value
    if (has2MASS) {
      measure[k].dbFlags |=  ID_MEAS_OBJECT_HAS_2MASS;
    } else {
      measure[k].dbFlags &= ~ID_MEAS_OBJECT_HAS_2MASS;
    }
    if (hasGAIA) {
      measure[k].dbFlags |=  ID_MEAS_OBJECT_HAS_GAIA;
    } else {
      measure[k].dbFlags &= ~ID_MEAS_OBJECT_HAS_GAIA;
    }
    if (hasTycho) {
      measure[k].dbFlags |=  ID_MEAS_OBJECT_HAS_TYCHO;
    } else {
      measure[k].dbFlags &= ~ID_MEAS_OBJECT_HAS_TYCHO;
    }
  }

  fit->Npoints = Npoints;

  int status = SELECT_MEAS_HAS_DATA;
  if (hasStack) status |= SELECT_MEAS_HAS_STACK; 
  if (has2MASS) status |= SELECT_MEAS_HAS_2MASS; 
  if (hasGAIA)  status |= SELECT_MEAS_HAS_GAIA; 
  if (hasTycho) status |= SELECT_MEAS_HAS_TYCHO; 
  return status;
}

int CatalogMaxNmeasure (Catalog *catalog, int Ncatalog) {

  int i, j;

  int Nmax = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      Nmax = MAX (Nmax, catalog[i].average[j].Nmeasure);
    }
  }
  return Nmax;
}

int FitAstromSetChisq (FitAstromResult *fit, FitAstromPoint *points, int Npoints, FitMode mode) {

  int i;

  myAbort ("this should not be called anymore");

  // add up the chi square for the fit
  double chisq = 0.0;
  for (i = 0; i < Npoints; i++) {
    double Xf = fit->Ro + fit->uR*points[i].T + fit->p*points[i].pR;
    double Yf = fit->Do + fit->uD*points[i].T + fit->p*points[i].pD;
    chisq += SQ(points[i].X - Xf) / SQ(points[i].dX);
    chisq += SQ(points[i].Y - Yf) / SQ(points[i].dY);
  }
  switch (mode) {
    case FIT_AVERAGE:
      fit->chisq = chisq / (2.0*Npoints - 2.0);
      break;
    case FIT_PM_ONLY:
      fit->chisq = chisq / (2.0*Npoints - 4.0);
      break;
    case FIT_PM_AND_PAR:
      fit->chisq = chisq / (2.0*Npoints - 5.0);
      break;
    default:
      myAbort ("invalid mode");
  }
  fit->Nfit = Npoints;
  return (TRUE);
}

int FitAstromResultSetPM (FitAstromResult *fit, int Nfit, Average *average) {

  int i;

  if (APPLY_PROPER_MOTION) {
    for (i = 0; i < Nfit; i++) {
      if (USE_GALAXY_MODEL) {
	fit[i].uR = average->uRgal;
	fit[i].uD = average->uDgal;
      } else {
	fit[i].uR = average->uR;
	fit[i].uD = average->uD;
      }
    }
  } else {
    for (i = 0; i < Nfit; i++) {
      fit[i].uR = 0.0;
      fit[i].uD = 0.0;
    }
  }

  return TRUE;
}
