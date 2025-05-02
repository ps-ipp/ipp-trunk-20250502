# include "astro.h"

int fitpm (int argc, char **argv) {
  
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

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fitplx (ra) (dR) (dec) (dD) (mjd) [-mask mask]\n");
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
  if (!FitPMonly (&fit, X, dX, Y, dY, t, n, VERBOSE)) {
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
int FitPMonly (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, int Npts, int VERBOSE) {

  int i;

  static double **A, **B;
  double wx, wy, Wx, Wy, Tx, Ty, Tx2, Ty2, Xs, Ys, XT, YT;
  double chisq, Xf, Yf;

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

  Wx = Wy = Tx = Ty = Tx2 = Ty2 = Xs = Ys = XT = YT = 0.0;
  for (i = 0; i < Npts; i++) {
    /* handle case where dX or dY = 0.0 */
    wx = 1.0 / SQ(dX[i]);
    wy = 1.0 / SQ(dY[i]);

    Wx += wx;
    Wy += wy;

    Tx += T[i]*wx;
    Ty += T[i]*wy;
    
    Tx2 += SQ(T[i])*wx;
    Ty2 += SQ(T[i])*wy;
    
    Xs += X[i]*wx;
    Ys += Y[i]*wy;

    XT += X[i]*T[i]*wx;
    YT += Y[i]*T[i]*wy;
  }

  A[0][0] = Wx;
  A[0][1] = Tx;

  A[1][0] = Tx;
  A[1][1] = Tx2;

  A[2][2] = Wy;
  A[2][3] = Ty;

  A[3][2] = Ty;
  A[3][3] = Ty2;

  B[0][0] = Xs;
  B[1][0] = XT;
  B[2][0] = Ys;
  B[3][0] = YT;

  if (!dgaussjordan ((double **)A, (double **)B, 4, 1)) {
    if (VERBOSE) fprintf (stderr, "error in fit\n");
    if (VERBOSE == 2) {
      int j;
      for (i = 0; i < 4; i++) {
	for (j = 0; j < 4; j++) {
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
  
  fit[0].dRo = sqrt(A[0][0]);
  fit[0].duR = sqrt(A[1][1]);
  fit[0].dDo = sqrt(A[2][2]);
  fit[0].duD = sqrt(A[3][3]);
  
  fit[0].p   = 0.0;
  fit[0].dp  = NAN;

  // add up the chi square for the fit
  chisq = 0.0;
  for (i = 0; i < Npts; i++) {
    Xf = fit[0].Ro + fit[0].uR*T[i];
    Yf = fit[0].Do + fit[0].uD*T[i];
    chisq += SQ(X[i] - Xf) / SQ(dX[i]);
    chisq += SQ(Y[i] - Yf) / SQ(dY[i]);
    // if (VERBOSE) fprintf (stderr, "chisq contrib : %f %f : %f %f : %f %f : %f %f : %f\n", Xf, Yf, X[i] - Xf, Y[i] - Yf, dX[i], dY[i], (X[i] - Xf) / dX[i], (Y[i] - Yf) / dY[i], chisq);
  }
  fit[0].Nfit = Npts;

  // the reduced chisq is divided by (Ndof = 2*Npts - 4)
  fit[0].chisq = chisq / (2.0*Npts - 4.0);
  return (TRUE);
}
