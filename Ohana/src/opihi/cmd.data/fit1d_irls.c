# include "data.h"

// These should probably be tunable:
# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6
# define WEIGHT_THRESHOLD 0.3

int fit_least_squares (double *s, double **b, double **c, int nterm, int mterm, opihi_flt *x, opihi_flt *y, opihi_flt *dy, opihi_flt *wt, char *mask, int Npts);
int get_yfit (opihi_flt *x, opihi_flt *y, opihi_flt *yf, opihi_flt *yoff, int Npts, int order, double **b);
double weight_cauchy (double x);
double VectorFractionInterpolate (double *values, float fraction, int Npts);

// using file-globals to carry values into 'fit_least_squares'
double *priorValue = NULL;
double *priorStdev = NULL;

int fit1d_irls (int argc, char **argv) {
  
  int i, nterm, mterm, order, N;
  Vector *xvec, *yvec, *dyvec;
  char name[64];

  int Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    Quiet = TRUE;
  }

  dyvec = NULL;
  if ((N = get_argument (argc, argv, "-dy"))) {
    remove_argument (N, &argc, argv);
    if ((dyvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  Vector *wtVec = NULL;
  if ((N = get_argument (argc, argv, "-wt"))) {
    remove_argument (N, &argc, argv);
    if (!(wtVec = SelectVector (argv[N], ANYVECTOR, TRUE))) return FALSE;
    remove_argument (N, &argc, argv);
  }

  Vector *yfitVec = NULL;
  if ((N = get_argument (argc, argv, "-yfit"))) {
    remove_argument (N, &argc, argv);
    if (!(yfitVec = SelectVector (argv[N], ANYVECTOR, TRUE))) return FALSE;
    remove_argument (N, &argc, argv);
  }
  Vector *mkVec = NULL;
  if ((N = get_argument (argc, argv, "-mask"))) {
    remove_argument (N, &argc, argv);
    if (!(mkVec = SelectVector (argv[N], ANYVECTOR, TRUE))) return FALSE;
    remove_argument (N, &argc, argv);
  }

  int NBOOTSTRAP = 100;
  if ((N = get_argument (argc, argv, "-Nboot"))) {
    remove_argument (N, &argc, argv);
    NBOOTSTRAP = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  int MaxIterations = 10;
  if ((N = get_argument (argc, argv, "-max-iterations"))) {
    remove_argument (N, &argc, argv);
    MaxIterations = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  float WeightThreshold = 0.3;
  if ((N = get_argument (argc, argv, "-weight-threshold"))) {
    remove_argument (N, &argc, argv);
    WeightThreshold = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  int UseMean = TRUE;
  if ((N = get_argument (argc, argv, "-use-median"))) {
    remove_argument (N, &argc, argv);
    UseMean = FALSE;
  }
  int UsePriors = FALSE;
  if ((N = get_argument (argc, argv, "-use-priors"))) {
    remove_argument (N, &argc, argv);
    UsePriors = TRUE;
  }

  if ((argc != 4) || !dyvec) {
    // require a y error value
    gprint (GP_ERR, "USAGE: fit1d_irls x y order [-dy wt] [-quiet/-q] [-wt wtOut] [-yfit yfit] [-mask maskOut] [-Nboot N] [-max-iterations N] [-weight-threshold f] [-use-median] [-use-priors]\n");
    gprint (GP_ERR, "  [-dy wt]        : optional error vector (use value of 1 by default)\n");
    gprint (GP_ERR, "  [-quiet/-q]     : reduce verbosity\n");
    gprint (GP_ERR, "  [-wt wtOut]     : save the weights in supplied vector\n");
    gprint (GP_ERR, "  [-yfit yfit]    : save the fitted values in supplied vector\n");
    gprint (GP_ERR, "  [-mask maskOut] : save the masked / unmasked state (weight below threshold\n");
    gprint (GP_ERR, "  [-Nboot N]      : number of bootstrap samples\n");
    gprint (GP_ERR, "  [-max-iterations N] : max number of iterations\n");
    gprint (GP_ERR, "  [-weight-threshold f] : limit for masked values\n");
    gprint (GP_ERR, "  [-use-median]   : use median (as opposed to mean) to calculate weight threshold\n");
    gprint (GP_ERR, "  [-use-priors]   : include priors for parameters (specified as prior value $P0v and stdev $dP0, $P1v, $dP1, etc).  Unspecified or NAN values are ignored.\n");
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (dyvec, OPIHI_FLT);
  if (xvec[0].Nelements != dyvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }

  if (mkVec) ResetVector (mkVec, OPIHI_INT, xvec[0].Nelements);

  /* nterm is number of polynomial terms, starting at x^0 */
  order = atof (argv[3]);
  nterm = order + 1;
  mterm = 2*order + 1;

  ALLOCATE_PTR (yoff, opihi_flt, xvec[0].Nelements);
  ALLOCATE_PTR (yfit, opihi_flt, xvec[0].Nelements);
  ALLOCATE_PTR (wt,   opihi_flt, xvec[0].Nelements);

  // ALLOCATE_PTR (mask, char, xvec[0].Nelements);
  // memset (mask, 0, xvec[0].Nelements);

  ALLOCATE_PTR (s, double, mterm);
  ALLOCATE_PTR (b, double *, nterm);
  ALLOCATE_PTR (c, double *, nterm);
  ALLOCATE_PTR (b_old, double *, nterm);
  for (i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm);
    ALLOCATE (b[i], double, 1);
    ALLOCATE (b_old[i], double, 1);
  }

  if (UsePriors) {
    ALLOCATE (priorValue, double, nterm);
    ALLOCATE (priorStdev, double, nterm);
    for (i = 0; i < nterm; i++) {
      int isFound;
      double tmpValue;
      snprintf (name, 64, "P%dv", i);
      tmpValue = get_double_variable (name, &isFound);
      priorValue[i] = isFound ? tmpValue : NAN;
      snprintf (name, 64, "dP%d", i);
      tmpValue = get_double_variable (name, &isFound);
      priorStdev[i] = isFound ? tmpValue : NAN;
      if (!isFound) priorValue[i] = NAN; // need both Pnv and dPn 
    }
  }

  // define initial weights as 1.0
  for (i = 0; i < xvec[0].Nelements; i++) { 
    wt[i] = 1.0;
  }
  opihi_flt *dy = dyvec[0].elements.Flt;

  // initial fit with ordinary least-squares
  if (!fit_least_squares (s, b, c, nterm, mterm, xvec[0].elements.Flt, yvec[0].elements.Flt, dyvec[0].elements.Flt, wt, NULL, xvec[0].Nelements)) goto escape;

  get_yfit (xvec[0].elements.Flt, yvec[0].elements.Flt, yfit, yoff, xvec[0].Nelements, order, b);

  int converged = FALSE;
  for (int iterations = 0; !converged && (iterations < MaxIterations); iterations++) {

    for (i = 0; i < xvec[0].Nelements; i++) {
      wt[i] = weight_cauchy (yoff[i] / dy[i]);
    }

    // save this solution
    for (i = 0; i < nterm; i++) {
      b_old[i][0] = b[i][0];
    }

    if (!fit_least_squares (s, b, c, nterm, mterm, xvec[0].elements.Flt, yvec[0].elements.Flt, dyvec[0].elements.Flt, wt, NULL, xvec[0].Nelements)) {
      // restore the saved solution
      for (i = 0; i < nterm; i++) {
	b[i][0] = b_old[i][0];
      }
      break;
    }

    get_yfit (xvec[0].elements.Flt, yvec[0].elements.Flt, yfit, yoff, xvec[0].Nelements, order, b);

    converged = TRUE;
    for (i = 0; i < nterm; i++) {
      if ((fabs(b[i][0] - b_old[i][0]) > FIT_TOLERANCE * fabs(b[i][0])) && 
	  (fabs(b[i][0] - b_old[i][0]) > FLT_TOLERANCE))
	converged = FALSE;
    }
  }
      
  /* XXX TEST 
  for (i = 0; i < nterm; i++) {
    snprintf (name, 64, "C%d", i);
    set_variable (name, b[i][0]);
    if (!Quiet) gprint (GP_ERR, "%f x^%d ", b[i][0], i);
    snprintf (name, 64, "dC%d", i);
    set_variable (name, sqrt(c[i][i]));
    if (!Quiet) gprint (GP_ERR, "%f x^%d ", sqrt(c[i][i]), i);
  }
  if (!Quiet) gprint (GP_ERR, "\n");
  return TRUE; */

  // save the solution. we re-use b[][] below in the bootstrap resampling
  for (i = 0; i < nterm; i++) {
    b_old[i][0] = b[i][0];
  }

  // calculate the weight thresholds to mask the bad points:
  double Sum_W = 0.0;
  ALLOCATE_PTR (wtlist, double, xvec[0].Nelements);
  for (i = 0; i < xvec[0].Nelements; i++) {
    wt[i] = weight_cauchy (yoff[i] / dy[i]);
    wtlist[i] = wt[i];
    Sum_W += wt[i];
  }
  dsort (wtlist, xvec[0].Nelements);
  int midpt = 0.5 * xvec[0].Nelements;
  double WtMedian = (xvec[0].Nelements % 2) ? wtlist[midpt] : 0.5*(wtlist[midpt] + wtlist[midpt-1]);
  double WtThreshold = UseMean ? WeightThreshold * Sum_W / (1.0 * xvec[0].Nelements) : WeightThreshold * WtMedian;
  free (wtlist);

  // generate the unmasked subset
  ALLOCATE_PTR ( xunmask, opihi_flt, xvec[0].Nelements);
  ALLOCATE_PTR ( yunmask, opihi_flt, xvec[0].Nelements);
  ALLOCATE_PTR (dyunmask, opihi_flt, xvec[0].Nelements);

  // save unmasked points
  int Npoints = 0;
  for (i = 0; i < xvec[0].Nelements; i++) {
    if (mkVec) mkVec->elements.Int[i] = 0;
    if (wt[i] < WtThreshold) {
      if (mkVec) mkVec->elements.Int[i] = 1;
      continue;
    }
     xunmask[Npoints] =  xvec[0].elements.Flt[i];
     yunmask[Npoints] =  yvec[0].elements.Flt[i];
    dyunmask[Npoints] = dyvec[0].elements.Flt[i];
    Npoints ++;
  }

  // bootstrap resampling to generate the errorbars
  ALLOCATE_PTR (xsample,   opihi_flt, Npoints);
  ALLOCATE_PTR (ysample,   opihi_flt, Npoints);
  ALLOCATE_PTR (dysample, opihi_flt, Npoints);
  ALLOCATE_PTR (dB,        double,    nterm);
  ALLOCATE_PTR (Bboot,     double *,  nterm);
  for (i = 0; i < nterm; i++) {
    ALLOCATE (Bboot[i], double, NBOOTSTRAP);
  }    

  int Nboot = 0;
  for (int iboot = 0; iboot < NBOOTSTRAP; iboot++) {
    
    // resample
    for (i = 0; i < Npoints; i++) {
      // I need to draw Npoints random entries from 'points' with replacement:
      int N = Npoints * drand48();
       xsample[i] =  xunmask[N];
       ysample[i] =  yunmask[N];
      dysample[i] = dyunmask[N];
    }

    if (!fit_least_squares (s, b, c, nterm, mterm, xsample, ysample, dysample, NULL, NULL, Npoints)) {
      continue;
    }

    for (i = 0; i < nterm; i++) {
      Bboot[i][Nboot] = b[i][0];
    }
    Nboot ++;
  }

  for (i = 0; i < nterm; i++) {
    dsort (Bboot[i], Nboot);

    double Slo = VectorFractionInterpolate (Bboot[i], 0.158655, Nboot);
    double Shi = VectorFractionInterpolate (Bboot[i], 0.841345, Nboot);
    double sigma = (Shi - Slo) / 2.0;
    dB[i] = sigma;
  }

  /* print & save basic fit parameters */
  if (!Quiet) gprint (GP_ERR, "y = ");
  for (i = 0; i < nterm; i++) {
    snprintf (name, 64, "C%d", i);
    set_variable (name, b_old[i][0]);
    if (!Quiet) gprint (GP_ERR, "%f x^%d ", b_old[i][0], i);
  }
  if (!Quiet) gprint (GP_ERR, "\n");

  if (!Quiet) gprint (GP_ERR, "    ");
  for (i = 0; i < nterm; i++) {
    snprintf (name, 64, "dC%d", i);
    set_variable (name, dB[i]);
    if (!Quiet) gprint (GP_ERR, "%f     ", dB[i]);
  }
  if (!Quiet) gprint (GP_ERR, "\n");

  set_int_variable ("Cn", order);
  set_int_variable ("Cnfit", Npoints);
  
  // I need the chisq
  // set_variable ("dC", sigma);
  // set_variable ("Cnv", (xvec[0].Nelements - Nmask));

  for (i = 0; i < nterm; i++) {
    free (b[i]);
    free (c[i]);
    free (b_old[i]);
    free (Bboot[i]);
  }
  free (Bboot);
  free (b_old);
  free (b);
  free (c);
  free (s);
  free (yoff);

  if (wtVec) {
    SetVectorValues (wtVec, wt, OPIHI_FLT, xvec[0].Nelements);
  } else {
    free (wt);
  }
  if (yfitVec) {
    SetVectorValues (yfitVec, yfit, OPIHI_FLT, xvec[0].Nelements);
  } else {
    free (yfit);
  }
  
  free (xunmask);
  free (yunmask);
  free (dyunmask);

  free (xsample);
  free (ysample);
  free (dysample);

  if (priorValue) {
    free (priorValue); priorValue = NULL;
    free (priorStdev); priorStdev = NULL;
  }

  return (TRUE);

escape:
  for (i = 0; i < nterm; i++) {
    free (b[i]);
    free (c[i]);
  }
  free (b);
  free (c);
  free (s);
  return (FALSE);
}

// wt and mask can be NULL
int fit_least_squares (double *s, double **b, double **c, int nterm, int mterm, opihi_flt *x, opihi_flt *y, opihi_flt *dy, opihi_flt *wt, char *mask, int Npts) { 

  int i, j;

  /* init registers for current pass */
  memset (s, 0, mterm*sizeof(double));
  for (i = 0; i < nterm; i++) {
    memset (c[i], 0, nterm*sizeof(double));
    memset (b[i], 0, sizeof(double));
  }

  /* perform linear fit */
  for (i = 0; i < Npts; i++, x++, y++, dy++) {
    if (mask && mask[i]) continue;
    if (!(finite(*x) && finite(*y))) continue;

    double dY = wt ? wt[i] / SQ(*dy) : 1.0 / SQ(*dy);

    double  X =  1*dY;
    double  Y = *y*dY;

    for (j = 0; j < nterm; j++) {
      s[j] += X;
      b[j][0] += Y;
      X = X * (*x);
      Y = Y * (*x);
    }
    for (j = nterm; j < mterm; j++) {
      s[j] += X;
      X = X * (*x);
    }
  }
  for (i = 0; i < nterm; i++) {
    for (j = 0; j < nterm; j++) {
      c[i][j] = s[i + j];
      if (priorValue && (i == j)) {
	if (!isnan(priorValue[i])) {
	  c[i][i] += 1.0 / SQ(priorStdev[i]);
	  b[i][0] += priorValue[i] / SQ(priorStdev[i]);
	}
      }
    }
  }
  if (!dgaussjordan (c, b, nterm, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  return TRUE;
}

int get_yfit (opihi_flt *x, opihi_flt *y, opihi_flt *yf, opihi_flt *yoff, int Npts, int order, double **b) {

  int i, j;

  /* generate fitted values */
  for (i = 0; i < Npts; i++, x++, y++, yf++, yoff++) {
    if (!finite(*x)) continue;
    *yf = 0;
    double X = 1;
    for (j = 0; j < order + 1; j++) {
      *yf += b[j][0]*X;
      X = X * (*x);
    }
    *yoff = *y - *yf;
  }
  return TRUE;
}

double weight_cauchy (double x) {
  double r = x / 2.385;
  return (1.0 / (1.0 + SQ(r)));
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
