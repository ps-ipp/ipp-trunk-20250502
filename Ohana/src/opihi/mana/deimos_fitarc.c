# include "data.h"
# define NPAR 4

typedef opihi_flt FitFunc (opihi_flt, opihi_flt *, int, opihi_flt *);

opihi_flt fgaussOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar);

void gaussian_fit_irls (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar);
void gaussian_fit (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar, FitFunc *FUNC);

double deimos_weight_cauchy (double x);

// use these globals to carry into sigmoid_fit
static int VERBOSE = FALSE;
static int QUIET   = FALSE;
static int MaxIterations = 10;

/* fitarc buffer Xo Yo Nx Ny 

 * for each col, fit the y-dir profile with a gauss
 * fit the C0 values to a line to get the angle
 */

int deimos_fitarc (int argc, char **argv) {

  // input parameters:
  // * xprofile    : extracted slit window coordinates (vector)
  // * xprofile    : extracted slit window profile (vector)
  // * Xs, Xe      : scale coordinates of window boundaries

  // output parameters:
  // * sigmoid_left_center, etc : coordinates of window boundaries

  int N;
  Buffer *buffer  = NULL;

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

  Buffer *residbuf = NULL;
  if ((N = get_argument (argc, argv, "-resid"))) {
    remove_argument (N, &argc, argv);
    residbuf = SelectBuffer (argv[N], ANYBUFFER, TRUE);
    if (!residbuf) goto usage;
    remove_argument (N, &argc, argv);
  }
  
  Buffer *varbuf = NULL;
  if ((N = get_argument (argc, argv, "-varbuf"))) {
    remove_argument (N, &argc, argv);
    varbuf = SelectBuffer (argv[N], OLDBUFFER, TRUE);
    if (!varbuf) goto usage;
    remove_argument (N, &argc, argv);
  }
  
  if (argc != 6) goto usage;
  if ((buffer = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  int nx = buffer[0].matrix.Naxis[0];
  int ny = buffer[0].matrix.Naxis[1];

  // supplied guess for boundaries
  int Xs = atof(argv[2]);
  int Ys = atof(argv[3]);
  int Nx = atof(argv[4]);
  int Ny = atof(argv[5]);

  if (residbuf) {
    ResetBuffer (residbuf, nx, ny, -32, 0.0, 1.0);
  }

  // validate Xs, Ys, Nx, Ny
  // Xs + Nx <= nx

  // loop over the cols

  ALLOCATE_PTR (xcol, double, Ny);
  ALLOCATE_PTR (ycol, double, Ny);
  ALLOCATE_PTR (ocol, double, Ny); // fitted value

  float *bufVal = (float *) buffer->matrix.buffer;

  for (int ix = Xs; ix < Xs + Nx; ix++) {

    // loop over the pixels in this column to get the x and y vectors
    // find the peak at the same time

    double xmax = -HUGE_VAL;
    double ymax = -HUGE_VAL;
    int Nc = 0;

    for (int iy = 0; iy < Ny; iy++) {

      float yval = bufVal[ix + (iy + Ys)*Nx];
      if (!isfinite(yval)) continue;

      xcol[Nc] = iy + Ys;
      ycol[Nc] = yval;

      if (yval > ymax) {
	ymax = yval;
	xmax = iy + Ys;
      }
      Nc ++;
    }

    // now do a Gauss fit given C0 = xmax, C1 = 2, C2 = ymax, C3 = 0
    
    // temporary storage vectors 
    opihi_flt par[NPAR];
    par[0] = xmax;
    par[1] = 2;
    par[2] = ymax;
    par[3] = 0;
    gaussian_fit_irls (xcol, ycol, NULL, ocol, Nc, par, NPAR);

    fprintf (stderr, "%d : %f %f : %f %f\n", ix, par[0], par[2], par[1], par[3]);
    set_variable ("C0", par[0]);
    set_variable ("C1", par[1]);
    set_variable ("C2", par[2]);
    set_variable ("C3", par[3]);

    if (residbuf) {
      float *residVal = (float *) residbuf->matrix.buffer;
      for (int iy = 0; iy < Ny; iy++) {
	float yval = fgaussOD (iy + Ys, par, NPAR, NULL);
	// residVal[ix + (iy + Ys)*Nx] = bufVal[ix + (iy + Ys)*Nx] - yval;
	residVal[ix + (iy + Ys)*Nx] = yval;
      }
    }
  } 

  return TRUE;
  
 usage:
  gprint (GP_ERR, "USAGE: deimos fitarc buff Xo Yo Nx Ny\n");
  gprint (GP_ERR, " [-resid buffer] : output residual image\n");
  gprint (GP_ERR, " [-varbuf buffer] : input variance image\n");
  return FALSE;
}

# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6

// xprofile : x-coordinate
// fprofile : flux (counts)
// wprofile : weight (true poisson error)
// oprofile : output profile (not used?)
void gaussian_fit_irls (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar) {
  
  // buffer to store result from each iteration
  ALLOCATE_PTR (par_old, opihi_flt, Npar);

  // define initial weights as 1.0
  ALLOCATE_PTR (wt, opihi_flt, Npts);
  ALLOCATE_PTR (wtemp, opihi_flt, Npts);
  for (int i = 0; i < Npts; i++) { 
    wtemp[i] = wprofile ? (fabs(wprofile[i]) > 1e-5 ? wprofile[i] : 1e-5) : 1.0;
    wt[i] = 1.0 / SQ(wtemp[i]);
  }

  // initial fit with ordinary least-squares
  // otemp is the fitted value of the function at xtemp
  gaussian_fit (xprofile, fprofile, wt, NULL, Npts, par, Npar, fgaussOD);

  int converged = FALSE;
  for (int iterations = 0; !converged && (iterations < MaxIterations); iterations++) {

    for (int i = 0; i < Npts; i++) {
      wt[i] = deimos_weight_cauchy ((fprofile[i] - oprofile[i]) / wtemp[i]) / SQ(wtemp[i]);
    }

    gaussian_fit (xprofile, fprofile, wt, oprofile, Npts, par, Npar, fgaussOD);

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

  for (int i = 0; i < Npts; i++) {
    oprofile[i] = fgaussOD (xprofile[i], par, Npar, NULL);
  }

  FREE (par_old);
  FREE (wt);
}

void gaussian_fit (opihi_flt *xprofile, opihi_flt *fprofile, opihi_flt *wprofile, opihi_flt *oprofile, int Npts, opihi_flt *par, int Npar, FitFunc *FUNC) {

  double ochisq = mrqinit (xprofile, fprofile, wprofile, Npts, par, Npar, FUNC, VERBOSE);
  double dchisq = ochisq + 2*Npts;

  int Niter;
  for (Niter = 0; (Niter < 20) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); Niter++) {
    double chisq = mrqmin (xprofile, fprofile, wprofile, Npts, par, Npar, FUNC, VERBOSE);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (VERBOSE) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (VERBOSE) gprint (GP_ERR, "%d iterations\n", Niter); 

  if (!QUIET) gprint (GP_LOG, "gauss @ %f (%f) : top %f bottom %f\n", par[0], par[1], par[2], par[3]);

  if (oprofile) {
    for (int i = 0; i < Npts; i++) {
      oprofile[i] = FUNC (xprofile[i], par, Npar, NULL);
    }
  }

  mrqfree (Npar);
}  
