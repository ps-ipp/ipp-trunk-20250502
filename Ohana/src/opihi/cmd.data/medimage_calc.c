# include "data.h"

float weight_cauchy_square_flt (float x2);
float irls_mean (float *val, float *wgt, int N, float *outvar, int *Npt);
float irls_fraction_interpolate (float *values, float fraction, int Npts);
float irls_robust_stdev (float *values, int Npts);
float irls_wtmean (float *val, float *wgt, int *idx, int Npt, float *variance);
void irls_bootstrap (int *idx, int Npts);
void irls_init (int N);
void irls_free (void);

# define IRLS_TOLERANCE 1e-4
int BOOTSTRAP = FALSE;
int BOOTSTRAP_NITER = 100;

static float *irls_valsub  = NULL;
static float *irls_wgtsub  = NULL;
static int   *irls_idx     = NULL;
static float *irls_testval = NULL;

enum {CALC_MEDIAN, CALC_MEAN, CALC_IRLS, CALC_WTMEAN, CALC_INNER_FRACTION};

int medimage_calc (int argc, char **argv) {

  int ix, iy, n, N;
  Buffer *output;
  
  BOOTSTRAP = FALSE;
  if ((N = get_argument (argc, argv, "-bootstrap"))) {
    BOOTSTRAP = TRUE;
    remove_argument (N, &argc, argv);
  }
  BOOTSTRAP_NITER = 100;
  if ((N = get_argument (argc, argv, "-bootstrap-iter"))) {
    remove_argument (N, &argc, argv);
    BOOTSTRAP_NITER = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    BOOTSTRAP = TRUE;
  }

  float minRange = NAN;
  float maxRange = NAN;

  int mode = CALC_MEDIAN;
  if ((N = get_argument (argc, argv, "-mean"))) {
    mode = CALC_MEAN;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-irls"))) {
    if (mode != CALC_MEDIAN) goto choose_one;
    mode = CALC_IRLS;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-wtmean"))) {
    if (mode != CALC_MEDIAN) goto choose_one;
    mode = CALC_WTMEAN;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-inner-fraction"))) {
    if (mode != CALC_MEDIAN) goto choose_one;
    if (argc < N + 3) goto fraction_options;
    mode = CALC_INNER_FRACTION;
    remove_argument (N, &argc, argv);
    minRange = atof(argv[N]);
    if ((minRange < 0.0) || (minRange > 1.0)) goto fraction_options;
    remove_argument (N, &argc, argv);
    maxRange = atof(argv[N]);
    if ((maxRange < 0.0) || (maxRange > 1.0)) goto fraction_options;
    if (maxRange <= minRange) goto fraction_options;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-median"))) {
    if (mode != CALC_MEDIAN) goto choose_one;
    mode = CALC_MEDIAN;
    remove_argument (N, &argc, argv);
  }

  Buffer *variance = NULL;
  if ((N = get_argument (argc, argv, "-variance"))) {
    remove_argument (N, &argc, argv);
    if ((variance = SelectBuffer (argv[N], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: medimage calc (name) (output) [-mean,-irls,-wtmean,-median] [-variance output] [-bootstrap] [-bootstrap-iter (N)]\n");
    gprint (GP_ERR, "       calculate the median image for the median image set\n");
    gprint (GP_ERR, "  This function calculates the 'average' image for the supplied set of images.\n");
    gprint (GP_ERR, "  Four options are available to calculate the average:\n");
    gprint (GP_ERR, "   -mean (straight arithmetic mean)\n");
    gprint (GP_ERR, "   -median\n");
    gprint (GP_ERR, "   -wtmean (arithmetic mean, weighted by supplied variances)\n");
    gprint (GP_ERR, "   -irls (iteratively-reweighted least-squares)\n");
    gprint (GP_ERR, "  If -variance is supplied the returned image represents the estimate of the variance on the average image\n");
    gprint (GP_ERR, "  For the four average methods above, the sqrt of the variance has the following meanings:\n");
    gprint (GP_ERR, "   -mean   : standard deviation / sqrt(Npts)\n");
    gprint (GP_ERR, "   -median : standard deviation / sqrt(Npts)\n");
    gprint (GP_ERR, "   -wtmean : formal error on the weighted mean [sum (1 / variances)]\n");
    gprint (GP_ERR, "   -irls   : formal error on the pixels used (excluding ~3 sigma outliers)\n");
    gprint (GP_ERR, "   note that the irls formal error slightly underestimates the observed standard deviation\n");
    return FALSE;
  }

  MedImageType *median = FindMedImage (argv[1]);
  if (!median) {
    gprint (GP_ERR, "median image %s not found\n", argv[1]);
    return FALSE;
  }

  if ((output = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  int Ninput = median->Ninput;
  int Nx = median->Nx;
  int Ny = median->Ny;

  ALLOCATE_PTR (val, float, Ninput);
  ALLOCATE_PTR (wgt, float, Ninput);

  if (mode == CALC_IRLS) irls_init (Ninput);

  gfits_free_matrix (&output->matrix);
  gfits_free_header (&output->header);
  if (!CreateBuffer (output, Nx, Ny, -32, 0.0, 1.0)) return FALSE;

  float *outvalue = (float *) output->matrix.buffer;
  float *varvalue = NULL;
  if (variance) {
    gfits_free_matrix (&variance->matrix);
    gfits_free_header (&variance->header);
    if (!CreateBuffer (variance, Nx, Ny, -32, 0.0, 1.0)) return FALSE;

    varvalue = (float *) variance->matrix.buffer;
  }

  // save the number of points per pixel
  Buffer *nptbuf = SelectBuffer ("irls_npt", ANYBUFFER, TRUE);
  gfits_free_matrix (&nptbuf->matrix);
  gfits_free_header (&nptbuf->header);
  if (!CreateBuffer (nptbuf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  float *NptVal = (float *) nptbuf->matrix.buffer;

  for (iy = 0; iy < Ny; iy++) {
    for (ix = 0; ix < Nx; ix++) {

      int N = 0;
      int Npix = ix + Nx*iy;
      for (n = 0; n < Ninput; n++) {
	float v = median->flx[n][Npix];
	if (!isfinite(v)) continue;
	val[N] = v;
	wgt[N] = 1.0;
	if (median->var[n]) {
	  float s = median->var[n][Npix];
	  if (!isfinite(s)) continue;
	  if (fabs(s) < 2*FLT_MIN) s = 2*FLT_MIN;
	  wgt[N] = 1.0 / s;
	}
	N++;
      }
      if (N == 0) continue;

      switch (mode) {
	case CALC_MEDIAN:
	  fsort (val, N);
	  outvalue[Npix] = val[(int)(0.5*N)];
	  if (varvalue) {
	    float sum = 0.0;
	    for (n = 0; n < N; n++) {
	      sum += SQ(val[n] - outvalue[Npix]);
	    }
	    // variance on the mean (stdev / sqrt(N))^2
	    varvalue[Npix] = sum / (N - 1) / N;
	  }
	  break;
	case CALC_INNER_FRACTION:
	  fsort (val, N);
	  int Ns = MIN(MAX(0, minRange * N),N); // e.g., 0.1 * 50 = 5
	  int Ne = MIN(MAX(0, maxRange * N),N); // e.g., 0.9 * 50 = 45 
	  int Npt = Ne - Ns;

	  float sum = 0.0;
	  for (n = Ns; n < Ne; n++) {
	    sum += val[n];
	  }
	  outvalue[Npix] = sum / (float) Npt;

	  if (varvalue) {
	    float sum = 0.0;
	    for (n = Ns; n < Ne; n++) {
	      sum += SQ(val[n] - outvalue[Npix]);
	    }
	    // variance on the mean (stdev / sqrt(N))^2
	    varvalue[Npix] = sum / (Npt - 1) / Npt;
	  }
	  break;
	case CALC_MEAN: {
	  float sum = 0.0;
	  for (n = 0; n < N; n++) {
	    sum += val[n];
	  }
	  outvalue[Npix] = sum / (float) N;

	  if (varvalue) {
	    float Sum = 0.0;
	    for (n = 0; n < N; n++) {
	      Sum += SQ(val[n] - outvalue[Npix]);
	    }
	    // variance on the mean (stdev / sqrt(N))^2
	    varvalue[Npix] = Sum / (N - 1) / N;
	  }
	  break;
	}
	case CALC_WTMEAN: {
	  float variance;
	  outvalue[Npix] = irls_wtmean (val, wgt, NULL, N, &variance);
	  if (varvalue) { varvalue[Npix] = variance; }
	  break;
	}
	case CALC_IRLS: 
	  if (varvalue) {
	    int Npts = 0;
	    outvalue[Npix] = irls_mean (val, wgt, N, &varvalue[Npix], &Npts);
	    NptVal[Npix] = Npts;
	  } else {
	    int Npts = 0;
	    outvalue[Npix] = irls_mean (val, wgt, N, NULL, &Npts);
	    NptVal[Npix] = Npts;
	  }
      }
    }
  }
  irls_free();
  return TRUE;

 choose_one:
  gprint (GP_ERR, "supply only one of -mean, -median, -irls, -wtmean, -inner-fraction\n");
  return FALSE; 

 fraction_options:
  gprint (GP_ERR, "USE: -inner-fraction (min) (max) : min & max in range 0.0 to 1.0\n");
  return FALSE; 
}

// allocate temporary vectors
void irls_init (int N) {

  ALLOCATE (irls_valsub, float, N);
  ALLOCATE (irls_wgtsub, float, N);
  ALLOCATE (irls_idx, int, N);
  ALLOCATE (irls_testval, float, BOOTSTRAP_NITER);
}

// free temporary vectors
void irls_free (void) {
  FREE (irls_valsub); 
  FREE (irls_wgtsub);
  FREE (irls_idx);
  FREE (irls_testval);
  irls_valsub = NULL;
  irls_wgtsub = NULL;
  irls_idx = NULL;
  irls_testval = NULL;
}

float irls_mean (float *val, float *wgt, int N, float *outvar, int *Npt) {

  // calculate weighted mean
  float Value = irls_wtmean (val, wgt, NULL, N, NULL);

  int converged = FALSE;
  for (int i = 0; (i < 10) && !converged; i++) {
    float ValueLast = Value;
    float S1 = 0.0, S2 = 0.0;
    
    // calculate weight modification based on distances (squared).
    // use modifier to calculate new weighted mean
    for (int n = 0; n < N; n++) {
      float dV = (val[n] - Value);
      float d2 = SQ(dV) * wgt[n];
      
      float Mod = weight_cauchy_square_flt (d2);
      S1 += Mod * wgt[n] * val[n];
      S2 += Mod * wgt[n];
    }
    Value = S1 / S2;

    float delta = fabs(Value - ValueLast);
    if (delta < Value * IRLS_TOLERANCE) converged = TRUE;
  }

  if (outvar) {
    if (BOOTSTRAP) {
      // generate a subset vector of just the accepted points
    
      // calculate stdev of high-weight points
      int npt = 0;
      for (int n = 0; n < N; n++) {

	float dV = (val[n] - Value);
	float d2 = SQ(dV) * wgt[n];
      
	float Mod = weight_cauchy_square_flt (d2);
	if (Mod < 0.1) continue; // totally ad-hoc number

	irls_valsub[npt] = val[n];
	irls_wgtsub[npt] = wgt[n];
	npt ++;
      }

      for (int iter = 0; iter < BOOTSTRAP_NITER; iter++) {
	irls_bootstrap (irls_idx, npt); // fill idx with the index of the resampled points
	irls_testval[iter] = irls_wtmean (irls_valsub, irls_wgtsub, irls_idx, npt, NULL);
      }

      float sigma = irls_robust_stdev (irls_testval, BOOTSTRAP_NITER);

      *outvar = SQ(sigma);
      *Npt = npt;
    } else {
      // calculate stdev of high-weight points
      float S1 = 0.0, S2 = 0.0;
      int npt = 0;
      for (int n = 0; n < N; n++) {

	float dV = (val[n] - Value);
	float d2 = SQ(dV) * wgt[n];
      
	float Mod = weight_cauchy_square_flt (d2);
	if (Mod < 0.1) continue; // totally ad-hoc number

	S1 += SQ(dV);
	S2 += wgt[n];
	npt ++;
      }
      *Npt = npt;
      *outvar = 1 / S2;
    }
  }

  return Value;
}

void irls_bootstrap (int *idx, int Npts) {
  // generate an index (idx) containing the index values
  // for the Npts randomly resampled points
  for (int i = 0; i < Npts; i++) {
    idx[i] = Npts * drand48();
  }
}

float irls_wtmean (float *val, float *wgt, int *idx, int Npt, float *variance) {
  float S1 = 0.0, S2 = 0.0;
  for (int n = 0; n < Npt; n++) {
    int N = idx ? idx[n] : n;
    S1 += wgt[N] * val[N];
    S2 += wgt[N];
  }
  float Value = S1 / S2;
  if (variance) *variance = 1.0 / S2;
  return Value;
}

float irls_robust_stdev (float *values, int Npts) {

  fsort (values, Npts);

  float Slo = irls_fraction_interpolate (values, 0.158655, Npts);
  float Shi = irls_fraction_interpolate (values, 0.841345, Npts);
  float sigma = (Shi - Slo) / 2.0;
  
  return sigma;
}

float irls_fraction_interpolate (float *values, float fraction, int Npts) {

  float F = fraction * Npts;
  int   N = fraction * Npts;

  if (N < 0        ) return NAN;
  if (N >= Npts - 2) return NAN;

  // interpolate between N,N+1
    
  float S = (F - N) * (values[N+1] - values[N]) + values[N];
  return S;
}


// exp(-(x^2/s^2)/2) = (1/2)
//     -(x^2/s^2)/2  = ln(1/2)
//      (x^2/s^2)/2  = ln(2)
//      (x^2/s^2)    = 2ln(2)
//      (x  /s)      = sqrt(2ln(2)) : half-width at half-max
//       FWHM        = 2sqrt(2ln(2))

// R2 = (X / 2.385)^2 = (X^2 / 2.385^2)

# define CAUCY_FACTOR 2.0

/*
float weight_cauchy_square_flt (float x2) {
  return (1.0);
}
*/

float weight_cauchy_square_flt (float x2) {
  float r2 = x2 / CAUCY_FACTOR;
  return (1.0 / (1.0 + r2));
}

