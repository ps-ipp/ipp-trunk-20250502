# include "data.h"

// temporary storage used in local functions:

static float *tmpv = NULL;
static float *tmpx = NULL;

float get_median_flt (float *val, int Nval) {

  if (Nval <= 0) return NAN;

  int Nval2 = Nval / 2;

  fsort (val, Nval);
  float value = (Nval % 2) ? val[Nval2] : 0.5*(val[Nval2-1] + val[Nval2]);

  return value;
}

int deimos_getobj (int argc, char **argv) {

  // determine rough object model parameters based on quick stats
  // input : buffer PSF trace profile stilt
  // output : object sky background
  // not used : LSF -- not needed (measurement per disp pixel)

  // This version uses the median of the pixels in the image which are
  // in regions where the slit profile is < 15% (as opposed to the
  // getobj version which attempts to fit both background and sky regions

  // input parameters:
  // * buffer      : 2D image of slit (full frame or cutout?)
  // * slit_trace  : spline fit of slit central x pos vs y-coord
  // * psf_trace   : spline fit of psf central x pos vs y-coord
  // * profile     : slit window profile (vector)
  // * PSF         : point-spread function vector (flux normalized, x-dir)
  // * stilt       : slit tilt response : 2D kernel? 

  // output parameters:
  // * object      : vector of object flux vs y-coord 
  // * sky         : vector of local sky signal vs y-coord
  // * background  : vector of extra-slit background flux vs y-coord

  int N;

  Spline *slit_trace_red = NULL;
  Spline *slit_trace_blu = NULL;
  Spline *psf_trace  = NULL;
  Vector *profile = NULL;

  Vector *object  = NULL;
  Vector *sky     = NULL;
  Vector *backgnd = NULL;

  Vector *psf     = NULL;

  Buffer *buffer  = NULL;

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

  // Input parameters:
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

  // Output parameters:
  if ((N = get_argument (argc, argv, "-object"))) {
    remove_argument (N, &argc, argv);
    if ((object = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    if ((sky = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-backgnd"))) {
    remove_argument (N, &argc, argv);
    if ((backgnd = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }

  if (argc != 3) goto usage;

  // buffer is the full image, Xref is reference coordinate of the profile in the buffer
  if ((buffer = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  float Xref = atof(argv[2]);
  
  // profile and PSF are defined with central reference pixel mapped to Xref (+ trace)
  int Nprofile = profile->Nelements;
  if (Nprofile % 2 == 0) {
    gprint (GP_ERR, "slit profile vector must have an odd number of pixels\n");
    return FALSE;
  }
  int Xprofile = Nprofile / 2;

  int Npsf = psf->Nelements;
  if (Npsf % 2 == 0) {
    gprint (GP_ERR, "PSF vector must have an odd number of pixels\n");
    return FALSE;
  }
  int Xpsf = Npsf / 2;

  // we will generate output vectors (object, sky, background) of length Ny
  int Nx = buffer[0].matrix.Naxis[0];
  int Ny = buffer[0].matrix.Naxis[1];
  ResetVector (object,  OPIHI_FLT, Ny);
  ResetVector (sky,     OPIHI_FLT, Ny);
  ResetVector (backgnd, OPIHI_FLT, Ny);
  
  // generate two buffers to store the flux and coordinate for the background, sky, object pixels:
  ALLOCATE (tmpv, float, Nx);
  ALLOCATE (tmpx, float, Nx);

  float *buffV = (float *) buffer[0].matrix.buffer;

  float dxMax = 0; // maximum distance between sky regions

  // extract sky-subtracted values for the psf fit, using the two sky vectors to predict the sky
  float sin_stilt = sin(stilt*RAD_DEG);
  float dyMax = dxMax*sin_stilt;  // dxMax is based on the profile width, calculated above

  // we are fitting this PSF to the data:
  opihi_flt *psfV = psf->elements.Flt;

  // init the regions which are outside the valid region
  for (int iy = 0; iy < fabs(dyMax); iy++) {
    object->elements.Flt[iy] = 0;
    sky->elements.Flt[iy] = 0;
    backgnd->elements.Flt[iy] = 0;
  }    
  for (int iy = Ny - fabs(dyMax); iy < Ny; iy++) {
    object->elements.Flt[iy] = 0;
    sky->elements.Flt[iy] = 0;
    backgnd->elements.Flt[iy] = 0;
  }    

  for (int iy = fabs(dyMax); iy < Ny - fabs(dyMax); iy++) {

    // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
    Spline *slit_trace = (iy < redlimit) ? slit_trace_red : slit_trace_blu;
    float dXtraceSLIT = spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy);
    float dXtracePSF  = spline_apply_dbl (psf_trace->xk, psf_trace->yk, psf_trace->y2, psf_trace->Nknots, iy);
      
    // Xref is the reference coordinate of the profile on the image
    // Xpsf is the center of the PSF.  
    // dXtrace follows the x-dir variations of the slit profile (or PSF?)

    // XtracePSF is the offset between the buffer ix and the psf elements at this location
    float XtracePSF = Xref - Xpsf + dXtraceSLIT + dXtracePSF;

    // XtraceSLIT is the offset between the buffer ix and the slit profile elements at this location
    float XtraceSLIT = Xref - Xprofile + dXtraceSLIT;

    // measure the background outside the slit

    // select pixels from the image which are in the background regions (profile < 0.15)
    int Nbck = 0;
    int Nsky = 0;
    for (int ix = 0; ix < Nx; ix ++) {
      int islit = ix - XtraceSLIT - 0.5; // slit window coordinate
      int ipsf = ix - XtracePSF - 0.5;   // psf model coordinate

      if (islit < 0) continue;
      if (islit >= Nprofile) continue;

      // coordinate of the PSF center:
      int dxpsf = ipsf - Xpsf;
      int dypsf = dxpsf * sin_stilt;

      int iY = iy + dypsf;

      if (iy == 6000) {
	fprintf (stderr, "bck : %d %d %d : %f : %f\n", ix, islit, ipsf, buffV[ix + iY*Nx], profile->elements.Flt[islit]);
      }

      // pixels in the off-slit background:
      if (profile->elements.Flt[islit] < 0.15) {
	tmpv[Nbck] = buffV[ix + iY*Nx];
	if (!isfinite(tmpv[Nbck])) continue;
	Nbck ++;
	continue;
      }

      // the psf window can be smaller than the slit window
      if (ipsf < 0) continue;
      if (ipsf >= Npsf) continue;

      // pixels in the on-slit sky:
      if ((profile->elements.Flt[islit] > 0.95) && (psfV[ipsf] < 0.15)) {
	tmpx[Nsky] = buffV[ix + iY*Nx];
	if (!isfinite(tmpx[Nsky])) continue;
	Nsky ++;
      }
    }
    backgnd->elements.Flt[iy] = get_median_flt (tmpv, Nbck);
    float So = get_median_flt (tmpx, Nsky);
    sky->elements.Flt[iy] = So;

    // now I need to find the limits of the PSF in the buffer at this iy location
    int Ntmp = 0;
    for (int ix = 0; ix < Nx; ix ++) {
      int islit = ix - XtraceSLIT - 0.5; // slit window coordinate
      int ipsf = ix - XtracePSF - 0.5;   // psf model coordinate

      if (islit < 0) continue;
      if (islit >= Nprofile) continue;

      // the psf window can be smaller than the slit window
      if (ipsf < 0) continue;
      if (ipsf >= Npsf) continue;

      // coordinate of the PSF center:
      int dxpsf = ipsf - Xpsf;
      int dypsf = dxpsf * sin_stilt;

      int iY = iy + dypsf;

      // filter on the PSF value?
      if (profile->elements.Flt[islit] < 0.95) continue;

      if (iy == 6000) {
	fprintf (stderr, "%d %d %d : %f %f : %f : %f\n", ix, islit, ipsf, buffV[ix + iY*Nx] - So, psfV[ipsf], (buffV[ix + iY*Nx] - So)*psfV[ipsf], profile->elements.Flt[islit]);
      }

      tmpv[Ntmp] = buffV[ix + iY*Nx] - So;
      tmpx[Ntmp] = psfV[ipsf];

      if (!isfinite(tmpv[Ntmp])) continue;
      if (!isfinite(tmpx[Ntmp])) continue;

      Ntmp ++;
    }

    // I now have a pair of vectors (tmpx, tmpv) which are aligned to the PSF at this location
    // now fit the PSF to tmpv by calculating a normalization
    float S1 = 0.0, S2 = 0.0;
    for (int ipsf = 0; ipsf < Ntmp; ipsf++) {
      if (iy == 6000) {
	fprintf (stderr, "%d : %f %f : %f\n", ipsf, tmpv[ipsf], tmpx[ipsf], tmpv[ipsf]*tmpx[ipsf]);
      }
      S1 += tmpv[ipsf]*tmpx[ipsf]; // if we have an errorbar, include / SQ(sigv[i])
      S2 += tmpx[ipsf]*tmpx[ipsf]; // if we have an errorbar, include / SQ(sigv[i])
    }
    object->elements.Flt[iy] = S1 / S2;
  }

  free (tmpv);
  free (tmpx);

  return TRUE;

 usage:
  gprint (GP_ERR, "USAGE: deimos getobj (buffer) (Xref) -object vector -sky vector -backgnd vector -trace spline -profile vector -psf vector -stilt (angle)\n");
  // can I make default values for trace (0.0), profile (?), psf (fraction of profile?), stilt (0.0)
  return FALSE;
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
  
