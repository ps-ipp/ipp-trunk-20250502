# include "data.h"
# include "deimos.h"

/*
  this is starting to work OK.  some improvements to make
  * use Gaussdev to sample
  * do not scale down range (or user-set scale-down)
 */

// internal functions to fitobj
static float      deimos_get_chisq (float *buffer, float *vardata, float *model, int Nx, int Ny, int row, int *Npts);
double            deimos_LMM_update (float *data, float *vardata, float *model_ref, float *model_obj, float *model_sky, float *model_bck, float *dObj, float *dSky, float *dBck, int Nx, int Ny, int row, float lambda);
static opihi_flt *deimos_make_test (opihi_flt *guess, float sigma, int row, int Nrow, float *dValue);
opihi_flt        *deimos_subvector (opihi_flt *fullVec, int row, int Nrow);

int deimos_fitobj (int argc, char **argv) {

  // input parameters:
  // * buffer         : 2D image of slit (full frame or cutout?)
  // * slit_trace_red : spline fit of slit central x pos vs y-coord
  // * slit_trace_blu : spline fit of slit central x pos vs y-coord
  // * psf_trace      : spline fit of slit central x pos vs y-coord
  // * profile        : slit window profile (vector)
  // * PSF            : point-spread function vector (flux normalized, x-dir)
  // * stilt          : slit tilt response : 2D kernel? 

  // in-out parameters:
  // * obj  : vector of object flux vs y-coord (starting guess and result)
  // * sky  : vector of local sky signal vs y-coord 
  // * bck  : vector of extra-slit background flux vs y-coord

  int N;

  Spline *slit_trace_red = NULL;
  Spline *slit_trace_blu = NULL;
  Spline *psf_trace  = NULL;

  Vector *profile    = NULL;
  Vector *psf        = NULL;

  Vector *obj        = NULL;
  Vector *sky        = NULL;
  Vector *bck        = NULL;

  Buffer *buffer     = NULL;

  float stilt        = 0.0; // angle of the slit

  ohana_gaussdev_init ();

  int DO_PLUS = FALSE;
  if ((N = get_argument (argc, argv, "-plus"))) {
    remove_argument (N, &argc, argv);
    DO_PLUS = TRUE;
  }

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  int SKIPFIT = FALSE;
  if ((N = get_argument (argc, argv, "-skipfit"))) {
    remove_argument (N, &argc, argv);
    SKIPFIT = TRUE;
  }

  // for a red vs blu spline, we need to specify the split point
  // XXX this is REALLY ad-hoc for Deimos.  not sure how to make
  // this more generic (need to define the ranges somewhere)
  int redlimit = 4096;
  if ((N = get_argument (argc, argv, "-redlimit"))) {
    remove_argument (N, &argc, argv);
    redlimit = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // a crude noise model valid for both the guess vectors and the
  // data buffer:
  float gain = 1.0;
  if ((N = get_argument (argc, argv, "-gain"))) {
    remove_argument (N, &argc, argv);
    gain = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  float noise = 0.0;
  if ((N = get_argument (argc, argv, "-noise"))) {
    remove_argument (N, &argc, argv);
    noise = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  float nsigma = 2.0;
  if ((N = get_argument (argc, argv, "-nsigma"))) {
    remove_argument (N, &argc, argv);
    nsigma = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  float lambda = 1.0;
  if ((N = get_argument (argc, argv, "-lambda"))) {
    remove_argument (N, &argc, argv);
    lambda = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int Niter = 25;
  if ((N = get_argument (argc, argv, "-iter"))) {
    remove_argument (N, &argc, argv);
    Niter = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  int Nrow = 5;
  if ((N = get_argument (argc, argv, "-row"))) {
    remove_argument (N, &argc, argv);
    Nrow = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // Input parameters:
  if ((N = get_argument (argc, argv, "-slit-trace-red"))) {
    remove_argument (N, &argc, argv);
    if ((slit_trace_red = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-slit-trace-blu"))) {
    remove_argument (N, &argc, argv);
    if ((slit_trace_blu = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }

  if ((N = get_argument (argc, argv, "-psf-trace"))) {
    remove_argument (N, &argc, argv);
    if ((psf_trace = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-profile"))) {
    remove_argument (N, &argc, argv);
    if ((profile = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-psf"))) {
    remove_argument (N, &argc, argv);
    if ((psf = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-stilt"))) {
    remove_argument (N, &argc, argv);
    stilt = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } else { goto usage; }

  // In-Out parameters:
  if ((N = get_argument (argc, argv, "-object"))) {
    remove_argument (N, &argc, argv);
    if ((obj = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    if ((sky = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-backgnd"))) {
    remove_argument (N, &argc, argv);
    if ((bck = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }

  // save the model
  Buffer *model = NULL;
  if ((N = get_argument (argc, argv, "-model"))) {
    remove_argument (N, &argc, argv);
    if ((model = SelectBuffer (argv[N], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  // save the model
  Buffer *varim = NULL;
  if ((N = get_argument (argc, argv, "-variance"))) {
    remove_argument (N, &argc, argv);
    if ((varim = SelectBuffer (argv[N], OLDBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) goto usage;

  // buffer is the full image, Xref is reference coordinate of the profile in the buffer
  if ((buffer = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  
  // profile and PSF are defined with central reference pixel mapped to Xref (+ trace)
  if (profile->Nelements % 2 == 0) {
    gprint (GP_ERR, "slit profile vector must have an odd number of pixels\n");
    return FALSE;
  }
  if (psf->Nelements % 2 == 0) {
    gprint (GP_ERR, "PSF vector must have an odd number of pixels\n");
    return FALSE;
  }

  // observed buffer size
  int Nx = buffer[0].matrix.Naxis[0];
  int Ny = buffer[0].matrix.Naxis[1];
  float *bufVal = (float *) buffer->matrix.buffer;

  if (varim) {
    if (Nx != varim[0].matrix.Naxis[0]) { gprint (GP_ERR, "inconsistent sizes for image and varim\n"); return FALSE; }
    if (Ny != varim[0].matrix.Naxis[1]) { gprint (GP_ERR, "inconsistent sizes for image and varim\n"); return FALSE; }
  }
  float *varVal = varim ? (float *) varim->matrix.buffer : NULL;

  // obj, sky, bck must be consistent with data
  if (Ny != obj->Nelements) {
    gprint (GP_ERR, "inconsistent wavelength scales (object)\n");
    return FALSE;
  }
  if (Ny != sky->Nelements) {
    gprint (GP_ERR, "inconsistent wavelength scales (sky)\n");
    return FALSE;
  }
  if (Ny != bck->Nelements) {
    gprint (GP_ERR, "inconsistent wavelength scales (backgnd)\n");
    return FALSE;
  }

  if (model) {
    if (!CreateBuffer (model, Nx, Ny, -32, 1.0, 0.0)) { gprint (GP_ERR, "error generating output model buffer\n"); return FALSE; }
  }

  // for the cross-dispersion reference pixel, use the user value or set to Nx/2 if < 0
  deimos_set_cross_ref (atoi(argv[2]), Nx); 
  deimos_make_kernel (stilt, Nx);

  Vector  *objMin = NULL;
  Vector  *skyMin = NULL;
  Vector  *bckMin = NULL;

  if ((objMin = SelectVector ("objMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (objMin, OPIHI_FLT, Ny);
  if ((skyMin = SelectVector ("skyMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (skyMin, OPIHI_FLT, Ny);
  if ((bckMin = SelectVector ("bckMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (bckMin, OPIHI_FLT, Ny);

  // the functions below accept an opihi_flt array for object, sky, background
  opihi_flt *objVal = objMin->elements.Flt;
  opihi_flt *skyVal = skyMin->elements.Flt;
  opihi_flt *bckVal = bckMin->elements.Flt;

  // we make a copy of the input guess vectors and minimize them
  for (int iy = 0; iy < Ny; iy++) {
    objVal[iy] = obj->elements.Flt[iy];
    skyVal[iy] = sky->elements.Flt[iy];
    bckVal[iy] = bck->elements.Flt[iy];
  }

  float noiseVar = SQ(noise);
  ALLOCATE_PTR (objNoise, float, Ny);
  ALLOCATE_PTR (skyNoise, float, Ny);
  ALLOCATE_PTR (bckNoise, float, Ny);

  // generate noise vectors
  for (int iy = 0; iy < Ny; iy++) {
    objNoise[iy] = nsigma*sqrt(hypot(fabs(objVal[iy]) / gain, noiseVar));
    skyNoise[iy] = nsigma*sqrt(hypot(fabs(skyVal[iy]) / gain, noiseVar));
    bckNoise[iy] = nsigma*sqrt(hypot(fabs(bckVal[iy]) / gain, noiseVar));
  }

  // obj, sky, bck are initial guesses.  Also use these values to define a range for the initial guesses.

  // XXX worry about last segment (< Nrow rows)
  for (int iter = 0; iter < Niter; iter++) {

    int dRow = Nrow/2;

    // use scaled-noise to define test ranges
    // we are going to try each row using a window Nrow wide to measure chisq:
    for (int row = dRow; !SKIPFIT && (row < Ny - dRow - 1); row++) {
      
      // I am using a numerical equivalent of Levenberg-Marquardt, but only for a single
      // p_m[i] = (obj,sky,bck) triplet [m is the index of obj,sky,bck; i is the row
      // index] at a time.  I start with a current best guess set of (obj,sky,bck).  I
      // need to calculate the model at this location, and then at offset locations p_m +
      // dp_m.  Note that I do not need to calculate chisq for each of these to generate
      // the elements of the LMM equations:

      int row_0 = row - dRow; // start row of the subset test range

      // Nrow = 5, dRow = 2, Ny = 100, last start is Ny - 2 - 1 = 97
      // last row_0 is 95

      // generate subset vectors for the range **centered** on row
      opihi_flt *objCurr = deimos_subvector (objVal, row_0, Nrow);
      opihi_flt *skyCurr = deimos_subvector (skyVal, row_0, Nrow);
      opihi_flt *bckCurr = deimos_subvector (bckVal, row_0, Nrow);

      // current vector value:
      // for deimos_make_model, row is the starting point of the subimage
      float *model_ref = deimos_make_model (objCurr, skyCurr, bckCurr, psf, profile, slit_trace_red, slit_trace_blu, redlimit, psf_trace, Nx, Nrow, row_0);

      // in the test functions below, dRow is the central pixel of the subset, e.g., obj[row] => objCurr[dRow]

      float dObj = 0.0, dSky = 0.0, dBck = 0.0;

      // delta obj (save dObj)
      opihi_flt *objtest = deimos_make_test (objCurr, objNoise[row], dRow, Nrow, &dObj);
      float *model_obj = deimos_make_model (objtest, skyCurr, bckCurr, psf, profile, slit_trace_red, slit_trace_blu, redlimit, psf_trace, Nx, Nrow, row_0);

      // delta sky (save dSky)
      opihi_flt *skytest = deimos_make_test (skyCurr, skyNoise[row], dRow, Nrow, &dSky);
      float *model_sky = deimos_make_model (objCurr, skytest, bckCurr, psf, profile, slit_trace_red, slit_trace_blu, redlimit, psf_trace, Nx, Nrow, row_0);

      // delta bck (save dBck)
      opihi_flt *bcktest = deimos_make_test (bckCurr, bckNoise[row], dRow, Nrow, &dBck);
      float *model_bck = deimos_make_model (objCurr, skyCurr, bcktest, psf, profile, slit_trace_red, slit_trace_blu, redlimit, psf_trace, Nx, Nrow, row_0);

      // dObj, dSky, dBck carry the delta used derivatives in and the delta applied to the values out
      float chisq = deimos_LMM_update (bufVal, varVal, model_ref, model_obj, model_sky, model_bck, &dObj, &dSky, &dBck, Nx, Nrow, row_0, lambda);

      if (FALSE && (row > 10) && (row < 20)) {
	fprintf (stderr, "%d %d : %f %f %f\n", iter, row, dObj, dSky, dBck);
      }

      if (1) {
	if (DO_PLUS) {
	  objVal[row] += dObj;
	  skyVal[row] += dSky;
	  bckVal[row] += dBck;
	} else {
	  objVal[row] -= dObj;
	  skyVal[row] -= dSky;
	  bckVal[row] -= dBck;
	}
      }

      free (objCurr);
      free (skyCurr);
      free (bckCurr);

      free (model_ref);

      free (model_obj);
      free (model_sky);
      free (model_bck);

      free (objtest);
      free (skytest);
      free (bcktest);

      if (VERBOSE) fprintf (stderr, "chisq: %f (%f, %f, %f)\n", chisq, dObj, dSky, dBck);
    }

    float *model_full = deimos_make_model (objVal, skyVal, bckVal, psf, profile, slit_trace_red, slit_trace_blu, redlimit, psf_trace, Nx, Ny, 0);

    int Npts = 0;
    float chisq_full  = deimos_get_chisq (bufVal, varVal, model_full, Nx, Ny, 0, &Npts);

    if (model) {
      free (model[0].matrix.buffer);
      model[0].matrix.buffer = (char *) model_full;
    } else {
      free (model_full);
    }

    fprintf (stderr, "** Full chisq: %f (%d) : %f)\n", chisq_full, Npts, chisq_full / (1.0 * Npts));
  }

  return TRUE;

 usage:
  gprint (GP_ERR, "USAGE: deimos fitobj (buffer) (Xref) -object vector -sky vector -backgnd vector -slit-trace-red (spline) -slit-trace-blu (spline) -psf-trace (spline) -profile vector -psf vector -stilt (angle)\n");
  gprint (GP_ERR, "OPTIONS: [-model image] [-variance image] [-gain value] [-noise value] [-nsigma value] [-lambda value] [-iter N] [-row N] [-redlimit Npixel] [-skipfit] [-v] [-plus]\n");
  return FALSE;
}

/****************** fitobj Support Functions *******************/

// we generate a new vector with a single element (row) modified based on sigma
opihi_flt *deimos_subvector (opihi_flt *fullVec, int row, int Nrow) {

  ALLOCATE_PTR (value, opihi_flt, Nrow);

  for (int i = 0; i < Nrow; i++) {
    value[i] = fullVec[i + row];
  }

  return value;
}


// we generate a new vector with a single element (row) modified based on sigma
static opihi_flt *deimos_make_test (opihi_flt *guess, float sigma, int row, int Nrow, float *dValue) {

  // if fullInput is TRUE,  the guess vector runs from 0 to Ny (full wavelength range)
  // if fullInput is FALSE, the guess vector runs from row to row + Nrow (subset range)
  // sigma is valid for the element at row

  ALLOCATE_PTR (value, opihi_flt, Nrow);

  for (int i = 0; i < Nrow; i++) {
    value[i] = guess[i];
  }

  value[row] = guess[row] + 0.1*sigma;

  *dValue = 0.1*sigma;
  return value;
}


double deimos_LMM_update (float *data, float *vardata, float *model_ref,
			  float *model_obj, float *model_sky, float *model_bck,
			  float *dObj, float *dSky, float *dBck, int Nx, int Ny, int row, float lambda) {
  
  // we are going to loop over the images calculating the following value:

  // W = 1 / sigma^2, leave as 1.0 for now
  // chisq = sum W * (data - model_ref)^2 
  // dF_X = sum W * (data - model_ref) * (model_ref - model_X) / dX
  // d2F_XY = sum W * (model_ref - model_X) * (model_ref - model_Y) / (dX * dY)

  double chisq = 0.0;
  double dF_obj = 0.0, dF_sky = 0.0, dF_bck = 0.0;
  double d2F_obj_obj = 0.0, d2F_obj_sky = 0.0, d2F_obj_bck = 0.0, d2F_sky_sky = 0.0, d2F_sky_bck = 0.0, d2F_bck_bck = 0.0;

  for (int iy = 0; iy < Ny; iy++) {
    for (int ix = 0; ix < Nx; ix++) {
      int pix = ix + iy*Nx;
      int pix_data  = ix + (iy + row)*Nx;

      // if any of the pixel values are NAN, skip the point:
      if (!isfinite(data[pix_data])) continue;
      if (!isfinite(model_ref[pix])) continue;
      if (!isfinite(model_obj[pix])) continue;
      if (!isfinite(model_sky[pix])) continue;
      if (!isfinite(model_bck[pix])) continue;

      if (vardata && !isfinite(vardata[pix_data])) continue;
      float Var = vardata ? vardata[pix_data] : 1.0;
      float W = (fabs(Var) < 0.01) ? 100.0 : 1.0 / Var;

      float df = W * (data[pix_data] - model_ref[pix]);

      chisq += SQ(df);

      float df_obj = (model_ref[pix] - model_obj[pix]) / *dObj;
      float df_sky = (model_ref[pix] - model_sky[pix]) / *dSky;
      float df_bck = (model_ref[pix] - model_bck[pix]) / *dBck;

      dF_obj += df*df_obj;
      dF_sky += df*df_sky;
      dF_bck += df*df_bck;

      d2F_obj_obj += W * SQ(df_obj);
      d2F_sky_sky += W * SQ(df_sky);
      d2F_bck_bck += W * SQ(df_bck);

      d2F_obj_sky += W * df_obj * df_sky;
      d2F_obj_bck += W * df_obj * df_bck;
      d2F_sky_bck += W * df_sky * df_bck;
    }
  }

  // these are the elements of Ax = B
  ALLOCATE_PTR (A, double *, 3);
  ALLOCATE_PTR (B, double *, 3);
  for (int i = 0; i < 3; i++) {
    ALLOCATE (A[i], double, 3);
    ALLOCATE (B[i], double, 1);
  }

  A[0][0] = d2F_obj_obj * (1.0 + lambda);
  A[1][1] = d2F_sky_sky * (1.0 + lambda);
  A[2][2] = d2F_bck_bck * (1.0 + lambda);

  A[0][1] = A[1][0] = d2F_obj_sky;
  A[1][2] = A[2][1] = d2F_sky_bck;
  A[0][2] = A[2][0] = d2F_obj_bck;

  B[0][0] = dF_obj;
  B[1][0] = dF_sky;
  B[2][0] = dF_bck;

  dgaussjordan (A, B, 3, 1);

  *dObj = B[0][0];
  *dSky = B[1][0];
  *dBck = B[2][0];

  return chisq;
}

static float deimos_get_chisq (float *buffer, float *vardata, float *model, int Nx, int Ny, int row, int *Npts) {

  int npts = 0;
  float chisq = 0;

  for (int iy = 0; iy < Ny; iy++) {

    for (int ix = 0; ix < Nx; ix++) {

      int pix_buffer = ix + (iy + row)*Nx;
      int pix_model  = ix +  iy*Nx;

      if (!isfinite(buffer[pix_buffer])) continue;
      if (!isfinite(model[pix_model])) continue;

      if (vardata && !isfinite(vardata[pix_buffer])) continue;
      float Var = vardata ? vardata[pix_buffer] : 1.0;
      float W = (fabs(Var) < 0.01) ? 100.0 : 1.0 / Var;

      float dchisq = SQ(buffer[pix_buffer] - model[pix_model]) * W;
      if (!isfinite(dchisq)) continue;
      chisq += dchisq;
      npts ++;
    }
  }
  *Npts = npts;
  return chisq;
}
