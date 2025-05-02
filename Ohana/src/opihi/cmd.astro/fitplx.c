# include "astro.h"

int fitplx (int argc, char **argv) {
  
  int i, N;

  Vector *rvec, *dvec, *tvec, *dRvec, *dDvec;

  Vector *mvec = NULL; // mask vector
  if ((N = get_argument (argc, argv, "-mask"))) {
    remove_argument (N, &argc, argv);
    if ((mvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
    CastVector (mvec, OPIHI_INT);
  }

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = 2;
  }

  int Noutlier = 0;
  float dPsigMax = FLT_MAX;
  if ((N = get_argument (argc, argv, "-outlier-tests"))) {
    remove_argument (N, &argc, argv);
    Noutlier = atoi(argv[N]);
    remove_argument (N, &argc, argv);
    dPsigMax = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int Nresample = 0;
  if ((N = get_argument (argc, argv, "-bootstrap-resample"))) {
    remove_argument (N, &argc, argv);
    Nresample = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Vector *dPvec = NULL;
  if ((N = get_argument (argc, argv, "-dPsig"))) {
    if (!Noutlier) { gprint (GP_ERR, "-dPsig requires -outlier-tests to be non-zero\n"); return FALSE; }
    remove_argument (N, &argc, argv);
    if (!(dPvec = SelectVector (argv[N], ANYVECTOR, TRUE))) return FALSE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fitplx (ra) (dR) (dec) (dD) (mjd) [-mask mask] [-v] [-vv]\n");
    gprint (GP_ERR, "  -outlier-tests Nsamples dPsigMax : run Nsample bootstrap-resamples to define the path deviations and reject based on dPsigMax\n");
    gprint (GP_ERR, "  -dPsig vec : save path deviations in vec\n");
    gprint (GP_ERR, "  -mask mask : excluded points are marked with a 0 mask value\n");
    return (FALSE);
  }

  /* select input / output buffers */
  // if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  // Nx = buf[0].header.Naxis[0];
  // Ny = buf[0].header.Naxis[1];
  
  if ((rvec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", argv[1]);
  if ((dvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", argv[3]);
  if ((tvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", argv[5]);

  if ((dRvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", argv[2]);
  if ((dDvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", argv[4]);

  double *R = rvec->elements.Flt;
  double *D = dvec->elements.Flt;
  double *T = tvec->elements.Flt;
  
  double *dR = dRvec->elements.Flt;
  double *dD = dDvec->elements.Flt;

  opihi_int *mask = NULL;
  if (mvec) {
    mask = mvec->elements.Int;
  }

  // Ntotal : all points supplied by user
  // Nsubset : unmasked points
  int Ntotal = tvec->Nelements; // XXX check other lengths
  if (dPvec) ResetVector (dPvec, OPIHI_FLT, Ntotal);

  double Rmean, Dmean, Tmean;
  PlxSetMeanEpoch (R, D, T, &Rmean, &Dmean, &Tmean, mask, Ntotal);

  /* project coordinates to a plane centered on the object with units of arcsec */
  Coords coords;
  InitCoords (&coords, "DEC--SIN");
  coords.crval1 = Rmean;
  coords.crval2 = Dmean;
  coords.cdelt1 = coords.cdelt2 = 1.0 / 3600.0;

  PlxFitData fitdata;
  PlxFitDataAlloc (&fitdata, Ntotal);
  PlxSetEpochPosition (&fitdata, R, D, dR, dD, T, mask, Ntotal, &coords, Tmean);

  PlxFit fit; memset (&fit, 0, sizeof(PlxFit));

  // determine dPsig for detections based on Noutlier attempts (mask is updated to mark the bad stars, mask == 0)
  if (Noutlier) {
    int clipRetry = TRUE;
    for (i = 0; clipRetry && (i < 3); i++) {
      // XXX NOTE This will segfault is mask is not supplied:
      clipRetry = !PlxOutlierClip (&fitdata, mask, Noutlier, dPsigMax, dPvec, VERBOSE);

      // using the new mask values, reset fitdata
      PlxSetMeanEpoch (R, D, T, &Rmean, &Dmean, &Tmean, mask, Ntotal);
      PlxSetEpochPosition (&fitdata, R, D, dR, dD, T, mask, Ntotal, &coords, Tmean);
      if (VERBOSE) fprintf (stderr, "keep %d of %d\n", fitdata.Npts, Ntotal);
    }
  }

  for (i = 0; (VERBOSE == 2) && (i < fitdata.Npts); i++) {
    int n = fitdata.index[i];
    int maskValue = mask ? mask[n] : 1;
    fprintf (stderr, "%f %f : %f %d : %f %f %f\n", R[n], D[n], T[n], maskValue, fitdata.t[i], fitdata.X[i], fitdata.Y[i]);
  }

  fit.getChisq = TRUE;
  if (!FitPMandPar (&fit, 
		    fitdata.X, fitdata.dX, 
		    fitdata.Y, fitdata.dY, 
		    fitdata.t, fitdata.pX, fitdata.pY, fitdata.Npts, VERBOSE)) {
    return FALSE;
  }

  if (Nresample){
    PlxFitData sample;
    PlxFitDataAlloc (&sample, fitdata.Npts);

    PlxFit *testfit = NULL;
    ALLOCATE (testfit, PlxFit, Nresample);

    int Ngood = 0;
    for (i = 0; i < Nresample; i++) {
      PlxBootstrapResample (&fitdata, &sample);
      
      if (i % 100000 == 99999) fprintf (stderr, ".");

      // fit the sample
      testfit[Ngood].getChisq = FALSE;
      if (!FitPMandPar (&testfit[Ngood], 
			sample.X, sample.dX, 
			sample.Y, sample.dY, sample.t, 
			sample.pX, sample.pY, sample.Npts, VERBOSE)) continue;
      Ngood ++;
    }

    Vector *pvec, *uRvec, *uDvec, *Rvec, *Dvec;

    // save the Nresample histograms
    if ((pvec  = SelectVector ("plxVector", ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", "plxVector");
    if ((uRvec = SelectVector ("uRVector",  ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", "uDVector");
    if ((uDvec = SelectVector ("uDVector",  ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", "uRVector");
    if ((Rvec  = SelectVector ("RoVector",  ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", "RoVector");
    if ((Dvec  = SelectVector ("DoVector",  ANYVECTOR, TRUE)) == NULL) ESCAPE ("missing vector %s\n", "DoVector");
    
    ResetVector ( pvec, OPIHI_FLT, Ngood);
    ResetVector (uRvec, OPIHI_FLT, Ngood);
    ResetVector (uDvec, OPIHI_FLT, Ngood);
    ResetVector ( Rvec, OPIHI_FLT, Ngood);
    ResetVector ( Dvec, OPIHI_FLT, Ngood);
    
    for (i = 0; i < Ngood; i++) {
      pvec->elements.Flt[i]  = testfit[i].p;
      uRvec->elements.Flt[i] = testfit[i].uR;
      uDvec->elements.Flt[i] = testfit[i].uD;
      Rvec->elements.Flt[i]  = testfit[i].Ro;
      Dvec->elements.Flt[i]  = testfit[i].Do;
    }

    // now calculate median and sigma for each vector
    VectorRobustStats (pvec,  &fit.p,  &fit.dp);
    VectorRobustStats (uRvec, &fit.uR, &fit.duR);
    VectorRobustStats (uDvec, &fit.uD, &fit.duD);
    VectorRobustStats (Rvec,  &fit.Ro, &fit.dRo);
    VectorRobustStats (Dvec,  &fit.Do, &fit.dDo);
  }

  // fprintf (stderr, "%f +/- %f | %f %f\n", fit.p, fit.dp, fit.uR, fit.uD);

/*
  FILE *f = fopen ("test.pf.dat", "w");
  for (i = 0; i < Ntotal; i++) {
    double Xf = fit.Ro + fit.uR*fitdata.t[i] + fit.p*fitdata.pX[i];
    double Yf = fit.Do + fit.uD*fitdata.t[i] + fit.p*fitdata.pY[i];
    fprintf (f, "%f : %f %f : %f %f : %f : %f %f : %f %f\n", T[i], R[i], D[i], Xf, Yf, fitdata.t[i], fitdata.X[i], fitdata.Y[i], fitdata.pX[i], fitdata.pY[i]);
  }
  fclose (f);
*/

  // fprintf (stderr, "Roff, Doff: %f, %f; dRo, dDo: %f, %f\n", fit.Ro, fit.Do, fit.dRo, fit.dDo);
  
  XY_to_RD (&Rmean, &Dmean, fit.Ro, fit.Do, &coords);
  if (VERBOSE) {
    fprintf (stderr, "Ro, Do: %f, %f +/- %f, %f (%f, %f)\n", Rmean, Dmean, fit.dRo, fit.dDo, fit.Ro, fit.Do);
    fprintf (stderr, "uR, uD: %f, %f; duR, duD: %f, %f\n", fit.uR, fit.uD, fit.duR, fit.duD);
    fprintf (stderr, "par: %f +/- %f\n", fit.p, fit.dp);
    fprintf (stderr, "chisq: %f Nfit %d\n", fit.chisq, fit.Nfit);
  }

  set_variable ("RA",   Rmean);
  set_variable ("DEC",  Dmean);
  set_variable ("dR",   fit.dRo);
  set_variable ("dD",   fit.dDo);
  set_variable ("uR",   fit.uR);
  set_variable ("uD",   fit.uD);
  set_variable ("duR",  fit.duR);
  set_variable ("duD",  fit.duD);
  set_variable ("plx",  fit.p);
  set_variable ("dplx", fit.dp);
  
  set_variable ("Tmean",  Tmean);

  set_variable ("chisq", fit.chisq);
  set_variable ("Nfit",  fit.Nfit);

  return (TRUE);
}

/* do we want an init function which does the alloc and a clear function to free? */
int FitPMandPar (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, double *pR, double *pD, int Npts, int VERBOSE) {

  int i;

  static double **A = NULL;
  static double **B = NULL;
  double wx, wy, Wx, Wy, Tx, Ty, Tx2, Ty2, Xs, Ys, XT, YT;
  double PR, PD, PRT, PDT, PRX, PDY, PR2, PD2;
  double chisq, Xf, Yf;

  if (A == NULL) {
    ALLOCATE (A, double *, 5);
    ALLOCATE (B, double *, 5);
    for (i = 0; i < 5; i++) {
      ALLOCATE (A[i], double, 5);
      ALLOCATE (B[i], double, 1);
      memset (A[i], 0, 5*sizeof(double));
      memset (B[i], 0, 1*sizeof(double));
    }
  }

  PR = PD = PRT = PDT = PRX = PDY = PR2 = PD2 = 0.0;
  Wx = Wy = Tx = Ty = Tx2 = Ty2 = Xs = Ys = XT = YT = 0.0;
  for (i = 0; i < Npts; i++) {

    if (VERBOSE == 2) fprintf (stderr, "%f %f : %f %f : %f : %f %f\n", X[i], dX[i], Y[i], dY[i], T[i], pR[i], pD[i]);

    /* handle case where dX or dY = 0.0 */
    wx = (fabs(dX[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dX[i]);
    wy = (fabs(dY[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dY[i]);

    Wx += wx;
    Wy += wy;

    Tx += T[i]*wx;
    Ty += T[i]*wy;
    
    Tx2 += SQ(T[i])*wx;
    Ty2 += SQ(T[i])*wy;
    
    PR += pR[i]*wx;
    PD += pD[i]*wy;
    
    PRT += pR[i]*T[i]*wx;
    PDT += pD[i]*T[i]*wy;
    
    PRX += pR[i]*X[i]*wx;
    PDY += pD[i]*Y[i]*wy;
    
    PR2 += SQ(pR[i])*wx;
    PD2 += SQ(pD[i])*wy;

    Xs += X[i]*wx;
    Ys += Y[i]*wy;

    XT += X[i]*T[i]*wx;
    YT += Y[i]*T[i]*wy;
  }

  A[0][0] = Wx;
  A[0][1] = Tx;
  A[0][4] = PR;

  A[1][0] = Tx;
  A[1][1] = Tx2;
  A[1][4] = PRT;

  A[2][2] = Wy;
  A[2][3] = Ty;
  A[2][4] = PD;

  A[3][2] = Ty;
  A[3][3] = Ty2;
  A[3][4] = PDT;

  A[4][0] = PR;
  A[4][1] = PRT;
  A[4][2] = PD;
  A[4][3] = PDT;
  A[4][4] = PR2 + PD2;

  B[0][0] = Xs;
  B[1][0] = XT;
  B[2][0] = Ys;
  B[3][0] = YT;
  B[4][0] = PRX + PDY;

  if (!dgaussjordan ((double **)A, (double **)B, 5, 1)) {
    if (VERBOSE) fprintf (stderr, "error in fit\n");
    if (VERBOSE == 2) {
      int j;
      for (i = 0; i < 5; i++) {
	for (j = 0; j < 5; j++) {
	  fprintf (stderr, "%e ", A[i][j]);
	}
	fprintf (stderr, " : %e\n", A[i][0]);
      }
    }
    return FALSE;
  }

  fit[0].Ro = B[0][0];
  fit[0].uR = B[1][0];
  fit[0].Do = B[2][0];
  fit[0].uD = B[3][0];
  fit[0].p  = B[4][0];
  
  fit[0].dRo = sqrt(A[0][0]);
  fit[0].duR = sqrt(A[1][1]);
  fit[0].dDo = sqrt(A[2][2]);
  fit[0].duD = sqrt(A[3][3]);
  fit[0].dp  = sqrt(A[4][4]);
  
  // (optionally) add up the chi square for the fit
  if (fit->getChisq) {
    chisq = 0.0;
    for (i = 0; i < Npts; i++) {
      Xf = fit[0].Ro + fit[0].uR*T[i] + fit[0].p*pR[i];
      Yf = fit[0].Do + fit[0].uD*T[i] + fit[0].p*pD[i];
      wx = (fabs(dX[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dX[i]);
      wy = (fabs(dY[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dY[i]);
      chisq += SQ(X[i] - Xf) * wx;
      chisq += SQ(Y[i] - Yf) * wy;
      // if (VERBOSE) fprintf (stderr, "chisq contrib : %f %f : %f %f : %f %f : %f %f : %f\n", Xf, Yf, X[i] - Xf, Y[i] - Yf, dX[i], dY[i], (X[i] - Xf) / dX[i], (Y[i] - Yf) / dY[i], chisq);
    }
    // the reduced chisq is divided by (Ndof = 2*Npts - 5)
    fit[0].chisq = chisq / (2.0*Npts - 5.0);
  }
  
  fit[0].Nfit = Npts;
  return (TRUE);
}

int PlxBootstrapResample (PlxFitData *src, PlxFitData *tgt) {
  int i;
  tgt->Npts = src->Npts;
  for (i = 0; i < src->Npts; i++) {
    int N = tgt->Npts * drand48();
    // int N = i;
    tgt->X [i] = src->X [N];
    tgt->Y [i] = src->Y [N];
    tgt->dX[i] = src->dX[N];
    tgt->dY[i] = src->dY[N];
    tgt->t [i] = src->t [N];
    tgt->pX[i] = src->pX[N];
    tgt->pY[i] = src->pY[N];

    // *** make this optional?
    tgt->Wx[i] = src->Wx[N];
    tgt->Wy[i] = src->Wy[N];
  }
  return TRUE;
}

int PlxSetMeanEpoch (double *R, double *D, double *T, double *Rmean, double *Dmean, double *Tmean, opihi_int *mask, int Ntotal) {

  int i;

  // find mean values to remove
  double Nmean = 0;
  *Tmean = 0;
  *Rmean = 0;
  *Dmean = 0;
  double Tmin = +1000000;
  double Tmax = -1000000;
  for (i = 0; i < Ntotal; i++) {
    if (mask && !mask[i]) continue;
    *Rmean += R[i];
    *Dmean += D[i];
    *Tmean += T[i];
    Tmin = MIN(Tmin, T[i]);
    Tmax = MAX(Tmax, T[i]);
    Nmean += 1.0;
  }
  *Rmean /= Nmean;
  *Dmean /= Nmean;
  *Tmean /= Nmean;
  
  double Trange = Tmax - Tmin;

  // fprintf (stderr, "R,D : %f,%f, T: %f, Trange: %f, Tmin: %f, Tmax: %f\n", *Rmean, *Dmean, *Tmean, Trange, Tmin, Tmax);

  set_variable ("Trange", Trange);
  return TRUE;
}

// generate the fit values (projected X,Y; parallax factors; 
int PlxSetEpochPosition (PlxFitData *fitdata, double *R, double *D, double *dR, double *dD, double *T, opihi_int *mask, int Ntotal, Coords *coords, double Tmean) {

  int i;

  float pXmin = +2.0;
  float pXmax = -2.0;
  float pYmin = +2.0;
  float pYmax = -2.0;

  int Nsubset = 0;
  for (i = 0; i < Ntotal; i++) {
    if (mask && !mask[i]) continue;
    RD_to_XY (&fitdata->X[Nsubset], &fitdata->Y[Nsubset], R[i], D[i], coords);
    fitdata->dX[Nsubset] = dR[i];
    fitdata->dY[Nsubset] = dD[i];
    fitdata->t[Nsubset] = (T[i] - Tmean) / 365.25;
    ParFactor (&fitdata->pX[Nsubset], &fitdata->pY[Nsubset], R[i], D[i], T[i]);
    pXmin = MIN (pXmin, fitdata->pX[Nsubset]);
    pXmax = MAX (pXmax, fitdata->pX[Nsubset]);
    pYmin = MIN (pYmin, fitdata->pY[Nsubset]);
    pYmax = MAX (pYmax, fitdata->pY[Nsubset]);

    fitdata->Wx[Nsubset] = 1.0;
    fitdata->Wy[Nsubset] = 1.0;    
    fitdata->index[Nsubset] = i;
    Nsubset++;
  }
  fitdata->Npts = Nsubset;
  float dXRange = pXmax - pXmin;
  float dYRange = pYmax - pYmin;
  float parRange = hypot (dXRange, dYRange);

  set_variable ("Prange", parRange);
  // fprintf (stderr, "par factor range: %f\n", parRange);

  return TRUE;
}

/* Outlier clipping based on bootstrap-resampling tests of the plx path
 * generate Noutlier resampled datasets
 * fit the Noutlier plx paths
 * determine and save the distribution of dXsig and dYsig for each point
 * sort the resulting distributions and find dPsig (median point) for each measurement
 * find the 90% point of dPsig : if > dPsigMax, only clip the 10% most deviant points
 * set the dPvec values if desired
 * -- mask is modified, dPvec values are set
 * -- fitdata is unchanged
 */

# define MAX_REJECT 0.1

int PlxOutlierClip (PlxFitData *fitdata, opihi_int *mask, int Noutlier, float dPsigMax, Vector *dPvec, int VERBOSE) {

  int i, n;

  PlxFit testfit;
  testfit.getChisq = FALSE;

  PlxFitData sample;
  PlxFitDataAlloc (&sample, fitdata->Npts);

  double **dXsig, **dYsig;
  ALLOCATE (dXsig, double *, fitdata->Npts);
  ALLOCATE (dYsig, double *, fitdata->Npts);
  for (i = 0; i < fitdata->Npts; i++) {
    ALLOCATE (dXsig[i], double, Noutlier);
    ALLOCATE (dYsig[i], double, Noutlier);
  }

  int Nsamples = 0;
  for (n = 0; n < Noutlier; n++) {
    // bootstrap resample (fitdata -> sample)
    PlxBootstrapResample (fitdata, &sample);
      
    if (n % 100000 == 99999) fprintf (stderr, ".");

    // fit the sample
    if (!FitPMandPar (&testfit, 
		      sample.X, sample.dX, 
		      sample.Y, sample.dY, sample.t, 
		      sample.pX, sample.pY, sample.Npts, VERBOSE)) continue;

    // fprintf (stderr, "%f +/- %f | %f %f\n", testfit.p, testfit.dp, testfit.uR, testfit.uD);

    // find the distances to the path
    for (i = 0; i < fitdata->Npts; i++) {
      double Xf = testfit.Ro + testfit.uR*fitdata->t[i] + testfit.p*fitdata->pX[i];
      double Yf = testfit.Do + testfit.uD*fitdata->t[i] + testfit.p*fitdata->pY[i];
      dXsig[i][Nsamples] = fabs(fitdata->X[i] - Xf) / fitdata->dX[i];
      dYsig[i][Nsamples] = fabs(fitdata->Y[i] - Yf) / fitdata->dY[i];
      // fprintf (stderr, "%f : %f %f : %f %f : %f %f : %f %f %f\n", T[i], Xf, Yf, fitdata->X[i], fitdata->Y[i], fitdata->dX[i], fitdata->dY[i], fitdata->t[i], fitdata->pX[i], fitdata->pY[i]);
    }
    Nsamples ++;
  }

  double *dPsig;
  ALLOCATE (dPsig, double, fitdata->Npts);
    
  for (i = 0; i < fitdata->Npts; i++) {
    dsort (dXsig[i], Nsamples);
    dsort (dYsig[i], Nsamples);

    // choose the median values
    double dXsigMedian, dYsigMedian;
    if (Nsamples % 2) {
      int Ncenter = Nsamples / 2;
      dXsigMedian = dXsig[i][Ncenter];
      dYsigMedian = dYsig[i][Ncenter];
    } else {
      int Ncenter = Nsamples / 2 - 1;
      dXsigMedian = 0.5*(dXsig[i][Ncenter] + dXsig[i][Ncenter + 1]);
      dYsigMedian = 0.5*(dYsig[i][Ncenter] + dYsig[i][Ncenter + 1]);
    }
    // XXX replace with hypotenuse?
    dPsig[i] = 0.5*(dXsigMedian + dYsigMedian);
    // fprintf (stderr, "%d %10.6f %10.6f %10.6f  %f %f : %f\n", i, R[i], D[i], T[i], dXsig[i][Ncenter], dYsig[i][Ncenter], dPsig[i]);
  }

  // make a copy of dPsig[] and check if > 10% are > dPsigMax
  double *dPsigSort;
  ALLOCATE (dPsigSort, double, fitdata->Npts);
  for (i = 0; i < fitdata->Npts; i++) {
    dPsigSort[i] = dPsig[i];
  }
  dsort (dPsigSort, fitdata->Npts);
  int Nmax = (1.0 - MAX_REJECT)*fitdata->Npts;

  int completeClip = TRUE;
  if (dPsigSort[Nmax] > dPsigMax) {
    if (VERBOSE) fprintf (stderr, "too many outliers: %f at 90\n", dPsigSort[Nmax]);
    dPsigMax = dPsigSort[Nmax];
    completeClip = FALSE;
  }

  for (i = 0; i < fitdata->Npts; i++) {
    if (dPsig[i] < dPsigMax) continue;
    int n = fitdata->index[i];
    // fprintf (stderr, "clip %d: %f : %f\n", i, fitdata->t[i], dPsig[i]);
    mask[n] = 0; // mask these points
  }

  // only set dPvec if we have completed the clipping?
  if (dPvec) {
    for (i = 0; i < dPvec->Nelements; i++) {
      dPvec->elements.Flt[i] = NAN;
    }
    for (i = 0; i < fitdata->Npts; i++) {
      int n = fitdata->index[i];
      dPvec->elements.Flt[n] = dPsig[i];
    }
  }

  free (dPsig);
  free (dPsigSort);
  
  for (i = 0; i < fitdata->Npts; i++) {
    free (dXsig[i]);
    free (dYsig[i]);
  }
  free (dXsig);
  free (dYsig);

  return completeClip;
}

int VectorRobustStats (Vector *vector, double *median, double *sigma) {

  // warn if vector->Nelements > 1000? 10000?)
  // warn if vector is not float

  // we need to copy the vector to avoid changing the sort order
  double *values = NULL;
  ALLOCATE (values, double, vector->Nelements);

  int i;
  int Npts = 0;
  for (i = 0; i < vector->Nelements; i++) {
    if (!isfinite(vector->elements.Flt[i])) continue;
    values[Npts] = vector->elements.Flt[i];
    Npts++;
  }

  dsort (values, Npts);

  if (median) {
    if (Npts % 2) {
      int Ncenter = Npts / 2;
      *median = values[Ncenter];
    } else {
      int Ncenter = Npts / 2 - 1;
      *median = 0.5*(values[Ncenter] + values[Ncenter + 1]);
    }
  }
    
  if (sigma) {
    double Slo = VectorFractionInterpolate (values, 0.158655, Npts);
    double Shi = VectorFractionInterpolate (values, 0.841345, Npts);
    *sigma = (Shi - Slo) / 2.0;
  }
  
  return TRUE;
}

double VectorFractionInterpolate (double *values, float fraction, int Npts) {

  float F = fraction * Npts;
  int   N = fraction * Npts;

  if (N < 0        ) return NAN;
  if (N >= Npts - 2) return NAN;

  // interpolate between N,N+1
    
  double S = (F - N) * (values[N+1] - values[N]) + values[N];
  return S;
}
