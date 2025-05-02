# include "data.h"

// temporary storage used in local functions:

static float *tmpv = NULL;
static float *tmpx = NULL;

// pass in a pointer to the row (&buffV[iy*Nx])
float get_segment_median (float *buffV, float Xs, float Xe, float *Xref) {

  // get the pixels in the left portion below profile:
  // XXX: worry about the effect of a fractional offset
  for (int ix = Xs + 0.5; ix < Xe + 0.5; ix++) {
    int i = ix - Xs;
    tmpv[i] = buffV[ix];
    tmpx[i] = ix;
  }
  int Nv = Xe - Xs;

  // generate a statistic from these pixels: mean, median, etc?
  fsort (tmpv, Nv);
  int Nv2 = Nv/2; // Nv == 1, Nv2 = 0, Nv == 2, Nv2 = 1, Nv == 3, Nv2 = 1, Nv == 4, Nv == 2
  float V = (Nv % 2) ? tmpv[Nv2] : 0.5*(tmpv[Nv2-1] + tmpv[Nv2]);
    
  // calculate average coordinate (not flux-weighted...)
  // XXX : if we allow (and exclude) NAN-valued pixels, we can calculate this more explicitly
  *Xref = 0.5*(Xe + Xs);

  return V;
}

int deimos_getobj (int argc, char **argv) {

  // determine rough object model parameters based on quick stats
  // input : buffer PSF trace profile stilt
  // output : object sky background
  // not used : LSF -- not needed (measurement per disp pixel)

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

  Spline *slit_trace = NULL;
  Spline *psf_trace  = NULL;
  Vector *profile = NULL;

  Vector *object  = NULL;
  Vector *sky     = NULL;
  Vector *backgnd = NULL;

  Vector *psf     = NULL;

  Buffer *buffer  = NULL;

  float stilt = 0.0; // angle of the slit

  int SaveSegments = FALSE;
  if ((N = get_argument (argc, argv, "-save-segments"))) {
    remove_argument (N, &argc, argv);
    SaveSegments = TRUE;
  } 

  // Input parameters:
  if ((N = get_argument (argc, argv, "-slit-trace"))) {
    remove_argument (N, &argc, argv);
    if ((slit_trace = FindSpline (argv[N])) == NULL) return (FALSE);
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
  
  // I need to generate a sky-left and sky-right vector in a region on the slit portion of
  // the profile, but off the object.  Then each of these vectors can be shifted up and down
  // based on stilt.  Use these sky vectors to predict the sky under the PSF

  // *** scan the profile to determine the background and sky cut points ***

  // values below in the profile coordinate system need to be shifted by (Xref - Nprofile/2)
  // to match the image coordinate system

  int B_s_l = 0, B_e_l = -1; // left background window
  int B_s_r = -1, B_e_r = Nprofile - 1; // right background window
  int S_left = -1, S_right = -1; // top portion of profile
  for (int i = 0; i < Nprofile; i++) {
    if ((B_e_l   <  0) && (profile->elements.Flt[i] > 0.10)) B_e_l = i - 1;
    if ((B_e_l   >= 0) && (S_left  < 0) && (profile->elements.Flt[i] > 0.95)) S_left = i;
    if ((S_left  >= 0) && (S_right < 0) && (profile->elements.Flt[i] < 0.95)) S_right = i - 1;
    if ((S_right >= 0) && (profile->elements.Flt[i] < 0.10)) { B_s_r = i; break; }
  }
  if (B_e_l == B_s_l) B_e_l ++;
  if (B_e_r == B_s_r) B_s_r --;

  // S_left to S_right defines are window at the top, choose 0.05 - 0.15, 0.85 - 0.95
  int N_top = S_right - S_left + 1;

  // if N_top < 20, these jumps might be too small
  // if N_top < 20, these jumps might be too small
  int S_s_l = S_left + 0.05*N_top;
  int S_e_l = S_left + 0.15*N_top;
  if (S_e_l == S_s_l) S_e_l ++;
  int S_s_r = S_left + 0.85*N_top;
  int S_e_r = S_left + 0.95*N_top;
  if (S_e_r == S_s_r) S_s_r --;

  gprint (GP_ERR, "windows: %.1f - %.1f, %.1f - %.1f, %.1f - %.1f, %.1f - %.1f\n",
	  B_s_l + Xref - Xprofile, B_e_l + Xref - Xprofile,
	  S_s_l + Xref - Xprofile, S_e_l + Xref - Xprofile,
	  S_s_r + Xref - Xprofile, S_e_r + Xref - Xprofile,
	  B_s_r + Xref - Xprofile, B_e_r + Xref - Xprofile);

  // OPEN QUESTIONS:
  // NOTE: if the range of pixels in the windows is too large, then the tilt will wash out the signal.
  // this pass is a guess, so perhaps that does not matter, but there is clearly a trade-off here.

  // generate two buffers to store the flux and coordinate for the background, sky, object pixels:
  ALLOCATE (tmpv, float, Nx);
  ALLOCATE (tmpx, float, Nx);

  float *buffV = (float *) buffer[0].matrix.buffer;

  // use these to store the flux vectors
  ALLOCATE_PTR (bckF_lf, float, Ny);
  ALLOCATE_PTR (bckF_rt, float, Ny);
  ALLOCATE_PTR (skyF_lf, float, Ny);
  ALLOCATE_PTR (skyF_rt, float, Ny);

  // use these to store the x-coord vectors
  ALLOCATE_PTR (bckX_lf, float, Ny);
  ALLOCATE_PTR (bckX_rt, float, Ny);
  ALLOCATE_PTR (skyX_lf, float, Ny);
  ALLOCATE_PTR (skyX_rt, float, Ny);

  float dxMax = 0; // maximum distance between sky regions

  Vector *skyLsV = NULL, *skyLeV = NULL, *skyLfV = NULL, *skyRsV = NULL, *skyReV = NULL, *skyRfV = NULL;
  Vector *bckLsV = NULL, *bckLeV = NULL, *bckLfV = NULL, *bckRsV = NULL, *bckReV = NULL, *bckRfV = NULL;
  if (SaveSegments) {
    skyLsV = SelectVector ("skyLs", ANYVECTOR, TRUE);  ResetVector (skyLsV, OPIHI_FLT, Ny);
    skyLeV = SelectVector ("skyLe", ANYVECTOR, TRUE);  ResetVector (skyLeV, OPIHI_FLT, Ny);
    skyLfV = SelectVector ("skyLf", ANYVECTOR, TRUE);  ResetVector (skyLfV, OPIHI_FLT, Ny);
    skyRsV = SelectVector ("skyRs", ANYVECTOR, TRUE);  ResetVector (skyRsV, OPIHI_FLT, Ny);
    skyReV = SelectVector ("skyRe", ANYVECTOR, TRUE);  ResetVector (skyReV, OPIHI_FLT, Ny);
    skyRfV = SelectVector ("skyRf", ANYVECTOR, TRUE);  ResetVector (skyRfV, OPIHI_FLT, Ny);
    bckLsV = SelectVector ("bckLs", ANYVECTOR, TRUE);  ResetVector (bckLsV, OPIHI_FLT, Ny);
    bckLeV = SelectVector ("bckLe", ANYVECTOR, TRUE);  ResetVector (bckLeV, OPIHI_FLT, Ny);
    bckLfV = SelectVector ("bckLf", ANYVECTOR, TRUE);  ResetVector (bckLfV, OPIHI_FLT, Ny);
    bckRsV = SelectVector ("bckRs", ANYVECTOR, TRUE);  ResetVector (bckRsV, OPIHI_FLT, Ny);
    bckReV = SelectVector ("bckRe", ANYVECTOR, TRUE);  ResetVector (bckReV, OPIHI_FLT, Ny);
    bckRfV = SelectVector ("bckRf", ANYVECTOR, TRUE);  ResetVector (bckRfV, OPIHI_FLT, Ny);
  }

  // loop over the y positions
  // examine the slit window profile to measure background and slit sky
  for (int iy = 0; iy < Ny; iy++) {

    // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
    float dXtrace = spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy);

    // Xref is the reference coordinate of the profile on the image
    // Xprofile is the center of the profile.  
    // dXtrace follows the x-dir variations of the slit profile
    float Xtrace = Xref - Xprofile + dXtrace;

    bckF_lf[iy] = get_segment_median(&buffV[iy*Nx], B_s_l + Xtrace, B_e_l + Xtrace, &bckX_lf[iy]);
    bckF_rt[iy] = get_segment_median(&buffV[iy*Nx], B_s_r + Xtrace, B_e_r + Xtrace, &bckX_rt[iy]);
    skyF_lf[iy] = get_segment_median(&buffV[iy*Nx], S_s_l + Xtrace, S_e_l + Xtrace, &skyX_lf[iy]);
    skyF_rt[iy] = get_segment_median(&buffV[iy*Nx], S_s_r + Xtrace, S_e_r + Xtrace, &skyX_rt[iy]);
    dxMax = MAX (dxMax, skyX_rt[iy] - skyX_lf[iy]); // this is a constant value unless we ignore some of the pixels
    // assert skyX_rt > skyX_lf ?

    if (SaveSegments) {
      skyLsV->elements.Flt[iy] = S_s_l + Xtrace;
      skyLeV->elements.Flt[iy] = S_e_l + Xtrace;
      skyLfV->elements.Flt[iy] = skyF_lf[iy];
      skyRsV->elements.Flt[iy] = S_s_r + Xtrace;
      skyReV->elements.Flt[iy] = S_e_r + Xtrace;
      skyRfV->elements.Flt[iy] = skyF_rt[iy];
      bckLsV->elements.Flt[iy] = B_s_l + Xtrace;
      bckLeV->elements.Flt[iy] = B_e_l + Xtrace;
      bckLfV->elements.Flt[iy] = bckF_lf[iy];
      bckRsV->elements.Flt[iy] = B_s_r + Xtrace;
      bckReV->elements.Flt[iy] = B_e_r + Xtrace;
      bckRfV->elements.Flt[iy] = bckF_rt[iy];
    }
  }

  // extract sky-subtracted values for the psf fit, using the two sky vectors to predict the sky
  float dyMax = dxMax*sin(stilt*RAD_DEG);  // dxMax is based on the profile width, calculated above

  // find nominal dy_lf, dy_rt offsets for the sky vectors.  
  int dy_lf = -dyMax / 2; 
  int dy_rt = +dyMax / 2;

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


  // Ys, Ye based on the dyMax value above (skip the first few lines to avoid running out of bounds on sky)
  for (int iy = fabs(dyMax); iy < Ny - fabs(dyMax); iy++) {

    // X_lf & X_rt are calculated in image x-coords
    float S_lf = skyF_lf[iy + dy_lf];
    float S_rt = skyF_rt[iy + dy_rt];
    float X_lf = skyX_lf[iy + dy_lf];
    float X_rt = skyX_rt[iy + dy_rt];
    
    // evaluate the trace spline at this y-coord to find the x-coord offset of the profile center
    float dXtraceSLIT = spline_apply_dbl (slit_trace->xk, slit_trace->yk, slit_trace->y2, slit_trace->Nknots, iy);
    float dXtracePSF  = spline_apply_dbl (psf_trace->xk, psf_trace->yk, psf_trace->y2, psf_trace->Nknots, iy);
      
    // Xref is the reference coordinate of the profile on the image
    // Xpsf is the center of the PSF.  
    // dXtrace follows the x-dir variations of the slit profile (or PSF?)
    float XtracePSF = Xref - Xpsf + dXtraceSLIT + dXtracePSF;
    float XtraceSLIT = Xref - Xprofile + dXtraceSLIT;

    // include the PSF trace and restrict the fit below to portions where the profile is > 0.95

    // now I need to find the limits of the PSF in the buffer at this iy location
    int Ntmp = 0;
    for (int ipsf = 0; ipsf < Npsf; ipsf ++) {

      int ix = ipsf + XtracePSF + 0.5; // image coordinate (nearest pixel center)
      int islit = ix - XtraceSLIT - 0.5; // image coordinate (nearest pixel center)

      float So = S_lf + (ix - X_lf)*(S_rt - S_lf)/(X_rt - X_lf); // interpolated sky under PSF peak

      if (islit < 0) continue;
      if (islit >= profile->Nelements) continue;

      if (iy == 6000) {
	fprintf (stderr, "%d : %f %f : %f : %f\n", ipsf, buffV[ix + iy*Nx] - So, psfV[ipsf], (buffV[ix + iy*Nx] - So)*psfV[ipsf], profile->elements.Flt[islit]);
      }

      if (profile->elements.Flt[islit] < 0.95) continue;

      tmpv[Ntmp] = buffV[ix + iy*Nx] - So;
      tmpx[Ntmp] = psfV[ipsf];
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

    int Xo = Xref - dXtraceSLIT - dXtracePSF; // center of PSF in image x-coordinates
    float So = S_lf + (Xo - X_lf)*(S_rt - S_lf)/(X_rt - X_lf); // interpolated sky under PSF peak
    object->elements.Flt[iy] = S1 / S2;
    sky->elements.Flt[iy] = So;
    backgnd->elements.Flt[iy] = 0.5*(bckF_lf[iy] + bckF_rt[iy]);
  }

  free (tmpv);
  free (tmpx);

  free (bckF_lf);
  free (bckF_rt);
  free (skyF_lf);
  free (skyF_rt);

  free (bckX_lf);
  free (bckX_rt);
  free (skyX_lf);
  free (skyX_rt);

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
  
