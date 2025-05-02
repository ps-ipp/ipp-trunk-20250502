# include "astro.h"

int fitplx_irls (int argc, char **argv) {
  
  int i, N;

  Vector *rvec, *dvec, *tvec, *dRvec, *dDvec;

  Vector *mvec = NULL; // mask vector
  if ((N = get_argument (argc, argv, "-mask"))) {
    remove_argument (N, &argc, argv);
    if ((mvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  Vector *WxOutV = NULL; // mask vector
  if ((N = get_argument (argc, argv, "-Wxvec"))) {
    remove_argument (N, &argc, argv);
    if ((WxOutV = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  Vector *WyOutV = NULL; // mask vector
  if ((N = get_argument (argc, argv, "-Wyvec"))) {
    remove_argument (N, &argc, argv);
    if ((WyOutV = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
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

  int Nresample = 0;
  if ((N = get_argument (argc, argv, "-bootstrap-resample"))) {
    remove_argument (N, &argc, argv);
    Nresample = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int max_iterations = 10;
  if ((N = get_argument (argc, argv, "-max-iterations"))) {
    remove_argument (N, &argc, argv);
    max_iterations = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  double outlier_limit = 0.1;
  if ((N = get_argument (argc, argv, "-outlier-limit"))) {
    remove_argument (N, &argc, argv);
    outlier_limit = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  
  double binning_step = 0.0;
  if ((N = get_argument (argc, argv, "-binning"))) {
    remove_argument (N, &argc, argv);
    binning_step = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fitplx (ra) (dR) (dec) (dD) (mjd) [-mask mask] [-v] [-vv]\n");
    gprint (GP_ERR, "  -max-iterations : maximum number of IRLS iterations to run (default 10)\n");
    gprint (GP_ERR, "  -outlier-limit : fraction of average weight to reject on (default 0.1)\n");
    gprint (GP_ERR, "  -binning <step_size> : fraction of a year to use in a bin for initial pass (default 0.0/no binning)\n");
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

  // Ntotal : all points supplied by user
  // Nsubset : unmasked points
  int Ntotal = tvec->Nelements; // XXX check other lengths

  if (rvec->Nelements  != Ntotal) ESCAPE ("mis-match in vector lengths (mjd vs ra)  = (%d vs %d)\n", rvec->Nelements, Ntotal);
  if (dvec->Nelements  != Ntotal) ESCAPE ("mis-match in vector lengths (mjd vs dec) = (%d vs %d)\n", dvec->Nelements, Ntotal);
  if (dRvec->Nelements != Ntotal) ESCAPE ("mis-match in vector lengths (mjd vs dR)  = (%d vs %d)\n", dRvec->Nelements, Ntotal);
  if (dDvec->Nelements != Ntotal) ESCAPE ("mis-match in vector lengths (mjd vs dD)  = (%d vs %d)\n", dDvec->Nelements, Ntotal);
  
  // if mask exists and is an INT, treat as a supplied mask
  // otherwise, reset

  opihi_int *mask = NULL;
  if (mvec) {
    if (mvec->type != OPIHI_INT) {
      ResetVector (mvec, OPIHI_INT, Ntotal);
      mask = mvec->elements.Int;
      for (i = 0; i < Ntotal; i++) { mask[i] = 1; }
    } else {
      mask = mvec->elements.Int;
    }
  }

  if (WxOutV) {
    ResetVector (WxOutV, OPIHI_FLT, Ntotal);
    for (i = 0; i < Ntotal; i++) { WxOutV->elements.Flt[i] = NAN; }
  }
  if (WyOutV) {
    ResetVector (WyOutV, OPIHI_FLT, Ntotal);
    for (i = 0; i < Ntotal; i++) { WyOutV->elements.Flt[i] = NAN; }
  }


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
  // fitdata only contains the points which are not masked

  PlxFit fit; memset (&fit, 0, sizeof(PlxFit));

  for (i = 0; (VERBOSE == 2) && (i < fitdata.Npts); i++) {
    int n = fitdata.index[i];
    opihi_int maskValue = mask ? mask[n] : 1;
    fprintf (stderr, "%f %f : %f "OPIHI_INT_FMT" : %f %f %f\n", R[n], D[n], T[n], maskValue, fitdata.t[i], fitdata.X[i], fitdata.Y[i]);
  }

  fit.getChisq = TRUE;
  if (!FitPMandPar_IRLS (&fit, 
			 fitdata.X, fitdata.dX, 
			 fitdata.Y, fitdata.dY, 
			 fitdata.t, fitdata.pX, fitdata.pY,
			 fitdata.Wx, fitdata.Wy,
			 fitdata.Qx, fitdata.Qy,
			 fitdata.qx, fitdata.qy,
			 fitdata.Npts, max_iterations, outlier_limit, binning_step, VERBOSE)) {
    return FALSE;
  }
  // FitPMandPar_IRLS sets the values of Wx,Wy based on the fit distance

  if (VERBOSE && Nresample) {
    fprintf (stderr, "-- solution before resample --\n");
    fprintf (stderr, "Ro, Do: %f, %f +/- %f, %f (%f, %f)\n", Rmean, Dmean, fit.dRo, fit.dDo, fit.Ro, fit.Do);
    fprintf (stderr, "uR, uD: %f, %f; duR, duD: %f, %f\n", fit.uR, fit.uD, fit.duR, fit.duD);
    fprintf (stderr, "par: %f +/- %f\n", fit.p, fit.dp);
    fprintf (stderr, "chisq: %f Nfit %d\n", fit.chisq, fit.Nfit);
  }

  // update the mask based on the input mask and the outlier limits.
  if (mask) {
    double Sum_Wx = 0;
    double Sum_Wy = 0;
    
    // calculate the total weight
    for (i = 0; i < fitdata.Npts; i++) {
      Sum_Wx += fitdata.Wx[i];
      Sum_Wy += fitdata.Wy[i];
    }
    for (i = 0; i < fitdata.Npts; i++) {
      // fitdata only includes the previously unmasked points
      if ((fitdata.Wx[i] < outlier_limit * Sum_Wx / (1.0 * fitdata.Npts))||
	  (fitdata.Wy[i] < outlier_limit * Sum_Wy / (1.0 * fitdata.Npts))) {
	int n = fitdata.index[i];
	mask[n] = 0;
      
	if (VERBOSE == 2) {
	  fprintf (stderr, "%f %f : %f "OPIHI_INT_FMT" : %f %f %f : %f %f %f %f\n", R[n], D[n], T[n], mask[n], fitdata.t[i], fitdata.X[i], fitdata.Y[i], fitdata.Wx[i], fitdata.Wy[i], Sum_Wx, Sum_Wy);
	}
      }
    }
  }
  if (WxOutV) {
    // save the calculated weights
    for (i = 0; i < fitdata.Npts; i++) {
      // fitdata only includes the previously unmasked points
      int n = fitdata.index[i];
      WxOutV->elements.Flt[n] = fitdata.Wx[i];
    }
  }
  if (WyOutV) {
    // save the calculated weights
    for (i = 0; i < fitdata.Npts; i++) {
      // fitdata only includes the previously unmasked points
      int n = fitdata.index[i];
      WyOutV->elements.Flt[n] = fitdata.Wy[i];
    }
  }

  if (Nresample) {
    // if the mask has been updated, we need to recalculate mean epoch and positions
    if (mask) {
      PlxSetMeanEpoch (R, D, T, &Rmean, &Dmean, &Tmean, mask, Ntotal);
      PlxSetEpochPosition (&fitdata, R, D, dR, dD, T, mask, Ntotal, &coords, Tmean);
    }

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
			sample.pX, sample.pY,
			sample.Npts, VERBOSE)) continue;
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

    // now calculate robust sigma for each vector
    VectorRobustStats (pvec,  NULL, &fit.dp);
    VectorRobustStats (uRvec, NULL, &fit.duR);
    VectorRobustStats (uDvec, NULL, &fit.duD);
    VectorRobustStats (Rvec,  NULL, &fit.dRo);
    VectorRobustStats (Dvec,  NULL, &fit.dDo);

    PlxFitDataFree (&sample);    
  }

  // fprintf (stderr, "%f +/- %f | %f %f\n", fit.p, fit.dp, fit.uR, fit.uD);

  Vector *dRresPOS, *dDresPOS, *dRresPMP, *dDresPMP, *dRresPLX, *dDresPLX;

  // save fit residuals (with only pm removed, and pm and plx removed)
  if ((dRresPOS = SelectVector ("dRresPOS", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dRresPOS");
  if ((dDresPOS = SelectVector ("dDresPOS", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dDresPOS");
  if ((dRresPMP = SelectVector ("dRresPMP", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dRresPMP");
  if ((dDresPMP = SelectVector ("dDresPMP", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dDresPMP");
  if ((dRresPLX = SelectVector ("dRresPLX", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dRresPLX");
  if ((dDresPLX = SelectVector ("dDresPLX", ANYVECTOR, TRUE)) == NULL) ESCAPE ("cannot generate vector %s\n", "dDresPLX");
    
  ResetVector (dRresPOS, OPIHI_FLT, Ntotal);
  ResetVector (dDresPOS, OPIHI_FLT, Ntotal);
  ResetVector (dRresPMP, OPIHI_FLT, Ntotal);
  ResetVector (dDresPMP, OPIHI_FLT, Ntotal);
  ResetVector (dRresPLX, OPIHI_FLT, Ntotal);
  ResetVector (dDresPLX, OPIHI_FLT, Ntotal);
  
  for (i = 0; i < Ntotal; i++) {
    
    double x0, y0;
    RD_to_XY (&x0, &y0, R[i], D[i], &coords);

    double pX0, pY0;
    ParFactor (&pX0, &pY0, R[i], D[i], T[i]);

    double t0 = (T[i] - Tmean)/365.25;

    double Xpmp = fit.Ro + fit.uR*t0 + fit.p*pX0;
    double Ypmp = fit.Do + fit.uD*t0 + fit.p*pY0;
    double Xplx = fit.Ro + fit.uR*t0;
    double Yplx = fit.Do + fit.uD*t0;

    dRresPOS->elements.Flt[i] = x0;
    dDresPOS->elements.Flt[i] = y0;
    dRresPMP->elements.Flt[i] = x0 - Xpmp;
    dDresPMP->elements.Flt[i] = y0 - Ypmp;
    dRresPLX->elements.Flt[i] = x0 - Xplx;
    dDresPLX->elements.Flt[i] = y0 - Yplx;
  }

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

  PlxFitDataFree (&fitdata);
  
  return (TRUE);
}

/* do we want an init function which does the alloc and a clear function to free? */
int FitPMandPar_IRLS (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, double *pR, double *pD,
		      double *Wx, double *Wy, double *Qx, double *Qy, double *qx, double *qy,
		      int Npts, int max_iterations, double outlier_limit, double binning_step, int VERBOSE) {

  int i,j;

  static double **A = NULL;
  static double **B = NULL;
  double chisq, Xf, Yf;

  double **Cov;                // Ordinary least squares Covariance matrix
  double *Beta, *Beta_prev;    // Current parameter vector, previous parameter vector.

  double sigma_ols, sigma_hat; // Sigma estimates.
  double *rx, *ry;             // Deviation from model
  double *u;                   // Deviation magnitude
  int converged;
  int iterations;
  double tolerance = 1e-4;
  int p = 5;                  // Number of fit parameters
  
  // Allocate what will be the Ax=B matrices
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

  // XXX EAM : I suggest making these static as well like the A and B matrices
  // Allocate the IRLS specific objects
  ALLOCATE (Cov, double *, 5);
  for (i = 0; i < 5; i++) {
    ALLOCATE ( Cov[i], double, 5);
  }

  ALLOCATE(Beta, double, 5);
  ALLOCATE(Beta_prev, double, 5);
  ALLOCATE(rx,  double, Npts);
  ALLOCATE(ry,  double, Npts);
  ALLOCATE(u,  double, Npts);
  
  // Convert the measurement errors into the initial weights.
  for (i = 0; i < Npts; i++) {
    Qx[i] = (fabs(dX[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dX[i]);
    Qy[i] = (fabs(dY[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dY[i]);
  }
  
  // Solve OLS equation
  
  // Solve OLS equation
  if (binning_step == 0.0) {
    if (!weighted_LS_PLX(T,pR,pD,X,Qx,Y,Qy,Npts,A,B,VERBOSE)) {
      // Handle fail case
      return(FALSE);
    }
  }
  else {
    int Nbins;
    double *Tbin = NULL;
    double *pRbin = NULL;
    double *pDbin = NULL;
    double *Xbin = NULL;
    double *Ybin = NULL;
    double *WXbin = NULL;
    double *WYbin = NULL;

    if (!bin_points_PLX(T, pR, pD, X, Qx, Y, Qy, Npts,
			&Tbin, &pRbin, &pDbin, &Xbin, &WXbin, &Ybin, &WYbin, &Nbins,  binning_step) ) {
      return(FALSE);
    }
    if (!weighted_LS_PLX(Tbin,pRbin,pDbin,Xbin,WXbin,Ybin,WYbin, Nbins,
			 A,B,VERBOSE)) {
      return(FALSE);
    }

    FREE(Tbin);
    FREE(pRbin);
    FREE(pDbin);
    FREE(Xbin);
    FREE(Ybin);
    FREE(WXbin);
    FREE(WYbin);
  }


  // Calculate r vector of residuals and least squares sigma
  sigma_ols = 0.0;
  for (i = 0; i < Npts; i++) {
    // double Xf = B[0][0] + B[1][0]*T[i] + B[4][0]*pR[i];
    // double Yf = B[2][0] + B[3][0]*T[i] + B[4][0]*pD[i];

    rx[i] = X[i] - (T[i] * B[1][0] + B[0][0] + B[4][0] * pR[i]);
    ry[i] = Y[i] - (T[i] * B[3][0] + B[2][0] + B[4][0] * pD[i]);


    //    u[i] = r[i] /
    sigma_ols += SQ(rx[i]) + SQ(ry[i]);
  }
  sigma_ols = sqrt(sigma_ols / (Npts - 5.0));
  
  // Save OLS covariance;
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      Cov[i][j] = A[i][j];
    }
  }

  // Save Beta
  for (i = 0; i < 5; i++) {
    Beta[i] = B[i][0];
  }

  // Iteratively reweight and solve
  converged = FALSE;
  iterations = 0;
  sigma_hat = 0.0;

  do {
    if (VERBOSE == 2) {
      fprintf(stderr,"Iteration: %d (%f %f %f %f %f) sigmas: %f %f\n",
	      iterations,
	      Beta[0],Beta[1],Beta[2],Beta[3],Beta[4],
	      sigma_ols, sigma_hat);
    }
    // Save Beta
    for (i = 0; i < 5; i++) {
      Beta_prev[i] = Beta[i];
    }

    // Assign W
    for (i = 0; i < Npts; i++) {
      Wx[i] = weight_cauchy(rx[i] / dX[i]);
      Wy[i] = weight_cauchy(ry[i] / dY[i]);
      qx[i] = Qx[i] * Wx[i];
      qy[i] = Qy[i] * Wy[i];
    }

    // Solve
    if (!weighted_LS_PLX(T,pR,pD,X,qx,Y,qy,Npts,A,B,VERBOSE)) {
      // Handle fail case
      return(FALSE);
    }

    // Save Beta    
    for (i = 0; i < 5; i++) {
      Beta[i] = B[i][0];
    }

    // Calculate r vector of residuals
    for (i = 0; i < Npts; i++) {
      rx[i] = X[i] - (T[i] * B[1][0] + B[0][0] + B[4][0] * pR[i]);
      ry[i] = Y[i] - (T[i] * B[3][0] + B[2][0] + B[4][0] * pD[i]);
      u[i] = sqrt(SQ(rx[i] / dX[i]) + SQ(ry[i] / dY[i]));
    }

    // Calculate sigma_hat from distribution of residual magnitudes
    sigma_hat = MedianAbsDeviation(u,Npts) / 0.6745;

    // Check convergence
    converged = TRUE;

    for (i = 0; i < 5; i++) {
      if (fabs(Beta[i] - Beta_prev[i]) > tolerance * abs(Beta[i])) {
	converged = FALSE;
      }
    }

    iterations++;
    if (iterations >= max_iterations) {
      converged = TRUE;
      // Throw a warning or something here.
    }

  } while (!converged);

  double ax, ay;
  double bx, by;
  double lambda_x, lambda_y;
  double sigma_robust_x, sigma_robust_y;
  double sigma_final_x,  sigma_final_y;
  double Sum_Wx, Sum_Wy;

  Sum_Wx = 0.0;
  Sum_Wy = 0.0;
  ax = 0.0; ay = 0.0;
  bx = 0.0; by = 0.0;

  for (i = 0; i < Npts; i++) {
    Wx[i] = weight_cauchy(rx[i] / dX[i]);
    Wy[i] = weight_cauchy(ry[i] / dY[i]);

    ax += dpsi_cauchy(rx[i] / dX[i]);
    ay += dpsi_cauchy(ry[i] / dY[i]);

    bx += SQ(Wx[i]);
    by += SQ(Wy[i]);

    Sum_Wx += Wx[i];
    Sum_Wy += Wy[i];
  }
  ax /= 1.0 * Npts;  // mean(psi_dot(r))
  ay /= 1.0 * Npts;
  bx /= 1.0 * (Npts - p); // mean(psi^2(r)) * (N / (N-p))
  by /= 1.0 * (Npts - p);

  lambda_x = 1.0 + (p / Npts) * (1 - ax) / ax;
  lambda_y = 1.0 + (p / Npts) * (1 - ay) / ay;
  
  sigma_robust_x = lambda_x * sqrt(bx) * sigma_hat * 2.385 / ax;
  sigma_robust_y = lambda_y * sqrt(by) * sigma_hat * 2.385 / ay;

  // This is actually sigma^2, as that's the factor in the covariance (dumouchel 4.1)
  sigma_final_x  = MAX(SQ(sigma_robust_x), (Npts * SQ(sigma_robust_x) + SQ(p * sigma_ols)) / (Npts + SQ(p)));
  sigma_final_y  = MAX(SQ(sigma_robust_y), (Npts * SQ(sigma_robust_y) + SQ(p * sigma_ols)) / (Npts + SQ(p)));

  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      // This uses the original OLS covariance.
      if ((i < 2)&&(j < 2)) { // Upper portion
	Cov[i][j] *= sigma_final_x;
      }
      else if ((i < 4)&&(j < 4)) { // Lower portion
	Cov[i][j] *= sigma_final_y;
      }
      else { // Cross term
	Cov[i][j] *= sqrt(sigma_final_x * sigma_final_y);
      }
    }
  }

  // Finish
  fit[0].Ro = B[0][0];
  fit[0].uR = B[1][0];
  fit[0].Do = B[2][0];
  fit[0].uD = B[3][0];
  fit[0].p  = B[4][0];
  
  fit[0].dRo = sqrt(Cov[0][0]);
  fit[0].duR = sqrt(Cov[1][1]);
  fit[0].dDo = sqrt(Cov[2][2]);
  fit[0].duD = sqrt(Cov[3][3]);
  fit[0].dp  = sqrt(Cov[4][4]);
  
  // (optionally) add up the chi square for the fit
  if (fit->getChisq) {
    chisq = 0.0;
    fit[0].Nfit = 0;

    double wx,wy;
    double lim_Wx = outlier_limit * Sum_Wx / (1.0 * Npts);
    double lim_Wy = outlier_limit * Sum_Wy / (1.0 * Npts);
    for (i = 0; i < Npts; i++) {
      int skip = (Wx[i] < lim_Wx) || (Wy[i] < lim_Wy);
      if (!skip) {
	Xf = fit[0].Ro + fit[0].uR*T[i] + fit[0].p*pR[i];
	Yf = fit[0].Do + fit[0].uD*T[i] + fit[0].p*pD[i];
	wx = (fabs(dX[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dX[i]);
	wy = (fabs(dY[i]) < 0.0001) ? 1.0 : 1.0 / SQ(dY[i]);
	chisq += SQ(X[i] - Xf) * wx;
	chisq += SQ(Y[i] - Yf) * wy;
	fit[0].Nfit += 1;
      // if (VERBOSE) fprintf (stderr, "chisq contrib : %f %f : %f %f : %f %f : %f %f : %f\n", Xf, Yf, X[i] - Xf, Y[i] - Yf, dX[i], dY[i], (X[i] - Xf) / dX[i], (Y[i] - Yf) / dY[i], chisq);
      }
    }
    // the reduced chisq is divided by (Ndof = 2*Npts - 5)
    fit[0].chisq = chisq / (2.0*Npts - 5.0);
  }
  else {
    fit[0].Nfit = Npts;
  }
  
  return (TRUE);
}

int weighted_LS_PLX (double *T, double *pR, double *pD, double *X, double *WX, double *Y, double *WY, int Npts,
		     double **A, double **B, int VERBOSE) {

  int i;
  double wx, wy, Wx, Wy, Tx, Ty, Tx2, Ty2, Xs, Ys, XT, YT;
  double PR, PD, PRT, PDT, PRX, PDY, PR2, PD2;

  PR = PD = PRT = PDT = PRX = PDY = PR2 = PD2 = 0.0;
  Wx = Wy = Tx = Ty = Tx2 = Ty2 = Xs = Ys = XT = YT = 0.0;
  for (i = 0; i < Npts; i++) {

    if (VERBOSE == 2) fprintf (stderr, "%f %f : %f %f : %f : %f %f\n", X[i], WX[i], Y[i], WY[i], T[i], pR[i], pD[i]);

    wx = WX[i];
    wy = WY[i];

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

  // A => (X^T W X)^{-1}
  // B => beta

  return TRUE;
}

int bin_points_PLX (double *T, double *pR, double *pD, double *X, double *WX, double *Y, double *WY, int Npts,
		    double **Tbin, double **pRbin, double **pDbin, double **Xbin, double **WXbin, double **Ybin, double **WYbin, int *Nbins, double binning_step) {
  double *T_tmp = NULL;
  int i,j,k,l;

  int Nbins_test;
  int *bin_test = NULL;

  double *T_int = NULL, *pD_int = NULL, *pR_int = NULL, *X_int = NULL, *Y_int = NULL, *WX_int = NULL, *WY_int = NULL;
  double T_min; // not used , T_max;
  
  // Allocate more bins than we need
  ALLOCATE(T_tmp,double,Npts);
  for (i = 0; i < Npts; i++) {
    T_tmp[i] = T[i];
  }
  dsort(T_tmp,Npts);

  Nbins_test = floor((T_tmp[Npts - 1] - T_tmp[0]) / binning_step) + 1;
  ALLOCATE(bin_test,int,Nbins_test);

  T_min = T_tmp[0];
  // not used T_max = T_tmp[Npts - 1];
  
  // Check how many bins are filled.
  for (j = 0; j < Nbins_test; j++) {
    bin_test[j] = 0;
  }
  for (i = 0; i < Npts; i++) {
    j = floor((T_tmp[i] - T_tmp[0]) / binning_step);
    bin_test[j] ++;
  }

  *Nbins = 0;
  for (j = 0; j < Nbins_test; j++) {
    if (bin_test[j] != 0) {
      *Nbins = *Nbins + 1;
    }
  }

  double *Tbin_in = NULL;
  double *pRbin_in = NULL;
  double *pDbin_in = NULL;
  double *Xbin_in = NULL;
  double *Ybin_in = NULL;
  double *WXbin_in = NULL;
  double *WYbin_in = NULL;
  
  ALLOCATE(Tbin_in,double,*Nbins);
  ALLOCATE(pRbin_in, double, *Nbins);
  ALLOCATE(pDbin_in, double, *Nbins);
  ALLOCATE(Xbin_in,double,*Nbins);
  ALLOCATE(WXbin_in,double,*Nbins);
  ALLOCATE(Ybin_in,double,*Nbins);
  ALLOCATE(WYbin_in,double,*Nbins);

  k = 0;
  for (j = 0; j < Nbins_test; j++) {
    if (bin_test[j] == 0) { // No data for this bin
      continue;
    }
    else {
      // Allocate internal arrays to hold the data that goes into this bin.
      ALLOCATE(T_int,double, bin_test[j]);
      ALLOCATE(pR_int,double, bin_test[j]);
      ALLOCATE(pD_int,double, bin_test[j]);
      ALLOCATE(X_int,double, bin_test[j]);
      ALLOCATE(Y_int,double, bin_test[j]);
      ALLOCATE(WX_int,double, bin_test[j]);
      ALLOCATE(WY_int,double, bin_test[j]);

      // Fill those arrays.
      l = 0;
      for (i = 0; i < Npts; i++) {
	if ((T[i] >= T_min + j * binning_step)&&
	    (T[i] <  T_min + (j + 1) * binning_step)) {
	  T_int[l] = T[i];
	  pR_int[l] = pR[i];
	  pD_int[l] = pD[i];
	  X_int[l] = X[i];
	  Y_int[l] = Y[i];
	  WX_int[l] = WX[i];
	  WY_int[l] = WY[i];
	  l++;
	  //	  printf("bin: %d Points: %d Point: %d %d %f %f %f : %f %f\n",j,bin_test[j],l,i,T[i],X[i],Y[i],
	  //	 T_min + j * binning_step,T_min + (j+1) * binning_step);
	}
      } // End loop over input points

      // Make a decision what the binned values should be.
      if (l == 1) {
	Tbin_in[k] = T_int[0];
	Xbin_in[k] = X_int[0];
	Ybin_in[k] = Y_int[0];
	pRbin_in[k] = pR_int[0];
	pDbin_in[k] = pD_int[0];
	WXbin_in[k] = WX_int[0];
	WYbin_in[k] = WY_int[0];
      }
      else {
	// I think I'm going with medians.
	dsort(T_int,l);
	dsort(pR_int,l);
	dsort(pD_int,l);
	dsort(X_int,l);
	dsort(Y_int,l);
	
	// The median gives the midpoint of all the measurements
	if ((l % 2) == 0) {
	  Tbin_in[k] = 0.5*(T_int[(int)(0.5*l)] + T_int[(int)(0.5*l) - 1]);
	  pRbin_in[k] = 0.5*(pR_int[(int)(0.5*l)] + pR_int[(int)(0.5*l) - 1]);
	  pDbin_in[k] = 0.5*(pD_int[(int)(0.5*l)] + pD_int[(int)(0.5*l) - 1]);
	  Xbin_in[k] = 0.5*(X_int[(int)(0.5*l)] + X_int[(int)(0.5*l) - 1]);
	  Ybin_in[k] = 0.5*(Y_int[(int)(0.5*l)] + Y_int[(int)(0.5*l) - 1]);
	} else {
	  Tbin_in[k] = T_int[(int)(0.5*l)];
	  pDbin_in[k] = pD_int[(int)(0.5*l)];
	  pRbin_in[k] = pR_int[(int)(0.5*l)];
	  Xbin_in[k] = X_int[(int)(0.5*l)];
	  Ybin_in[k] = Y_int[(int)(0.5*l)];
	}
	
	// The scatter between points is probably the most useful measurement for the error.
	for (i = 0; i < l; i++) {
	  X_int[i] = fabs(X_int[i] - Xbin_in[k]);
	  Y_int[i] = fabs(Y_int[i] - Ybin_in[k]);
	}
	dsort(X_int,l);
	dsort(Y_int,l);
	
	if ((l % 2) == 0) {
	  WXbin_in[k] = 1.0 / (0.5*(X_int[(int)(0.5*l)] + X_int[(int)(0.5*l) - 1]));
	  WYbin_in[k] = 1.0 / (0.5*(Y_int[(int)(0.5*l)] + Y_int[(int)(0.5*l) - 1]));
	} else {
	  WXbin_in[k] = 1.0 / (X_int[(int)(0.5*l)]);
	  WYbin_in[k] = 1.0 / (Y_int[(int)(0.5*l)]);
	}
      }
      
      if (WXbin_in[k] == 0.0) {
	WXbin_in[k] = WX_int[0];
      }
      if (WYbin_in[k] == 0.0) {
	WYbin_in[k] = WY_int[0];
      }
      if (!isfinite(WXbin_in[k])) {
	WXbin_in[k] = 1.0;
      }
      if (!isfinite(WYbin_in[k])) {
	WYbin_in[k] = 1.0;
      }
      
      // Increment bin index.
      k++;
      
      // Clean up
      FREE(T_int);
      FREE(pR_int);
      FREE(pD_int);
      FREE(X_int);
      FREE(Y_int);
      FREE(WX_int);
      FREE(WY_int);	    
    }
  } // End loop over initial bin set.

  *Tbin = Tbin_in;
  *pRbin = pRbin_in;
  *pDbin = pDbin_in;
  *Xbin = Xbin_in;
  *Ybin = Ybin_in;
  *WXbin = WXbin_in;
  *WYbin = WYbin_in;

  FREE(T_tmp);
  FREE(bin_test);
  
  return TRUE;
}

