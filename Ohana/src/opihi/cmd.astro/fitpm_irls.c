# include "astro.h"

int fitpm_irls (int argc, char **argv) {
  
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

  double binning_step = 0.0;
  if ((N = get_argument (argc, argv, "-binning"))) {
    remove_argument (N, &argc, argv);
    binning_step = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  
  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fitplx_irls (ra) (dR) (dec) (dD) (mjd) [-mask mask] [-binning step_size]\n");
    // what about the errors?
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

  N = tvec->Nelements; // XXX check other lengths

  // find mean values to remove
  double Npts = 0;
  double Tmean = 0;
  double Rmean = 0;
  double Dmean = 0;
  double Tmin = +1000000;
  double Tmax = -1000000;
  for (i = 0; i < N; i++) {
    if (mask && !mask[i]) continue;
    Rmean += R[i];
    Dmean += D[i];
    Tmean += T[i];
    Tmin = MIN(Tmin, T[i]);
    Tmax = MAX(Tmax, T[i]);
    Npts += 1.0;
  }
  Rmean /= Npts;
  Dmean /= Npts;
  Tmean /= Npts;

  float Trange = Tmax - Tmin;
  // fprintf (stderr, "R,D : %f,%f, T: %f, Trange: %f, Tmin: %f, Tmax: %f\n", Rmean, Dmean, Tmean, Trange, Tmin, Tmax);

  /* project coordinates to a plane centered on the object with units of arcsec */
  Coords coords;
  InitCoords (&coords, "DEC--SIN");
  coords.crval1 = Rmean;
  coords.crval2 = Dmean;
  coords.cdelt1 = coords.cdelt2 = 1.0 / 3600.0;

  double *X, *Y, *t, *dX, *dY;
  ALLOCATE (X, double, N);
  ALLOCATE (Y, double, N);
  ALLOCATE (dX, double, N);
  ALLOCATE (dY, double, N);
  ALLOCATE (t, double, N);

  int n = 0;
  for (i = 0; i < N; i++) {
    if (mask && !mask[i]) continue;
    RD_to_XY (&X[n], &Y[n], R[i], D[i], &coords);
    dX[n] = dR[i];
    dY[n] = dD[i];
    t[n] = (T[i] - Tmean) / 365.25;
    n++;
  }

  PlxFit fit;
  if (!FitPMonly_IRLS (&fit, X, dX, Y, dY, t, n, binning_step, VERBOSE)) {
    return FALSE;
  }

  // fprintf (stderr, "Roff, Doff: %f, %f; dRo, dDo: %f, %f\n", fit.Ro, fit.Do, fit.dRo, fit.dDo);
  
  XY_to_RD (&Rmean, &Dmean, fit.Ro, fit.Do, &coords);
  if (VERBOSE) {
    fprintf (stderr, "Ro, Do: %f, %f +/- %f, %f\n", Rmean, Dmean, fit.dRo, fit.dDo);
    fprintf (stderr, "uR, uD: %f, %f; duR, duD: %f, %f\n", fit.uR, fit.uD, fit.duR, fit.duD);
    fprintf (stderr, "chisq: %f Nfit %d\n", fit.chisq, fit.Nfit);
  }

  set_variable ("RA",   Rmean);
  set_variable ("DEC",  Dmean);
  set_variable ("dR",   fit.dRo);
  set_variable ("dD",   fit.dDo);
  set_variable ("uR",   fit.uR);
  set_variable ("uD",   fit.uD);
  set_variable ("duR",   fit.duR);
  set_variable ("duD",   fit.duD);
  set_variable ("plx",  0.0);
  set_variable ("dplx", 0.0);
  
  set_variable ("Tmean",  Tmean);
  set_variable ("Trange", Trange);
  set_variable ("Prange", 0.0);

  set_variable ("chisq", fit.chisq);
  set_variable ("Nfit",  fit.Nfit);

  return (TRUE);
}

/* do we want an init function which does the alloc and a clear function to free? */
int FitPMonly_IRLS (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, int Npts, double binning_step, int VERBOSE) {

  int i,j;

  static double **A, **B;


  double chisq, Xf, Yf;

  double **Cov;
  double *Beta, *Beta_prev;
  
  double sigma_ols, sigma_hat;
  double *Wx, *Wy;
  double *rx, *ry;
  //  double *ux, *uy;
  double *u;
  int dof = 2 * Npts - 4;
  int p   = 4;
  int n   = 2 * Npts;
  double tolerance;
  int converged;
  int iterations;
  
  /* do I need to do this as 2 2x2 matrix equations? */
  if (A == NULL) {
    ALLOCATE (A, double *, 4);
    ALLOCATE (B, double *, 4);
    for (i = 0; i < 4; i++) {
      ALLOCATE (A[i], double, 4);
      ALLOCATE (B[i], double, 1);
      memset (A[i], 0, 4*sizeof(double));
      memset (B[i], 0, 1*sizeof(double));
    }
  }

  // things we need
  ALLOCATE (Cov, double *, 4);
  for (i = 0; i < 4; i++) {
    ALLOCATE ( Cov[i], double, 4);
  }

  ALLOCATE(Beta, double, 4);
  ALLOCATE(Beta_prev, double, 4);
  ALLOCATE(Wx, double, Npts);
  ALLOCATE(Wy, double, Npts);
  ALLOCATE(rx,  double, Npts);
  ALLOCATE(ry,  double, Npts);
  ALLOCATE(u,  double, Npts);
  
  // Convert the measurement errors into initial weights.
  for (i = 0; i < Npts; i++) {
    Wx[i] = 1 / dX[i];
    Wy[i] = 1 / dY[i];
  }
  
  // Solve OLS equation
  if (binning_step == 0.0) {
    if (!weighted_LS_PM(T,X,Wx,Y,Wy,Npts,
			A,B,VERBOSE)) {
      // Handle fail case
      return(FALSE);
    }
  }
  else {
    int Nbins;
    double *Tbin = NULL;
    double *Xbin = NULL;
    double *Ybin = NULL;
    double *WXbin = NULL;
    double *WYbin = NULL;
    
    if (!bin_points(T, X, Wx, Y, Wy, Npts,
		    &Tbin, &Xbin, &WXbin, &Ybin, &WYbin, &Nbins,  binning_step) ) {
      return(FALSE);
    }
    if (!weighted_LS_PM(Tbin,Xbin,WXbin,Ybin,WYbin, Nbins,
			A,B,VERBOSE)) {
      return(FALSE);
    }
    FREE(Tbin);
    FREE(Xbin);
    FREE(Ybin);
    FREE(WXbin);
    FREE(WYbin);
  }

  // Calculate r vector of residuals and least squares sigma
  sigma_ols = 0.0;
  for (i = 0; i < Npts; i++) {
    rx[i] = X[i] - (T[i] * B[1][0] + B[0][0]);
    ry[i] = Y[i] - (T[i] * B[3][0] + B[2][0]);
    //    u[i] = r[i] /
    sigma_ols += SQ(rx[i]) + SQ(ry[i]);

  }
  sigma_ols = sqrt(sigma_ols / dof);

  // Save OLS covariance;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      Cov[i][j] = A[i][j];
    }
  }

  // Save Beta
  for (i = 0; i < 4; i++) {
    Beta[i] = B[i][0];
  }

  // Iterately reweight and solve
  converged = FALSE;
  iterations = 0;
  do {
    // Save Beta.
    for (i = 0; i < 4; i ++) {
      Beta_prev[i] = Beta[i];
    }

    // Assign W
    for (i = 0; i < Npts; i++) {
      Wx[i] = weight_cauchy(rx[i] / dX[i]);
      Wy[i] = weight_cauchy(ry[i] / dY[i]);
    }    

    // Solve
    if (!weighted_LS_PM(T,X,Wx,Y,Wy,Npts,
		     A,B,VERBOSE)) {
      // Handle fail case
      return(FALSE);
    }

    for (i = 0; i < 4; i++) {
      Beta[i] = B[i][0];
    }

    // r
    sigma_hat = 0.0;
    for (i = 0; i < Npts; i++) {
      rx[i] = X[i] - (T[i] * B[1][0] + B[0][0]);
      ry[i] = Y[i] - (T[i] * B[3][0] + B[2][0]);
      u[i] = sqrt(SQ(rx[i] / dX[i]) + SQ(ry[i] / dY[i]));
    }
    sigma_hat = MedianAbsDeviation(u,Npts) / 0.6745;
    
    // Check convergence
    converged = TRUE;
    tolerance = 1e-4;  // This should probably be tunable.
    for (i = 0; i < 4; i++) {
      if (fabs(Beta[i] - Beta_prev[i]) > tolerance * abs(Beta[i])) {
	converged = FALSE;
      }
    }

    iterations++;
    if (iterations >= 10) {
      converged = TRUE;
      // Throw a warning or something here.
    }
    
  } while (!converged);

  double ax, ay;
  double bx, by;
  double lambda;
  double sigma_robust_x, sigma_robust_y;
  double sigma_final_x,  sigma_final_y;
  double Sum_Wx, Sum_Wy;
  
  Sum_Wx = 0.0;
  Sum_Wy = 0.0;
  ax = 0.0; ay = 0.0;
  bx = 0.0; by = 0.0;
  lambda = 0.0;
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
  
  sigma_robust_x = lambda * sqrt(bx) * sigma_hat * 2.385 / ax;
  sigma_robust_y = lambda * sqrt(by) * sigma_hat * 2.385 / ay;

  // This is actually sigma^2, as that's the factor in the covariance (dumouchel 4.1)
  sigma_final_x  = MAX(SQ(sigma_robust_x), (n * SQ(sigma_robust_x) + SQ(p * sigma_ols)) / (n + SQ(p)));
  sigma_final_y  = MAX(SQ(sigma_robust_y), (n * SQ(sigma_robust_y) + SQ(p * sigma_ols)) / (n + SQ(p)));

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      // This uses the original OLS covariance.
      if ((i < 2)&&(j < 2)) { // Upper portion
	Cov[i][j] *= sigma_final_x;
      }
      else if ((i > 1)&&(j > 1)) { // Lower portion
	Cov[i][j] *= sigma_final_y;
      }
      else { // Cross term
	Cov[i][j] *= sqrt(sigma_final_x * sigma_final_y);
      }
    }
  }

  // Finish.
  fit[0].Ro = Beta[0];
  fit[0].uR = Beta[1];
  fit[0].Do = Beta[2];
  fit[0].uD = Beta[3];
  
  fit[0].dRo = sqrt(Cov[0][0]);
  fit[0].duR = sqrt(Cov[1][1]);
  fit[0].dDo = sqrt(Cov[2][2]);
  fit[0].duD = sqrt(Cov[3][3]);

  // Sort out the final weight threshold.

  // add up the chi square for the fit
  chisq = 0.0;
  fit[0].Nfit = 0;
  for (i = 0; i < Npts; i++) {
    if ((Wx[i] > 0.1 * Sum_Wx / (1.0 * Npts))||
	(Wy[i] > 0.1 * Sum_Wy / (1.0 * Npts))) {
      Xf = fit[0].Ro + fit[0].uR*T[i];
      Yf = fit[0].Do + fit[0].uD*T[i];
      chisq += SQ(X[i] - Xf) / SQ(dX[i]);
      chisq += SQ(Y[i] - Yf) / SQ(dY[i]);
      fit[0].Nfit += 1;
    }
    // if (VERBOSE) fprintf (stderr, "chisq contrib : %f %f : %f %f : %f %f : %f %f : %f\n", Xf, Yf, X[i] - Xf, Y[i] - Yf, dX[i], dY[i], (X[i] - Xf) / dX[i], (Y[i] - Yf) / dY[i], chisq);
  }
  //  fit[0].Nfit = Npts;

  // the reduced chisq is divided by (Ndof = 2*Npts - 4)
  fit[0].chisq = chisq / (2.0*Npts - 4.0);
  return (TRUE);
}

int weighted_LS_PM (double *T, double *X, double *WX, double *Y, double *WY, int Npts, double **A, double **B, int VERBOSE) {

  int i,j;
  double Wx, Wy, Tx, Ty, Tx2, Ty2, Xs, Ys, XT, YT;
  Wx = Wy = Tx = Ty = Tx2 = Ty2 = Xs = Ys = XT = YT = 0.0;
  for (i = 0; i < Npts; i++) {
    Wx += WX[i];
    Wy += WY[i];

    Tx += T[i]*WX[i];
    Ty += T[i]*WY[i];
    
    Tx2 += SQ(T[i])*WX[i];
    Ty2 += SQ(T[i])*WY[i];
    
    Xs += X[i]*WX[i];
    Ys += Y[i]*WY[i];

    XT += X[i]*T[i]*WX[i];
    YT += Y[i]*T[i]*WY[i];
  }

  // X^T W X
  A[0][0] = Wx;
  A[0][1] = Tx;

  A[1][0] = Tx;
  A[1][1] = Tx2;

  A[2][2] = Wy;
  A[2][3] = Ty;

  A[3][2] = Ty;
  A[3][3] = Ty2;

  // X^T W Y
  B[0][0] = Xs;
  B[1][0] = XT;
  B[2][0] = Ys;
  B[3][0] = YT;

  if (!dgaussjordan ((double **)A, (double **)B, 4, 1)) {
    if (VERBOSE) fprintf (stderr, "error in fit\n");
    if (VERBOSE == 2) {
      for (i = 0; i < 4; i++) {
	for (j = 0; j < 4; j++) {
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

int bin_points (double *T, double *X, double *WX, double *Y, double *WY, int Npts,
		double **Tbin, double **Xbin, double **WXbin, double **Ybin, double **WYbin, int *Nbins, double binning_step) {
  double *T_tmp = NULL;
  int i,j,k,l;

  int Nbins_test;
  int *bin_test = NULL;

  double *T_int = NULL, *X_int = NULL, *Y_int = NULL, *WX_int = NULL, *WY_int = NULL;
  double T_min; // not used , T_max;


  //  printf("In binning code\n");

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

  // printf("Found %d points, putting into %d bins between %f and %f\n",Npts,Nbins_test,T_min,T_max);
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

  // printf("Using %d actual bins\n",*Nbins);

  double *Tbin_in = NULL;
  double *Xbin_in = NULL;
  double *Ybin_in = NULL;
  double *WXbin_in = NULL;
  double *WYbin_in = NULL;
  
  ALLOCATE(Tbin_in,double,*Nbins);
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
      // printf("%d bin %d, N = %d\n",k,j,bin_test[j]);
      // Allocate internal arrays to hold the data that goes into this bin.
      ALLOCATE(T_int,double, bin_test[j]);
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
	  X_int[l] = X[i];
	  Y_int[l] = Y[i];
	  WX_int[l] = WX[i];
	  WY_int[l] = WY[i];
	  // printf("%d %d %f %f %f %f %f\n",i,l,T[i],X[i],Y[i],WX[i],WY[i]);
	  l++;
	}
      } // End loop over input points
      // printf("%d bin %d, N = %d, l = %d\n",k,j,bin_test[j],l);

      // Make a decision what the binned values should be.
      if (l == 1) {
	Tbin_in[k] = T_int[0];
	Xbin_in[k] = X_int[0];
	Ybin_in[k] = Y_int[0];
	WXbin_in[k] = WX_int[0];
	WYbin_in[k] = WY_int[0];
      }
      else {
	// I think I'm going with medians.
	dsort(T_int,l);
	dsort(X_int,l);
	dsort(Y_int,l);
	
	// The median gives the midpoint of all the measurements
	if ((l % 2) == 0) {
	  Tbin_in[k] = 0.5*(T_int[(int)(0.5*l)] + T_int[(int)(0.5*l) - 1]);
	  Xbin_in[k] = 0.5*(X_int[(int)(0.5*l)] + X_int[(int)(0.5*l) - 1]);
	  Ybin_in[k] = 0.5*(Y_int[(int)(0.5*l)] + Y_int[(int)(0.5*l) - 1]);
	} else {
	  Tbin_in[k] = T_int[(int)(0.5*l)];
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
      
      //      printf("%f %f %f %f %f\n",Tbin_in[k],Xbin_in[k],Ybin_in[k],WXbin_in[k],WYbin_in[k]);
      // Increment bin index.
      k++;
      
      // Clean up
      FREE(T_int);
      FREE(X_int);
      FREE(Y_int);
      FREE(WX_int);
      FREE(WY_int);
    } 
  } // End loop over initial bin set.

  *Tbin = Tbin_in;
  *Xbin = Xbin_in;
  *Ybin = Ybin_in;
  *WXbin = WXbin_in;
  *WYbin = WYbin_in;

  FREE(T_tmp);
  FREE(bin_test);
  
  return TRUE;
}


double weight_cauchy (double x) {
  double r = x / 2.385;
  return (1.0 / (1.0 + SQ(r)));
}

// dpsi = (d/dx) (x * weight(x))
double dpsi_cauchy (double x) {
  double r2 = SQ(x / 2.385);
  return ((1.0 - r2) / (SQ(1 + r2)));
}


// median absolute deviation
// MAD = median(abs(x - median(x)))
double MedianAbsDeviation(double *in, int N) {
  double *x;
  double median = 0.0;
  int i;
  
  ALLOCATE(x,double,N);
  for (i = 0; i < N; i++) {
    x[i] = in[i];
  }

  dsort(x,N);

  if ((N % 2) == 0) {
    median = 0.5*(x[(int)(0.5*N)] + x[(int)(0.5*N) - 1]);
  } else {
    median = x[(int)(0.5*N)];
  }

  for (i = 0; i < N; i++ ) {
    x[i] = fabs(x[i] - median);
  }

  dsort(x,N);

  if ((N % 2) == 0) {
    median = 0.5*(x[(int)(0.5*N)] + x[(int)(0.5*N) - 1]);
  } else {
    median = x[(int)(0.5*N)];
  }

  FREE(x);
  return(median);
}
  
