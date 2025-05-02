# include "data.h"
# include "deimos.h"

// global parameters of the slit kernel:
static int   Nkernel_pix = 0; // total number of pixels
static int   Nkernel_y2 = 0;  // center of the interpolation 
static float *kernel = NULL; 
static int   SLIT_NO_INTERP = FALSE; // if slit tilt is minimal, do not interpolate

static int   Xref = -1;       // center of the slit profile

/****************** make_object & support functions *******************/

void deimos_set_cross_ref (int value, int Nx) {
  Xref = value;
  if (Xref < 0) { Xref = floor(Nx/2); }
}

// kernel for slit tilt; no LSF for now
void deimos_make_kernel (float stilt, int Nx) {

  // first, generate the interpolation kernels.  For a slit tilt of theta, there is a
  // displacement in the y-direction of dy = dx sin(theta) where dx is the x-coord
  // relative to the slit window center (Nx / 2).  we thus need an image of size Nx,
  // Nx*sin(theta)

  SLIT_NO_INTERP = (fabs(stilt) < 1e-5);

  float sin_stilt = sin(stilt*RAD_DEG);
  int Ny = ceil(Nx * fabs(sin_stilt)) + 1; // one extra pixel buffer
  if (Ny < 3) Ny = 3; // minimum of 3 pixels (or kernel assumption below fails)

  if (Ny % 2 == 0) Ny ++; // force Nyk to be odd

  Nkernel_y2 = Ny/2; // Nkernel_y2 is the center pixel in the interpolation direction

  ALLOCATE (kernel, float, Nx*Ny);
  Nkernel_pix = Nx*Ny;
  // fprintf (stderr, "allocating %d pixels (%d,%d)\n", Nkernel_pix, Nx, Ny);

  // XXX use Xref not Nx/2?
  int Nx2 = floor(Nx/2);

  for (int ix = 0; ix < Nx; ix++) {
    for (int iy = 0; iy < Ny; iy++) { kernel[ix + iy*Nx] = 0.0; }

    // displacement in y-dir due to slit tilt:
    float dy = (ix - Nx2) * sin_stilt;
    int dyi = floor(dy);
    float dyf = dy - dyi;

    // without LSF, slit tilt only affects two neighbor pixels
    int pix;
    pix = ix + (dyi + Nkernel_y2 + 0)*Nx;
    // fprintf (stderr, "%d, %d, %d -- ", ix, (dyi + Nkernel_y2 + 0), pix);
    myAssert (pix < Nkernel_pix, "oops (make_kernel 1)");
    kernel[pix] = 1.0 - dyf;

    pix = ix + (dyi + Nkernel_y2 + 1)*Nx;
    // fprintf (stderr, "%d, %d, %d -- ", ix, (dyi + Nkernel_y2 + 1), pix);
    myAssert (pix < Nkernel_pix, "oops (make_kernel 2)");
    kernel[pix] = dyf;
    // fprintf (stderr, ": %f %f\n", kernel[ix + (dyi + Nkernel_y2 + 0)*Nx], kernel[ix + (dyi + Nkernel_y2 + 1)*Nx]);
  }
}

void deimos_free_kernel () {
  free (kernel);
}

/* deimos_make_model generates a model image for a subset of the 
   wavelength range, starting at 'row' and going for Ny rows.  

   we have a choice here.  we could:
   
   1) assume obj,sky,bck are only segments corresponding to the same wavelength range
      (we still need to use 'row' to get the right trace values)

   2) require obj,sky,bck to correspond to the full wavelength range (pass in 'row' to get
      the correct values).

   I think (1) simplified the situation the most.

 */

// generate a subimage of the full model for the range row - row + Ny
// obj,sky,bck are subset vectors corresponding to the range row to row+Ny
// the slit_trace_* and psf_trace refer to the full image starting at row
float *deimos_make_model (opihi_flt *obj, opihi_flt *sky, opihi_flt *bck, Vector *psf, Vector *profile, Spline *slit_trace_red, Spline *slit_trace_blu, float redlimit, Spline *psf_trace, int Nx, int Ny, int row) {

  float *raw = deimos_make_straight_image (obj, sky, psf, psf_trace, Nx, Ny, row);
  float *new = deimos_apply_tilt (raw, Nx, Ny);
  deimos_apply_profile (profile, new, Nx, Ny);
  deimos_add_background (bck, new, Nx, Ny, row);
  float *out = deimos_apply_trace (slit_trace_red, slit_trace_blu, redlimit, new, Nx, Ny, row);

  free (raw);
  free (new);

  return out;
}

// *************** make_model support functions *****************

// *** generate a raw image with no slit tilt and no trace offsets.  apply the PSF to the
// object flux, add in the sky flux
float *deimos_make_straight_image (opihi_flt *obj, opihi_flt *sky, Vector *psf, Spline *psf_trace, int Nx, int Ny, int row) {
  OHANA_UNUSED_PARAM(row);
  
  opihi_flt *psfV = psf->elements.Flt;

  int NpsfFull = psf->Nelements;
  int Npsf = NpsfFull / 2;

  // Xref is a global supplied by the user (defaults to Nx/2 if < 0)
  int XoffRef = (int)(Xref - Npsf);

  // Nx : width of output window
  // Ny : number of output rows
  ALLOCATE_PTR (out, float, Nx*Ny);
  
  // loop over the y positions in the output image:
  for (int iy = 0; iy < Ny; iy++) {
    
    // we are generating the image for just the row range row to row+Ny
    // note that obj & sky are subset vectors for just this range of pixels
    opihi_flt objVy = obj[iy]; // if we shift to using the row offset: obj[iy + row]
    opihi_flt skyVy = sky[iy]; // if we shift to using the row offset: sky[iy + row]
      
    // integral and fractional pixel offsets of PSF (none, by default)
    int dxi = 0;
    float dxf = 0.0, dxr = 1.0;
    int Xoff = XoffRef;
    
    if (psf_trace) {
      // the psf_trace is referenced against the full image, so we need to add row to iy:
      float dx = spline_apply_dbl (psf_trace->xk, psf_trace->yk, psf_trace->y2, psf_trace->Nknots, iy + row);
      dxi = floor(dx);
      dxf = dx - dxi;  // -1.7 -> +0.3, -0.5 -> +0.5, +0.5 ->+0.5, +1.7 -> +0.7
      dxr = 1 - dxf;
      Xoff += dxi;
      // fprintf (stderr, "%d: %f : %d\n", iy + row, dx, Xoff);
    }

    // if fractional offset is small, do not interpolate
    int doInterp = fabs(dxf) < 1e-5 ? FALSE : TRUE;

    // flux = obj * PSF + sky
    for (int ix = 0; ix < Nx; ix++) {
      
      opihi_flt value = skyVy;
      
      int n = ix - Xoff;

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
	    value += objVy * psfV[n];
	  }
	}
      }
      out[ix + iy*Nx] = value;
    }
  }
  return out;
}

// the slit tilt shifts the flux in the dispersion direction an amount which depends on
// the cross-dispertion position.  the shift is not a function of row position.
float *deimos_apply_tilt (float *input, int Nx, int Ny) {

  // Nx : width of output window
  // Ny : number of output rows
  ALLOCATE_PTR (output, float, Nx*Ny);
  
  // loop over the y positions
  for (int iy = 0; iy < Ny; iy++) {

    // use the tilt kernel to interpolate to this x coordinate
    for (int ix = 0; ix < Nx; ix++) {

      // if the LSF is defined, we cannot bypass the convolution
      if (SLIT_NO_INTERP) {
	output[ix + iy*Nx] = input[ix + iy*Nx];
      } else {
	opihi_flt g = 0.0, s = 0.0;
	for (int n = -Nkernel_y2; n <= Nkernel_y2; n++) {
	  if (iy - n < 0) continue; // bottom edge of full image
	  if (iy - n >= Ny) continue; // top edge of full image
	  int pix = ix + (n + Nkernel_y2)*Nx;
	  myAssert (pix < Nkernel_pix, "oops (apply_tilt)");
	  s += kernel[pix]*input[ix + (iy - n)*Nx];
	  g += kernel[pix]; // optimization: save g[ix], all should be the same
	}
	output[ix + iy*Nx] = s / g;
      }
    }
  }
  return output;
}

// the slit window profile modifies the flux (essentially a vignetting term).
// the profile is constant vs row (do not need a row value)
void deimos_apply_profile (Vector *profile, float *out, int Nx, int Ny) {

  // next, apply the profile (output = input * profile)
  opihi_flt *profileV = profile->elements.Flt;

  int Nprof = profile->Nelements;
  int Nprof2 = Nprof / 2;

  // Xref is a global supplied by the user (defaults to Nx/2 if < 0)
  int Xoff = (int)(Xref - Nprof2);

  // loop over the y positions
  for (int iy = 0; iy < Ny; iy++) {
      
    for (int ix = 0; ix < Nx; ix++) {
      
      // equivalent coord in the profile:
      int pix = ix - Xoff;
      if ((pix >= 0) && (pix < Nprof)) {
	out[ix + iy*Nx] *= profileV[pix];
      } else {
	out[ix + iy*Nx] = 0.0;
      }
    }
  }
}  

// supplied background model covers full dispersion range, not the sub-image
void deimos_add_background (opihi_flt *bck, float *out, int Nx, int Ny, int row) {
  OHANA_UNUSED_PARAM(row);

  for (int iy = 0; iy < Ny; iy++) {
    for (int ix = 0; ix < Nx; ix++) {
      out[ix + iy*Nx] += bck[iy]; // if we shift to using the row offset: bck[iy + row]
    }
  }
}  

// shift pixels in x based on the trace
float *deimos_apply_trace (Spline *slit_trace_red, Spline *slit_trace_blu, float redlimit, float *input, int Nx, int Ny, int row) {

  ALLOCATE_PTR (output, float, Nx*Ny);

  for (int iy = 0; iy < Ny; iy++) {

    // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
    // NOTE: spline is evaluated at full dispersion coordinate
    int iy_full = iy + row;
    Spline *slit_trace = (iy_full < redlimit) ? slit_trace_red : slit_trace_blu;
    float dx = -spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy_full);
      
    // extract the integer pixel offset and the fractional offset
    int dxi = floor(dx); // -1.7 -> -2, -0.5 -> -1, +0.5 -> 0, +1.7 -> 1
    float dxf = dx - dxi;  // -1.7 -> +0.3, -0.5 -> +0.5, +0.5 ->+0.5, +1.7 -> +0.7
    float dxr = 1 - dxf;

    // if fractional offset is small, do not interpolate
    int doInterp = fabs(dxf) < 1e-5 ? FALSE : TRUE;
      
    // How do I handle this?  define a temporary window which is a fraction of the full output window?
    // if the output buffer is not the same width as the input buffer, use this adjustment:
    // Nx2 = int(Nx/2), NxBase2 = int(NxBase/2), where Nx is input width, NxBase is output width
    // int Sx = Nx2 - NxBase2 + dxi;
 
    for (int ix = 0; ix < Nx; ix++) {

      // equivalent coord in temporary buffer (Nx * Ny):
      int pix = ix + dxi;
      if (pix < 0) {
	output[ix + iy*Nx] = input[iy*Nx];
	continue;
      }
      if (pix >= Nx) {
	output[ix + iy*Nx] = input[(Nx - 1) + iy*Nx];
	continue;
      }
	
      // a default value:
      float vout = NAN;
	
      int Npix = pix + iy*Nx;

      if (doInterp) {
	if ((pix > 0) && (pix < Nx - 1)) {
	  vout = input[Npix]*dxr + input[Npix + 1]*dxf;
	}
	if (pix == 0) {
	  vout = input[Npix];
	}
	if (pix == Nx - 1) {
	  vout = input[Npix];
	}
      } else {
	vout = input[Npix];
      }
      output[ix + iy*Nx] = vout;
    }
  }
  return output;
}

