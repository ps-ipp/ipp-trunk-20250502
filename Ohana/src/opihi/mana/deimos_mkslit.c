# include "data.h"

int deimos_mkslit (int argc, char **argv) {

  // generate model observed flux given a uniformly illuminated slit

  // input parameters:
  //   slit profile response : vector of fractional flux vs x-coord
  //   trace_ref,trace_blu   : spline fit of slit central x pos vs y-coord
  //   flux (optional)       : vector of input signal vs y-coord
  //   sky  (optional)       : vector of input signal vs y-coord
  //   fluxbins (optional)   : size of output image in y-direction
  //   NOTE: one of flux, sky, or fluxbins must be defined

  // output : buff (an image with Nx defined by profile and Ny defined
  //                by flux, sky, or fluxbins)

  int N;

  Vector *profile   = NULL;
  Vector *flux      = NULL;
  Vector *sky       = NULL;
  Spline *trace_red = NULL;
  Spline *trace_blu = NULL;
  Buffer *buff      = NULL;

  // user-specified flux value (otherwise assumed to be 1)
  if ((N = get_argument (argc, argv, "-flux"))) {
    remove_argument (N, &argc, argv);
    if ((flux = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  // user-specified sky value (otherwise assumed to be 0)
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    if ((sky = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  // either flux or fluxbins must be specified to define output size
  int fluxbins = 0;
  if ((N = get_argument (argc, argv, "-fluxbins"))) {
    remove_argument (N, &argc, argv);
    fluxbins = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // either flux or fluxbins must be specified to define output size
  int profilebins = 0;
  if ((N = get_argument (argc, argv, "-profilebins"))) {
    remove_argument (N, &argc, argv);
    profilebins = atoi (argv[N]);
    remove_argument (N, &argc, argv);
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

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: deimos mkslit (profile) (trace_red) (trace_blu) (buffer) [-flux vector] [-fluxbins N] [-sky vector]\n");
    return FALSE;
  }

  if (!flux && !fluxbins) {
    gprint (GP_ERR, "either flux or fluxbins must be specified to define output size\n");
    return FALSE;
  }

  // XXX I probably should rename FindSpline as SelectSpline and give it the same behavior
  if ((profile   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((trace_red = FindSpline (argv[2])) == NULL) return (FALSE);
  if ((trace_blu = FindSpline (argv[3])) == NULL) return (FALSE);
  if ((buff      = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  // define the output window
  // if no profile half-width is specified, use supplied profile 
  int Nx = profilebins ? 2*profilebins + 1 : 2*profile[0].Nelements + 1;
  int Ny = flux ? flux[0].Nelements : fluxbins;
  
  ResetBuffer (buff, Nx, Ny, -32, 0.0, 1.0);

  // the profile is registered to the midpoint of the vector : (int) N / 2
  // the output window is registered to the midpoint of the window : (int) Nx / 2

  int NxMidProfile = profile->Nelements / 2;
  int NxMidWindow  = Nx / 2;

  float *out = (float *) buff[0].matrix.buffer;

  // loop over the y positions
  for (int iy = 0; iy < Ny; iy++) {

    // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
    Spline *trace = (iy < redlimit) ? trace_red : trace_blu;
    float dx = spline_apply_dbl (trace->xk, trace->yk, trace->y2, trace->Nknots, iy);
    
    // extract the integer pixel offset and the fractional offset
    int dxi = floor(dx); // -1.7 -> -2, -0.5 -> -1, +0.5 -> 0, +1.7 -> 1
    float dxf = dx - dxi;  // -1.7 -> +0.3, -0.5 -> +0.5, +0.5 ->+0.5, +1.7 -> +0.7

    // starting point in output window is :
    // XXX handle case if profile is wider than window
    int Sx = (int)(NxMidWindow - NxMidProfile) + dxi;

    // if fractional offset is small, do not interpolate
    int doInterp = fabs(dxf) < 1e-5 ? FALSE : TRUE;

    // loop over the relevant pixels in the window:
    for (int ix = 0; ix < Nx; ix++) {

      // set the sky in the entire window 
      out[ix + iy*Nx] = NAN;

      // equivalent coord in the profile:
      int px = ix - Sx;
      if (px < 0) continue;
      if (px >= profile->Nelements) continue;

      // a default value:
      float vsky = sky ? sky->elements.Flt[iy] : 0.0;
      float vout = NAN;

      if (doInterp) {
	if ((px > 0) && (px < profile->Nelements - 1)) {
	  // if ((px == 5) && (iy % 50 == 0)) {
	  //   fprintf (stderr, "%d : %d %d : %d, %f\n", iy, Sx, px, dxi, dxf);
	  // }
	  // XXX something is not quite right in my math: I think (1-dx), dx are reversed
	  // but if I use what I expect the interpolation is backwards...
	  vout = profile->elements.Flt[px]*(1 - dxf) + profile->elements.Flt[px-1]*dxf;
	}
	if (px == 0) {
	  vout = profile->elements.Flt[px]*dxf;
	}
	if (px == profile->Nelements - 1) {
	  vout = profile->elements.Flt[profile->Nelements - 1]*(1.0 - dxf);
	}
      } else {
	vout = profile->elements.Flt[px];
      }

      if (flux) vout *= flux->elements.Flt[iy];

      out[ix + iy*Nx] = vsky + vout;
    }
  }

  return TRUE;
}

