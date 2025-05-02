# include "data.h"
# include "deimos.h"

# if (0) 
int deimos_fitobj (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);
  return FALSE;
}

# else

/*
  this is starting to work OK.  some improvements to make
  * use Gaussdev to sample
  * do not scale down range (or user-set scale-down)
 */

// internal functions to fitobj
static float      deimos_get_chisq (float *buffer, float *model, int Nx, int Ny, int row, int *Npts);
int              *deimos_sort_result (DeimosResult *result, int Nresult);
int               deimos_get_random_sample (int Nresult);
static opihi_flt *deimos_make_test (opihi_flt *guess, float *sigma, int row, int Nrow, int fullInput);

int USE_GAUSS_DEV = FALSE;

int deimos_fitobj (int argc, char **argv) {

  // input parameters:
  // * buffer      : 2D image of slit (full frame or cutout?)
  // * trace       : spline fit of slit central x pos vs y-coord
  // * profile     : slit window profile (vector)
  // * PSF         : point-spread function vector (flux normalized, x-dir)
  // * stilt       : slit tilt response : 2D kernel? 

  // in-out parameters:
  // * obj  : vector of object flux vs y-coord (starting guess and result)
  // * sky  : vector of local sky signal vs y-coord 
  // * bck  : vector of extra-slit background flux vs y-coord

  int N;

  Spline *trace   = NULL;
  Vector *profile = NULL;

  Vector *obj     = NULL;
  Vector *sky     = NULL;
  Vector *bck     = NULL;

  Vector *psf     = NULL;

  Buffer *buffer  = NULL;

  float stilt = 0.0; // angle of the slit
  ohana_gaussdev_init ();

  fprintf (stderr, "use deimos fitalt instead\n");
  return FALSE;

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
  float scale = 0.75;
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    scale = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }


  int Niter = 25;
  if ((N = get_argument (argc, argv, "-iter"))) {
    remove_argument (N, &argc, argv);
    Niter = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  int Ntry = 5;
  if ((N = get_argument (argc, argv, "-try"))) {
    remove_argument (N, &argc, argv);
    Ntry = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  int Nrow = 5;
  if ((N = get_argument (argc, argv, "-row"))) {
    remove_argument (N, &argc, argv);
    Nrow = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  USE_GAUSS_DEV = FALSE;
  if ((N = get_argument (argc, argv, "-gaussdev"))) {
    remove_argument (N, &argc, argv);
    USE_GAUSS_DEV = TRUE;
  }

  int SAVE_ALL_VECTORS = FALSE;
  if ((N = get_argument (argc, argv, "-save-all-vectors"))) {
    remove_argument (N, &argc, argv);
    SAVE_ALL_VECTORS = TRUE;
  }

  int SAVE_MIN_VECTORS = FALSE;
  if ((N = get_argument (argc, argv, "-save-min-vectors"))) {
    remove_argument (N, &argc, argv);
    SAVE_MIN_VECTORS = TRUE;
  }

  // Input parameters:
  if ((N = get_argument (argc, argv, "-trace"))) {
    remove_argument (N, &argc, argv);
    if ((trace = FindSpline (argv[N])) == NULL) return (FALSE);
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

  // for the cross-dispersion reference pixle, use the user value or set to Nx/2 if < 0
  deimos_set_cross_ref (atoi(argv[2]), Nx); 
  deimos_make_kernel (stilt, Nx);

  // the functions below accept an opihi_flt array for object, sky, background
  opihi_flt *objVal = obj->elements.Flt;
  opihi_flt *skyVal = sky->elements.Flt;
  opihi_flt *bckVal = bck->elements.Flt;

  int NRESULT = Niter + Ntry*Niter + 1;
  ALLOCATE_PTR (result, DeimosResult, NRESULT);

  Vector **objOut = NULL;
  Vector **skyOut = NULL;
  Vector **bckOut = NULL;
  Vector  *objMin = NULL;
  Vector  *skyMin = NULL;
  Vector  *bckMin = NULL;

  // Save all result vectors for each of obj, sky, bck:
  if (SAVE_ALL_VECTORS) {
    ALLOCATE (objOut, Vector *, NRESULT);
    ALLOCATE (skyOut, Vector *, NRESULT);
    ALLOCATE (bckOut, Vector *, NRESULT);
    for (int i = 0; i < NRESULT; i++) {
      char name[64];
      snprintf (name, 64, "obj_%04d", i); if ((objOut[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (objOut[i], OPIHI_FLT, Ny);
      snprintf (name, 64, "sky_%04d", i); if ((skyOut[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (skyOut[i], OPIHI_FLT, Ny);
      snprintf (name, 64, "bck_%04d", i); if ((bckOut[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (bckOut[i], OPIHI_FLT, Ny);
      for (int iy = 0; iy < Ny; iy++) {
	objOut[i]->elements.Flt[iy] = 0.0;
	skyOut[i]->elements.Flt[iy] = 0.0;
	bckOut[i]->elements.Flt[iy] = 0.0;
      }
    }
  }
  if (SAVE_MIN_VECTORS) {
    if ((objMin = SelectVector ("objMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (objMin, OPIHI_FLT, Ny);
    if ((skyMin = SelectVector ("skyMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (skyMin, OPIHI_FLT, Ny);
    if ((bckMin = SelectVector ("bckMin", ANYVECTOR, TRUE)) == NULL) { return (FALSE); } ResetVector (bckMin, OPIHI_FLT, Ny);
    for (int iy = 0; iy < Ny; iy++) {
      objMin->elements.Flt[iy] = 0.0;
      skyMin->elements.Flt[iy] = 0.0;
      bckMin->elements.Flt[iy] = 0.0;
    }
  }
  Vector *chisqVect;
  if ((chisqVect = SelectVector ("chisqOut", ANYVECTOR, TRUE)) == NULL) { return FALSE; }
  ResetVector (chisqVect, OPIHI_FLT, NRESULT);
  for (int i = 0; i < NRESULT; i++) { chisqVect->elements.Flt[i] = 0.0; }

  // XXX temp hack: save chisq subset vectors for each row pass
  int Npass = Ny / Nrow;
  ALLOCATE_PTR (chiPass, Vector *, Npass);
  for (int pass = 0; pass < Npass; pass++) {
    char name[64];
    snprintf (name, 64, "chiPass_%03d", pass);
    if ((chiPass[pass] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) { return FALSE; } ResetVector (chiPass[pass], OPIHI_FLT, NRESULT);
  }

  float noiseVar = SQ(noise);
  ALLOCATE_PTR (objNoise, float, Nrow);
  ALLOCATE_PTR (skyNoise, float, Nrow);
  ALLOCATE_PTR (bckNoise, float, Nrow);

  // work on segments of Nrow at a time
  // XXX worry about last segment (< Nrow rows)
  int pass = 0;
  for (int row = 0; row < Ny - Nrow; row += Nrow, pass++) {

    int Nresult = 0;

    // obj, sky, bck are initial guesses.  Also use these values to define a range for the initial guesses.

    // If I just use the value in a given bin to define the range, random noise for the
    // faint end could have bad results (range ~ 0.0).  For that matter, if I am in a
    // region with large dynamic range (e.g., on an emission or absorption line), then we
    // likely have large errors in how the guess follows the truth. 

    /** old concept:
    float objRange = 0, skyRange = 0, bckRange = 0;
    for (int iy = 0; iy < Nrow; iy++) {
      objRange = MAX(objVal[iy + row], objRange);
      skyRange = MAX(skyVal[iy + row], skyRange);
      bckRange = MAX(bckVal[iy + row], bckRange);
    }
    **/

    // new concept: start with 2-sigma range, then ramp down below
    for (int iy = 0; iy < Nrow; iy++) {
      objNoise[iy] = nsigma*sqrt(hypot(fabs(objVal[iy + row]) / gain, noiseVar));
      skyNoise[iy] = nsigma*sqrt(hypot(fabs(skyVal[iy + row]) / gain, noiseVar));
      bckNoise[iy] = nsigma*sqrt(hypot(fabs(bckVal[iy + row]) / gain, noiseVar));
    }

    // first test is the input guess
    if (1) { 
      opihi_flt *objtest = deimos_make_test (objVal, NULL, row, Nrow, TRUE);
      opihi_flt *skytest = deimos_make_test (skyVal, NULL, row, Nrow, TRUE);
      opihi_flt *bcktest = deimos_make_test (bckVal, NULL, row, Nrow, TRUE);
      
      // generate the model based on the test values
      // objtest, skytest, bcktest are segments of the full parameter vectors
      float *model = deimos_make_model (objtest, skytest, bcktest, psf, profile, trace, NULL, Nx, Nrow, row);
      
      // calculate the current chisq
      // XXX need to add in the per-pixel error or variance
      int Npts = 0;
      float chisq = deimos_get_chisq (bufVal, model, Nx, Nrow, row, &Npts);
      fprintf (stderr, "guess: chisq: %f, Npts: %d\n", chisq, Npts);
      free (model);

      if (pass < Npass) {
	chiPass[pass]->elements.Flt[Nresult] = chisq / (float) Npts;
      }

      // save the results
      result[Nresult].chisq = chisq / (float) Npts;
      result[Nresult].obj = objtest;
      result[Nresult].sky = skytest;
      result[Nresult].bck = bcktest;
      Nresult ++;
    }

    // run Niter iterations
    for (int iter = 0; iter < Niter; iter ++) {

      // generate a set of test values for obj, sky, backgnd based on the current guess
      opihi_flt *objtest = deimos_make_test (objVal, objNoise, row, Nrow, TRUE);
      opihi_flt *skytest = deimos_make_test (skyVal, skyNoise, row, Nrow, TRUE);
      opihi_flt *bcktest = deimos_make_test (bckVal, bckNoise, row, Nrow, TRUE);
      
      for (int itmp = 0; FALSE && (itmp < Nrow); itmp++) {
	fprintf (stderr, "val: %f %f %f, tst: %f %f %f\n",
		 objVal[itmp + row],  skyVal[itmp + row],  bckVal[itmp + row], 
		 objtest[itmp], skytest[itmp], bcktest[itmp]);
      }

      // generate the model based on the test values
      // objtest, skytest, bcktest are segments of the full parameter vectors
      float *model = deimos_make_model (objtest, skytest, bcktest, psf, profile, trace, NULL, Nx, Nrow, row);
      
      // calculate the current chisq
      // XXX need to add in the per-pixel error or variance
      int Npts = 0;
      float chisq = deimos_get_chisq (bufVal, model, Nx, Nrow, row, &Npts);
      // fprintf (stderr, "iter: %d, chisq: %f, Npts: %d\n", iter, chisq, Npts);
      free (model);

      if (pass < Npass) {
	chiPass[pass]->elements.Flt[Nresult] = chisq / (float) Npts;
      }

      // save the results
      result[Nresult].chisq = chisq / (float) Npts;
      result[Nresult].obj = objtest;
      result[Nresult].sky = skytest;
      result[Nresult].bck = bcktest;
      Nresult ++;
    }
      
    for (int try = 0; try < Ntry; try ++) {
      // generate an index of results sorted by chisq
      int *IDX = deimos_sort_result (result, Nresult);

      int Nnewset = 0;

      // XXX not sure this scaling factor concept is good
      for (int iy = 0; iy < Nrow; iy++) {
	objNoise[iy] = scale*objNoise[iy];
	skyNoise[iy] = scale*skyNoise[iy];
	bckNoise[iy] = scale*bckNoise[iy];
      }

      // run Niter iterations, selecting guess values from the result set
      for (int iter = 0; iter < Niter; iter ++) {
	int bin = deimos_get_random_sample (Nresult);
	int entry = IDX[bin];
	// fprintf (stderr, "try: %d : %d, %f\n", bin, entry, result[entry].chisq);

	// generate a set of test values for obj, sky, backgnd based on the current guess
	opihi_flt *objtest = deimos_make_test (result[entry].obj, objNoise, row, Nrow, FALSE);
	opihi_flt *skytest = deimos_make_test (result[entry].sky, skyNoise, row, Nrow, FALSE);
	opihi_flt *bcktest = deimos_make_test (result[entry].bck, bckNoise, row, Nrow, FALSE);
      
	for (int itmp = 0; FALSE && (itmp < Nrow); itmp++) {
	  fprintf (stderr, "val: %f %f %f, tst: %f %f %f\n",
		   result[entry].obj[itmp], result[entry].sky[itmp], result[entry].bck[itmp], 
		   objtest[itmp], skytest[itmp], bcktest[itmp]);
	}

	// generate the model based on the test values
	float *model = deimos_make_model (objtest, skytest, bcktest, psf, profile, trace, NULL, Nx, Nrow, row);
      
	// calculate the current chisq
	int Npts = 0;
	float chisq = deimos_get_chisq (bufVal, model, Nx, Nrow, row, &Npts);
	// fprintf (stderr, "try: %d, iter: %d, chisq: %f, Npts: %d\n", try, iter, chisq, Npts);
	free (model);

	if (pass < Npass) {
	  chiPass[pass]->elements.Flt[Nresult + Nnewset] = chisq / (float) Npts;
	}

	// save the results
	result[Nresult + Nnewset].chisq = chisq / (float) Npts;
	result[Nresult + Nnewset].obj = objtest;
	result[Nresult + Nnewset].sky = skytest;
	result[Nresult + Nnewset].bck = bcktest;
	Nnewset ++;
      }
      Nresult += Nnewset;
    }

    int *IDX = deimos_sort_result (result, Nresult);

    // ENDING: save the best result and info about the allowed chisq region
    // int *index = sort_result (result, Nresult);
    // copy result[index[0]].obj,sky,bck to obj,sky,bck
    for (int i = 0; i < Nresult; i++) {
      for (int iy = 0; iy < Nrow; iy++) {
	if (SAVE_ALL_VECTORS) {
	  objOut[i]->elements.Flt[iy + row] = result[IDX[i]].obj[iy];
	  skyOut[i]->elements.Flt[iy + row] = result[IDX[i]].sky[iy];
	  bckOut[i]->elements.Flt[iy + row] = result[IDX[i]].bck[iy];
	}
      }
      chisqVect->elements.Flt[i] += result[IDX[i]].chisq;
    }
    if (SAVE_MIN_VECTORS) {
      for (int iy = 0; iy < Nrow; iy++) {
	objMin->elements.Flt[iy + row] = result[IDX[0]].obj[iy];
	skyMin->elements.Flt[iy + row] = result[IDX[0]].sky[iy];
	bckMin->elements.Flt[iy + row] = result[IDX[0]].bck[iy];
      }
    }
  }

  return TRUE;

 usage:
  gprint (GP_ERR, "USAGE: deimos fitobj (buffer) (Xref) -object vector -sky vector -backgnd vector -trace spline -profile vector -psf vector -stilt (angle)\n");
  return FALSE;
}

/****************** fitobj Support Functions *******************/

static opihi_flt *deimos_make_test (opihi_flt *guess, float *sigma, int row, int Nrow, int fullInput) {

  // we have a vector, guess, with some length >= row + Nrow.
  // generate a set of random values in the vicinty of guess[row] to guess[row+Nrow]
  // return vector is of length Nrow

  // if fullInput is TRUE,  the guess vector runs from 0 to Ny (full wavelength range)
  // if fullInput is FALSE, the guess vector runs from row to row + Nrow (subset range)
  // sigma is always only Nrow long

  ALLOCATE_PTR (value, opihi_flt, Nrow);

  int start_pix = fullInput ? row : 0;
  int stop_pix  = fullInput ? row + Nrow : Nrow;

  int iv = 0;
  for (int i = start_pix; i < stop_pix; i++, iv++) {
    if (!sigma) {
      value[iv] = guess[i];
      continue;
    }
    if (USE_GAUSS_DEV) {
      value[iv] = ohana_gaussdev_rnd(guess[i], sigma[iv]);
      continue;
    } 
    
    // uniform distribution:
    value[iv] = sigma[iv]*(drand48() - 0.5) + guess[i];
  }
  return value;
}


static float deimos_get_chisq (float *buffer, float *model, int Nx, int Ny, int row, int *Npts) {

  int npts = 0;
  float chisq = 0;

  for (int iy = 0; iy < Ny; iy++) {

    for (int ix = 0; ix < Nx; ix++) {

      int pix_buffer = ix + (iy + row)*Nx;
      int pix_model  = ix +  iy*Nx;

      if (!isfinite(buffer[pix_buffer])) continue;
      if (!isfinite(model[pix_model])) continue;
      chisq += SQ(buffer[pix_buffer] - model[pix_model]);
      npts ++;
    }
  }
  *Npts = npts;
  return chisq;
}

int *deimos_sort_result (DeimosResult *result, int Nresult) {

  ALLOCATE_PTR (IDX,   int,   Nresult);
  ALLOCATE_PTR (chisq, float, Nresult);

  for (int i = 0; i < Nresult; i++) {
    IDX[i] = i;
    chisq[i] = result[i].chisq;
  }

  sort_float_index (chisq, IDX, Nresult);
  free (chisq);

  return IDX;
}

static float DEIMOS_A = 2.0; // value for random sample

int deimos_get_random_sample (int Nresult) {

  // choose a bin from the range 0 - Nresult, with front-loaded weighting.

  // bin = Nresult * (exp(q) - exp(-A)) / (exp(A) - exp(-A))
  // q = 2A(x - 1/2) : random number between -A and +A
  // x : random number between 0 and 1

  // f(q;A) = (exp(q) - exp(-A)) / (exp(A) - exp(-A))
  // f(q;A) ranges from 0 to +1 as x ranges from 0 to +1

  float x = drand48();
  float q = 2*DEIMOS_A*(x - 0.5);
  float f = (exp(q) - exp(-DEIMOS_A)) / (exp(DEIMOS_A) - exp(-DEIMOS_A));
  // XXX optimization : precalculate and save the elements of this function
  
  int bin = floor(Nresult * f);

  return bin;
}
# endif
