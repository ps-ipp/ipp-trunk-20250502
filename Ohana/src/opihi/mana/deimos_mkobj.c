# include "data.h"

int deimos_mkobj (int argc, char **argv) {

  // generate model observed flux for an object in a slit with background

  // input parameters:
  // * slit_trace_red : spline fit of slit central x pos vs y-coord for red chip
  // * slit_trace_blu : spline fit of slit central x pos vs y-coord for blu chip
  // * psf_trace      : spline fit of psf central x pos vs y-coord
  // * profile        : slit window profile (vector)
  // * object         : vector of object flux vs y-coord 
  // * sky            : vector of local sky signal vs y-coord
  // * backgnd        : vector of extra-slit background flux vs y-coord
  // * PSF            : point-spread function vector (flux normalized, x-dir)
  // * LSF            : sline-spread function vector (flux normalized, y-dir)
  // * stilt          : slit tilt response : 2D kernel? 

  // * redlimit       : pixel break between red and blu chips
  // * Nwave          : number of wavelength pixels in output image

  int N;

  // if any of these are not defined, they will have assumed identity values
  Spline *slit_trace_red = NULL;
  Spline *slit_trace_blu = NULL;
  Spline *psf_trace  = NULL;
  Vector *profile    = NULL;
  Vector *object     = NULL;
  Vector *sky        = NULL;
  Vector *backgnd    = NULL;
		     
  Vector *psf        = NULL;
  Vector *lsf        = NULL;
  // add tilt later

  Buffer *output     = NULL;

  float stilt = 0.0; // angle of the slit

  // for a red vs blu spline, we need to specify the split point
  // XXX this is REALLY ad-hoc for Deimos.  not sure how to make
  // this more generic (need to define the ranges somewhere)
  int redlimit = 4096;
  if ((N = get_argument (argc, argv, "-redlimit"))) {
    remove_argument (N, &argc, argv);
    redlimit = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-slit-trace-red"))) {
    remove_argument (N, &argc, argv);
    if ((slit_trace_red = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-slit-trace-blu"))) {
    remove_argument (N, &argc, argv);
    if ((slit_trace_blu = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-psf-trace"))) {
    remove_argument (N, &argc, argv);
    if ((psf_trace = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-profile"))) {
    remove_argument (N, &argc, argv);
    if ((profile = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-object"))) {
    remove_argument (N, &argc, argv);
    if ((object = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    if ((sky = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-backgnd"))) {
    remove_argument (N, &argc, argv);
    if ((backgnd = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-psf"))) {
    remove_argument (N, &argc, argv);
    if ((psf = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-lsf"))) {
    remove_argument (N, &argc, argv);
    if ((lsf = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-stilt"))) {
    remove_argument (N, &argc, argv);
    stilt = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // either flux or fluxbins must be specified to define output size
  int Nwave = 0;
  if ((N = get_argument (argc, argv, "-Nwave"))) {
    remove_argument (N, &argc, argv);
    Nwave = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: deimos mkobj (buffer) (Ncross) [-object vector] [-sky vector] [-backgnd vector] [-slit-trace-red spline] [-slit-trace-blu spline] [-psf-trace spline] [-profile vector] [-psf vector] [-lsf vector] [-stilt angle] [-Nwave Npixel] [-redlimit pixel]\n");
    gprint (GP_ERR, "  slit_trace_red : spline fit of slit central x pos vs y-coord for red chip\n");
    gprint (GP_ERR, "  slit_trace_blu : spline fit of slit central x pos vs y-coord for blu chip\n");
    gprint (GP_ERR, "  psf_trace      : spline fit of psf central x pos vs y-coord\n");
    gprint (GP_ERR, "  profile        : slit window profile (vector)\n");
    gprint (GP_ERR, "  object         : vector of object flux vs y-coord \n");
    gprint (GP_ERR, "  sky            : vector of local sky signal vs y-coord\n");
    gprint (GP_ERR, "  backgnd        : vector of extra-slit background flux vs y-coord\n");
    gprint (GP_ERR, "  PSF            : point-spread function vector (flux normalized, x-dir)\n");
    gprint (GP_ERR, "  LSF            : sline-spread function vector (flux normalized, y-dir)\n");
    gprint (GP_ERR, "  stilt          : slit tilt response : 2D kernel? \n");
    gprint (GP_ERR, "  redlimit       : pixel break between red and blu chips\n");
    gprint (GP_ERR, "  Nwave          : number of wavelength pixels in output image\n");
    return FALSE;
  }

  int waveVector = (object != NULL) || (sky != NULL) || (backgnd != NULL);

  if (!waveVector && !Nwave) {
    gprint (GP_ERR, "either flux or fluxbins must be specified to define output size\n");
    return FALSE;
  }
  if (waveVector && Nwave) {
    gprint (GP_ERR, "-Nwave incompatible with -object, -sky, -backgnd\n");
    return FALSE;
  }

  // all supplied vectors must be consistent 
  int Ny = object ? object->Nelements : Nwave;
  if (sky && Ny) {
    if (Ny != sky->Nelements) {
      gprint (GP_ERR, "inconsistent wavelength scales (sky)\n");
      return FALSE;
    }
  } else {
    Ny = sky ? sky->Nelements : Ny;
  }
  if (backgnd && Ny) {
    if (Ny != backgnd->Nelements) {
      gprint (GP_ERR, "inconsistent wavelength scales (backgnd)\n");
      return FALSE;
    }
  } else {
      Ny = backgnd ? backgnd->Nelements : Ny;
  }

  if ((output = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  int NxBase = atoi(argv[2]);
  if (NxBase % 2 == 0) NxBase ++; // force Nx to be odd
  int Nx = NxBase;
  
  // if we are appying a trace offset spline, we need to generate an output window which
  // is Nx + the full swing of the trace, then window back down
  if (slit_trace_red && slit_trace_blu) {
    float dXmin = +1000;
    float dXmax = -1000;
    for (int iy = 0; iy < Ny; iy++) {
      // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
      Spline *slit_trace = (iy < redlimit) ? slit_trace_red : slit_trace_blu;
      float dx = spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy);
      dXmin = MIN (dx, dXmin);
      dXmax = MAX (dx, dXmax);
    }
    int dXrange = ceil(dXmax - dXmin);
    if (dXrange % 2) dXrange ++; // force dXrange to be even (so Nx will be odd)
    Nx += dXrange;
  }

  ResetBuffer (output, Nx, Ny, -32, 0.0, 1.0);

  opihi_flt *objV = (opihi_flt *) (object ? object->elements.Flt : NULL);
  opihi_flt *skyV = (opihi_flt *) (sky    ? sky->elements.Flt : NULL);
  opihi_flt *psfV = (opihi_flt *) (psf    ? psf->elements.Flt : NULL);
  opihi_flt *lsfV = (opihi_flt *) (lsf    ? lsf->elements.Flt : NULL);

  // psf vector must have odd number of pixels:
  int Npsf = 0;
  int NpsfFull = 0;
  if (psf) {
    if (psf->Nelements % 2 == 0) {
      gprint (GP_ERR, "psf vector must have an odd number of pixels\n");
      return FALSE;
    }
    NpsfFull = psf->Nelements;
    Npsf = NpsfFull / 2;
  }

  // lsf vector must have odd number of pixels:
  int Nlsf = 0;
  int NlsfFull = 0;
  if (lsf) {
    if (lsf->Nelements % 2 == 0) {
      gprint (GP_ERR, "lsf vector must have an odd number of pixels\n");
      return FALSE;
    }
    NlsfFull = lsf->Nelements;
    Nlsf = NlsfFull / 2;
  }

  // *** apply the PSF to the object flux, add in the sky flux

  ALLOCATE_PTR (s1, float, Nx*Ny);

  // loop over the y positions
  for (int iy = 0; iy < Ny; iy++) {

    opihi_flt objVy = objV ? objV[iy] : 1.0; // if object is not supplied, flux defaults to 1.0
    opihi_flt skyVy = skyV ? skyV[iy] : 0.0; // if sky is not supplied, flux defaults to 0.0
      
    int Npof = floor(Nx/2);
    if (psf) Npof -= Npsf;

    // integral and fractional pixel offsets of PSF (none, by default)
    int dxi = 0;
    float dxf = 0.0, dxr = 1.0;
    
    if (psf_trace) {
      float dx = spline_apply_dbl (psf_trace->xk, psf_trace->yk, psf_trace->y2, psf_trace->Nknots, iy);
      dxi = floor(dx);
      dxf = dx - dxi;  // -1.7 -> +0.3, -0.5 -> +0.5, +0.5 ->+0.5, +1.7 -> +0.7
      dxr = 1 - dxf;
      Npof += dxi;
      // fprintf (stderr, "%d: %f : %d\n", iy, dx, Npof);
      if (VERBOSE && (iy % 200 == 0)) { gprint (GP_ERR, "%d : %f : %d : %f %f\n", iy, dx, dxi, dxf, dxr); }
    }

    // if fractional offset is small, do not interpolate
    int doInterp = fabs(dxf) < 1e-5 ? FALSE : TRUE;

    // flux = obj * PSF + sky
    for (int ix = 0; ix < Nx; ix++) {
      
      opihi_flt value = skyVy;

      if (psfV) {
	int n = ix - Npof;
	// n is the pixel in the PSF corresponding to the ix pixel in the output image
	// only add in the flux if we are in range of the PSF
	if ((n >= 0) && (n < NpsfFull)) {
	  if (doInterp) {
	    if ((n > 0) && (n < NpsfFull)) {
	      if (isfinite(psfV[n]) && isfinite(psfV[n-1])) {
		value += objVy*psfV[n]*dxr + objVy*psfV[n-1]*dxf;
	      }
	    }
	    if (n == 0) {
	      if (isfinite(psfV[n])) {
		value += objVy*psfV[n]*dxr;
	      }
	    }
	  } else {
	    if (isfinite(psfV[n])) {
	      value += objVy*psfV[n];
	    }
	  }
	}
      } else {
	if (ix == Nx / 2) value += objVy;
      }	  
      s1[ix + iy*Nx] = value;
    }
  }

  // *** apply the slit tilt & lsf

  // first, generate the interpolation kernels.  For a slit tilt of theta, there is a
  // displacement in the y-direction of dy = dx sin(theta) where dx is the x-coord
  // relative to the slit window center (Nx / 2).  we thus need an image of size Nx,
  // Nx*sin(theta)

  float sin_stilt = sin(stilt*RAD_DEG);
  int Nyk = ceil(Nx * fabs(sin_stilt)) + 1; // one extra pixel buffer
  if (Nyk < 3) Nyk = 3; // minimum of 3 pixels (or kernel assumption below fails)
  Nyk += NlsfFull;
  if (Nyk % 2 == 0) Nyk ++; // force Nyk to be odd
  int Nyk2 = Nyk/2; // Nyk2 is the center pixel of the interpolations
  ALLOCATE_PTR (kernel, float, Nx*Nyk);
  int Nkpix = Nx*Nyk;

  int Nx2 = floor(Nx/2);
  for (int ix = 0; ix < Nx; ix++) {
    for (int iy = 0; iy < Nyk; iy++) { kernel[ix + iy*Nx] = 0.0; }

    // displacement in y-dir due to slit tilt:
    float dy = (ix - Nx2) * sin_stilt;
    int dyi = floor(dy);
    float dyf = dy - dyi;

    if (lsf) {
      int Sy = Nyk2 - Nlsf + dyi;
      int doInterp = (fabs(dyf) < 1e-5) ? FALSE : TRUE;
      for (int iy = 0; iy < Nyk; iy++) {
	int py = iy - Sy;
	if (py < 0) continue;
	if (py >= NlsfFull) continue;

	float vout = NAN;
	if (doInterp) {
	  if ((py > 0) && (py < NlsfFull - 1)) {
	    vout = lsfV[py]*(1 - dyf) + lsfV[py-1]*dyf;
	  }
	  if (py == 0) {
	    vout = lsfV[py]*dyf;
	  }
	  if (py == NlsfFull - 1) {
	    vout = lsfV[py]*(1.0 - dyf);
	  }
	} else {
	  vout = lsfV[py];
	}
	int Nkpix_i = ix + iy*Nx;
	myAssert (Nkpix_i < Nkpix, "oops");
	kernel[Nkpix_i] = vout;
      }
    } else {
      int Nkpix_i;
      Nkpix_i = ix + (dyi + Nyk2 + 0)*Nx;
      myAssert (Nkpix_i < Nkpix, "oops");
      kernel[Nkpix_i] = 1.0 - dyf;

      Nkpix_i = ix + (dyi + Nyk2 + 1)*Nx;
      myAssert (Nkpix_i < Nkpix, "oops");
      kernel[Nkpix_i] = dyf;
      fprintf (stderr, "%d, %d: %f %f\n", ix, (dyi + Nyk2), kernel[ix + (dyi + Nyk2 + 0)*Nx], kernel[ix + (dyi + Nyk2 + 1)*Nx]);
    }
  }

  float *out = (float *) output[0].matrix.buffer;

  // next, apply the interpolation kernel to the image

  // loop over the y positions
  for (int iy = 0; iy < Ny; iy++) {

    // use the tilt kernel to interpolate to this x coordinate
    for (int ix = 0; ix < Nx; ix++) {

      // if the LSF is defined, we cannot bypass the convolution
      if (!lsf && (fabs(stilt) < 1e-5)) {
	out[ix + iy*Nx] = s1[ix + iy*Nx];
      } else {
	opihi_flt g = 0.0, s = 0.0;
	for (int n = -Nyk2; n <= Nyk2; n++) {
	  if (iy - n < 0) continue; // bottom edge of full image
	  if (iy - n >= Ny) continue; // top edge of full image
	  int Nkpix_i = ix + (n + Nyk2)*Nx;
	  myAssert (Nkpix_i < Nkpix, "oops");
	  s += kernel[Nkpix_i]*s1[ix + (iy - n)*Nx];
	  g += kernel[Nkpix_i];
	}
	out[ix + iy*Nx] = s / g;
      }
    }
  }
  free (kernel);
  free (s1);
  
  // next, apply the profile (output = input * profile)
  opihi_flt *profileV = (opihi_flt *) (profile ? profile->elements.Flt : NULL);
  if (profile) {
    int Nprof = profile->Nelements;
    int Nprof2 = Nprof / 2;

    // loop over the y positions
    for (int iy = 0; iy < Ny; iy++) {
      
      int Sx = (int)(Nx2 - Nprof2);
      
      for (int ix = 0; ix < Nx; ix++) {
	
	// equivalent coord in the profile:
	int px = ix - Sx;
	if ((px >= 0) && (px < Nprof)) {
	  out[ix + iy*Nx] *= profileV[px];
	} else {
	  out[ix + iy*Nx] = 0.0;
	}
      }
    }
  }

  // add in the background
  opihi_flt *backgndV = (opihi_flt *) (backgnd ? backgnd->elements.Flt : NULL);
  if (backgnd) {
    for (int iy = 0; iy < Ny; iy++) {
      for (int ix = 0; ix < Nx; ix++) {
	out[ix + iy*Nx] += backgndV[iy];
      }
    }
  }

  // shift pixels in x based on the trace
  if (slit_trace_red && slit_trace_blu) {
    ALLOCATE_PTR (outTraceBuffer, char, NxBase*Ny*sizeof(float));
    float *outTrace = (float *) outTraceBuffer;
    int NxBase2 = NxBase / 2;

    for (int iy = 0; iy < Ny; iy++) {

      // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
      Spline *slit_trace = (iy < redlimit) ? slit_trace_red : slit_trace_blu;
      float dx = -spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy);
      
      // extract the integer pixel offset and the fractional offset
      int dxi = floor(dx); // -1.7 -> -2, -0.5 -> -1, +0.5 -> 0, +1.7 -> 1
      float dxf = dx - dxi;  // -1.7 -> +0.3, -0.5 -> +0.5, +0.5 ->+0.5, +1.7 -> +0.7
      float dxr = 1 - dxf;

      // if fractional offset is small, do not interpolate
      int doInterp = fabs(dxf) < 1e-5 ? FALSE : TRUE;
      
      // How do I handle this?  define a temporary window which is a fraction of the full output window?
      int Sx = Nx2 - NxBase2 + dxi;
 
      // save the original output row
      // for (int ix = 0; ix < Nx; ix++) { tmprow[ix] = out[ix + iy*Nx]; }

      for (int ix = 0; ix < NxBase; ix++) {

	// equivalent coord in temporary buffer (Nx * Ny):
	int px = ix + Sx;
	if (px < 0) {
	  outTrace[ix + iy*NxBase] = out[iy*Nx];
	  continue;
	}
	if (px >= Nx) {
	  outTrace[ix + iy*NxBase] = out[(Nx - 1) + iy*Nx];
	  continue;
	}
	
	// a default value:
	float vout = NAN;
	
	int Npix = px + iy*Nx;

	if (doInterp) {
	  if ((px > 0) && (px < Nx - 1)) {
	    vout = out[Npix]*dxr + out[Npix + 1]*dxf;
	  }
	  if (px == 0) {
	    vout = out[Npix];
	  }
	  if (px == Nx - 1) {
	    vout = out[Npix];
	  }
	} else {
	  vout = out[Npix];
	}
	outTrace[ix + iy*NxBase] = vout;
      }
    }

    ResetBuffer (output, NxBase, Ny, -32, 0.0, 1.0);
    free (output[0].matrix.buffer);
    output[0].matrix.buffer = outTraceBuffer;
  }

  return TRUE;
}

  /* 
    // convolve the object flux at this y-coord with PSF and add sky
    for (int ix = 0; i < Nx; i++) {
      opihi_flt g = s = 0;
      for (int n = -Npsf; n <= Npsf; n++) {
	if (ix + n < 0) continue;
	if (ix + n >= Nx) continue;
	opihi_flt value = objV ? objV[ix+n] : 1.0;
	s += psfV[n]*value;
	g += psfV[n];
      }
      out[ix + iy*Nx] = Npsf ? s / g : (objV ? objV[ix+n] : 1.0);
      if (skyV) {
	out[ix + iy*Nx] += skyV[iy]
    }
  */
  
