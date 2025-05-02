# include "data.h"

typedef opihi_flt FitFunc (opihi_flt, opihi_flt *, int, opihi_flt *);
typedef enum {MODE_UPPER, MODE_LOWER} SigmoidMode;

void sigmoid_fit (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar, FitFunc *FUNC);
void sigmoid_fit_irls (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int NptsAll, opihi_flt *par, int Npar, SigmoidMode mode, double limit);

opihi_flt fsigmoidDeimos (opihi_flt, opihi_flt *, int, opihi_flt *);
opihi_flt rsigmoidDeimos (opihi_flt, opihi_flt *, int, opihi_flt *);

double deimos_weight_cauchy (double x);

// use these globals to carry into sigmoid_fit
static int VERBOSE = FALSE;
static int QUIET   = FALSE;
static int MaxIterations = 10;

int deimos_fitprofile (int argc, char **argv) {

  // fit simple slit window profile model to vector
  // I am fitting a sigmoid at both edges

  // input parameters:
  // * xprofile    : extracted slit window coordinates (vector)
  // * xprofile    : extracted slit window profile (vector)
  // * Xs, Xe      : scale coordinates of window boundaries

  // output parameters:
  // * sigmoid_left_center, etc : coordinates of window boundaries

  int N;
  Vector *xprofile = NULL;
  Vector *fprofile = NULL;
  Vector *wprofile = NULL;

  QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  MaxIterations = 10;
  if ((N = get_argument (argc, argv, "-max-iterations"))) {
    remove_argument (N, &argc, argv);
    MaxIterations = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) goto usage;
  if ((xprofile = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((fprofile = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((wprofile = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xprofile->Nelements != fprofile->Nelements) goto usage;
  if (xprofile->Nelements != wprofile->Nelements) goto usage;

  // supplied guess for boundaries
  double Xs = atof(argv[4]);
  double Xe = atof(argv[5]);

  // output vectors of fits
  Vector *Lprofile = SelectVector ("sigmoid_left", ANYVECTOR, TRUE);
  if (Lprofile == NULL) return (FALSE);
  MatchVector (Lprofile, xprofile, OPIHI_FLT);
  Vector *Rprofile = SelectVector ("sigmoid_right", ANYVECTOR, TRUE);
  if (Rprofile == NULL) return (FALSE);
  MatchVector (Rprofile, xprofile, OPIHI_FLT);

  // first, use supplied Xs & Xe guess to make a guess for BckL, Sky, BckR
  
  float Sky_V = 0.0, BckL_V = 0.0, BckR_V = 0.0;
  float Sky_N = 0.0, BckL_N = 0.0, BckR_N = 0.0;
  for (int i = 0; i < xprofile->Nelements; i++) {
    if (!isfinite(wprofile->elements.Flt[i])) continue;
    if (!isfinite(fprofile->elements.Flt[i])) continue;
    float wt = 1 / SQ(wprofile->elements.Flt[i]);
    if (xprofile->elements.Flt[i] < Xs) {
      BckL_V += fprofile->elements.Flt[i]*wt;
      BckL_N += wt;
      continue;
    }
    if (xprofile->elements.Flt[i] > Xe) {
      BckR_V += fprofile->elements.Flt[i]*wt;
      BckR_N += wt;
      continue;
    }
    Sky_V += fprofile->elements.Flt[i]*wt;
    Sky_N += wt;
  }
  float Sky = Sky_V / Sky_N;
  float BckL = BckL_V / BckL_N;
  float BckR = BckR_V / BckR_N;

  // temporary storage vectors 
  opihi_flt par[4];

  float Xmid = 0.5*(Xs + Xe);

  int Npar = 4;
  par[0] = Xs;
  par[1] = 1;
  par[2] = Sky;
  par[3] = BckL;
  sigmoid_fit_irls (xprofile->elements.Flt, fprofile->elements.Flt, wprofile->elements.Flt, Lprofile->elements.Flt, xprofile->Nelements, par, Npar, MODE_LOWER, Xmid);
  set_variable ("sigmoid_left_center", par[0]);
  set_variable ("sigmoid_left_sigma",  par[1]);
  set_variable ("sigmoid_left_top",    par[2]);
  set_variable ("sigmoid_left_bottom", par[3]);

  par[0] = Xe;
  par[1] = 1;
  par[2] = Sky;
  par[3] = BckR;
  sigmoid_fit_irls (xprofile->elements.Flt, fprofile->elements.Flt, wprofile->elements.Flt, Rprofile->elements.Flt, xprofile->Nelements, par, Npar, MODE_UPPER, Xmid);
  set_variable ("sigmoid_right_center", par[0]);
  set_variable ("sigmoid_right_sigma",  par[1]);
  set_variable ("sigmoid_right_top",    par[2]);
  set_variable ("sigmoid_right_bottom", par[3]);
 
  return TRUE;
  
usage:
  gprint (GP_ERR, "USAGE: deimos fitprofile xprof fprof wprof Xs Xe\n");
  return FALSE;
}

/* pars: x_o, sigma, top, bottom */
opihi_flt rsigmoidDeimos (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  double z = (x - par[0])/par[1];
  double r = exp (z);
  double s = 1 + r;
  double q = 1 / s;
  double f = par[2]*q + par[3];

  // df/dp0 = df/dq * dq/dr * dr/dz * dz/dp0
  // df/dq  = par[2]
  // dq/ds  = -s^-2 = -q/s
  // ds/dr  = 1
  // dr/dz  = r
  // dz/dp0 = -1 / par[1]
  // dz/dp1 = -z / par[1]

  // df/dp0 = par[2]*(-q/s)*r*(-1/par[1]) = par[2]*q*r/(s*par[1])
  // df/dp1 = par[2]*(-q/s)*r*(-z/par[1]) = par[2]*q*r*z/(s*par[1])

  if (dpar) {
    dpar[0] = par[2]*q*r/(s*par[1]); 
    dpar[1] = dpar[0]*z;
    dpar[2] = q;
    dpar[3] = 1;
  }
  
  return (f);
}

/* pars: x_o, sigma, top, bottom */
opihi_flt fsigmoidDeimos (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  double z = (x - par[0])/par[1];
  double r = exp (-z);
  double s = 1 + r;
  double q = 1 / s;
  double f = par[2]*q + par[3];

  // df/dp0 = df/dq * dq/dr * dr/dz * dz/dp0
  // df/dq  = par[2]
  // dq/ds  = -s^-2 = -q/s
  // ds/dr  = 1
  // dr/dz  = -r
  // dz/dp0 = -1 / par[1]
  // dz/dp1 = -z / par[1]

  // df/dp0 = -par[2]*(-q/s)*r*(-1/par[1]) = par[2]*q*r/(s*par[1])
  // df/dp1 = -par[2]*(-q/s)*r*(-z/par[1]) = par[2]*q*r*z/(s*par[1])

  if (dpar) {
    dpar[0] = -par[2]*q*r/(s*par[1]); 
    dpar[1] = dpar[0]*z;
    dpar[2] = q;
    dpar[3] = 1;
  }
  
  return (f);
}

# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6

// wprofile is the true poisson error
void sigmoid_fit_irls (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int NptsAll, opihi_flt *par, int Npar, SigmoidMode mode, double limit) {

  FitFunc *FUNC = (mode == MODE_LOWER) ? fsigmoidDeimos : rsigmoidDeimos;

  ALLOCATE_PTR (par_old, opihi_flt, Npar);

  // allocate these as global static in this file?
  ALLOCATE_PTR (xtemp, opihi_flt, NptsAll);
  ALLOCATE_PTR (ytemp, opihi_flt, NptsAll);
  ALLOCATE_PTR (wtemp, opihi_flt, NptsAll);
  ALLOCATE_PTR (otemp, opihi_flt, NptsAll);

  // extract the points on the right side and fit
  int Npts = 0;
  for (int i = 0; i < NptsAll; i++) {
    if (!isfinite(wprofile[i])) continue;
    if (!isfinite(fprofile[i])) continue;
    int keep = (mode == MODE_UPPER) ^ (xprofile[i] < limit);
    if (keep) {
      xtemp[Npts] = xprofile[i];
      ytemp[Npts] = fprofile[i];
      wtemp[Npts] = (wprofile[i] == 0.0) ? 1.0 : wprofile[i]; // crude
      Npts ++;
    }
  }

  // define initial weights as 1.0
  ALLOCATE_PTR (wt, opihi_flt, Npts);
  for (int i = 0; i < Npts; i++) { 
    wt[i] = 1.0 / SQ(wtemp[i]);
  }

  // initial fit with ordinary least-squares
  // otemp is the fitted value of the function at xtemp
  sigmoid_fit (xtemp, ytemp, wt, otemp, Npts, par, Npar, FUNC);

  int converged = FALSE;
  for (int iterations = 0; !converged && (iterations < MaxIterations); iterations++) {

    for (int i = 0; i < Npts; i++) {
      wt[i] = deimos_weight_cauchy ((ytemp[i] - otemp[i]) / wtemp[i]) / SQ(wtemp[i]);
    }

    sigmoid_fit (xtemp, ytemp, wt, otemp, Npts, par, Npar, FUNC);

    // save this solution
    for (int i = 0; i < Npar; i++) {
      par_old[i] = par[i];
    }

    converged = TRUE;
    for (int i = 0; i < Npts; i++) {
      if ((fabs(par[i] - par_old[i]) > FIT_TOLERANCE * fabs(par[i])) && 
	  (fabs(par[i] - par_old[i]) > FLT_TOLERANCE))
	converged = FALSE;
    }
  }

  for (int i = 0; i < NptsAll; i++) {
    oprofile[i] = FUNC (xprofile[i], par, Npar, NULL);
  }

  FREE (xtemp);
  FREE (ytemp);
  FREE (wtemp);
  FREE (otemp);

  FREE (par_old);
  FREE (wt);
}

void sigmoid_fit (opihi_flt *xprofile, opihi_flt *yprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar, FitFunc *FUNC) {

  double ochisq = mrqinit (xprofile, yprofile, wprofile, Npts, par, Npar, FUNC, VERBOSE);
  double dchisq = ochisq + 2*Npts;

  int Niter;
  for (Niter = 0; (Niter < 20) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); Niter++) {
    double chisq = mrqmin (xprofile, yprofile, wprofile, Npts, par, Npar, FUNC, VERBOSE);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (VERBOSE) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (VERBOSE) gprint (GP_ERR, "%d iterations\n", Niter); 

  if (!QUIET) gprint (GP_LOG, "sigmoid @ %f (%f) : top %f bottom %f\n", par[0], par[1], par[2], par[3]);

  for (int i = 0; i < Npts; i++) {
    oprofile[i] = FUNC (xprofile[i], par, Npar, NULL);
  }

  mrqfree (Npar);
}  

double deimos_weight_cauchy (double x) {
  double r = x / 2.385;
  return (1.0 / (1.0 + SQ(r)));
}

