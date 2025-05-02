# include "data.h"
# define IRLS_TOLERANCE 1e-4

float weight_cauchy_square_flt (float x2);
static void fitflux (float *Fwind, float *Fprof, float *weight, int Nx, double *flux, double *sky);

int deimos_fitslit (int argc, char **argv) {

  // fitslit (window) (vari) (slit) (flux) (sky)

  Vector *flux    = NULL;
  Vector *sky     = NULL;

  Buffer *wind    = NULL;
  Buffer *vari    = NULL;
  Buffer *slit    = NULL;

  int N;
  int Niter = 10;
  if ((N = get_argument (argc, argv, "-irls-iter"))) {
    remove_argument (N, &argc, argv);
    Niter = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Buffer *model = NULL;
  if ((N = get_argument (argc, argv, "-model"))) {
    remove_argument (N, &argc, argv);
    if ((model = SelectBuffer (argv[N], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: deimos fitslit (window) (variance) (slit) (flux) (sky)\n");
    gprint (GP_ERR, "  inputs:  window (observed 2D flux), variance (on 2D flux), slit (model 2D flux)\n");
    gprint (GP_ERR, "  outputs: flux (best-fit 1D flux), sky (best-fit 1D background)\n");
    return FALSE;
  }

  // XXX I probably should rename FindSpline as SelectSpline and give it the same behavior
  if ((wind = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vari = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((slit = SelectBuffer (argv[3], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((flux = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sky  = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // define the output window
  int Nx = wind[0].matrix.Naxis[0];
  int Ny = wind[0].matrix.Naxis[1];
  
  // confirm wind, vari, slit have same dimensions
  if (Nx != slit[0].matrix.Naxis[0]) { gprint (GP_ERR, "size mismatch in fitslit for window and slit\n"); return FALSE; }
  if (Ny != slit[0].matrix.Naxis[1]) { gprint (GP_ERR, "size mismatch in fitslit for window and slit\n"); return FALSE; }
  if (Nx != vari[0].matrix.Naxis[0]) { gprint (GP_ERR, "size mismatch in fitslit for window and variance\n"); return FALSE; }
  if (Ny != vari[0].matrix.Naxis[1]) { gprint (GP_ERR, "size mismatch in fitslit for window and variance\n"); return FALSE; }

  ResetVector (flux, OPIHI_FLT, Ny);
  ResetVector (sky,  OPIHI_FLT, Ny);

  if (model) {
    if (!CreateBuffer (model, Nx, Ny, -32, 1.0, 0.0)) { gprint (GP_ERR, "error generating output model buffer\n"); return FALSE; }
  }

  float *Fwind = (float *) wind[0].matrix.buffer;
  float *Fvari = (float *) vari[0].matrix.buffer;
  float *Fprof = (float *) slit[0].matrix.buffer;
  float *FmodOut = model ? (float *) model[0].matrix.buffer : NULL;

  ALLOCATE_PTR (weight, float, Nx);
  ALLOCATE_PTR (rawwgt, float, Nx);

  // loop over the rows
  for (int iy = 0; iy < Ny; iy++) {

    double F = 0.0, S = 0.0;

    // set weight based on the variance
    for (int ix = 0; ix < Nx; ix++) {
      float vari = Fvari[ix + iy*Nx];
      weight[ix] = !isfinite(vari) || (fabs(vari) < 1e-6) ? 1.0 : 1.0 / vari;
      rawwgt[ix] = weight[ix];
    }

    // calculate elements of the chi-square for this row:
    fitflux (&Fwind[iy*Nx], &Fprof[iy*Nx], weight, Nx, &F, &S);

    // do an IRLS loop here to reject outliers
    int converged = FALSE;
    for (int iter = 0; (iter < Niter) && !converged; iter++) {

      double Flast = F;
      double Slast = S;

      // calculate weight modification based on distances (squared).
      // use modifier to calculate new weighted mean
      for (int ix = 0; ix < Nx; ix++) {

	// F & S are model parameters; compare the model and observed values for each pixel
	float Fmodel = S + F*Fprof[ix + iy*Nx];
	float dV = (Fwind[ix + iy*Nx] - Fmodel);
	float d2 = SQ(dV) * rawwgt[ix];
	
	float Mod = weight_cauchy_square_flt (d2);
	weight[ix] = Mod * rawwgt[ix];
      }
      fitflux (&Fwind[iy*Nx], &Fprof[iy*Nx], weight, Nx, &F, &S);

      float dF = fabs(F - Flast);
      float dS = fabs(S - Slast);

      if ((dF < F * IRLS_TOLERANCE) && (dS < S * IRLS_TOLERANCE)) converged = TRUE;
    }

    if (FmodOut) {
      for (int ix = 0; ix < Nx; ix++) {
	// F & S are model parameters; compare the model and observed values for each pixel
	float Fmodel = S + F*Fprof[ix + iy*Nx];
	float dV = (Fwind[ix + iy*Nx] - Fmodel);
	float d2 = SQ(dV) * rawwgt[ix];
	float Mod = weight_cauchy_square_flt (d2);
	if (Mod < 0.1) {
	  FmodOut[ix + iy*Nx] = NAN;
	} else {
	  FmodOut[ix + iy*Nx] = Fmodel;
	}
      }
    }

    flux->elements.Flt[iy] = F;
    sky->elements.Flt[iy] = S;
  }
  return TRUE;
}

// weight is 1 / variance
// Fwind, Fprof are pointers to the start of a single row 
// Fwind is the observed flux in the window
// Fprof is the slit profile
void fitflux (float *Fwind, float *Fprof, float *weight, int Nx, double *flux, double *sky) {

  // calculate elements of the chi-square fit for this row:

  double R = 0.0, P = 0.0, P2 = 0.0, f = 0.0, fp = 0.0;

  for (int ix = 0; ix < Nx; ix++) {
      
    double fxy = Fwind[ix];
    double pxy = Fprof[ix];

    // skip NAN / Inf pixels in either window or slit
    if (!isfinite(fxy)) continue;
    if (!isfinite(pxy)) continue;

    double wt = (weight == NULL) ? 1.0 : weight[ix];
    R  += wt;
    P  += pxy*wt;
    P2 += pxy*pxy*wt;
    f  += fxy*wt;
    fp += fxy*pxy*wt;
  }

  double det = R*P2 - P*P;
  *sky = (P2*f - P*fp) / det;
  *flux = (R*fp - P*f) / det;
}
