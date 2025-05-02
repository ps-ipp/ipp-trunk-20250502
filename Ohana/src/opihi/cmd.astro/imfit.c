# include "imfit.h"

int imfit (int argc, char **argv) {

  int N;
  Buffer *buf;

  char *Save = NULL;
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    Save = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int Insert = FALSE;
  if ((N = get_argument (argc, argv, "-insert"))) {
    remove_argument (N, &argc, argv);
    Insert = TRUE;
    if (Save) { gprint (GP_ERR, "-save and -insert are mutually exclusive\n"); free (Save); return (FALSE); }
  }

  int minIter = 5;
  if ((N = get_argument (argc, argv, "-min-iter"))) {
    remove_argument (N, &argc, argv);
    minIter = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int SatThreshold = 0xffff;
  if ((N = get_argument (argc, argv, "-sat"))) {
    remove_argument (N, &argc, argv);
    SatThreshold = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* Gain in e/DN */
  float Gain = 1.0;
  if ((N = get_argument (argc, argv, "-gain"))) {
    remove_argument (N, &argc, argv);
    Gain = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* RD noise in DN */
  float RDnoise = 0.0;
  if ((N = get_argument (argc, argv, "-rdnoise"))) {
    remove_argument (N, &argc, argv);
    RDnoise = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  /* set fitting function : defines par, Npar, fitfunc, etc globals (imfit.h) */
  fgauss_setup ("fgauss");
  if ((N = get_argument (argc, argv, "-func"))) {
    fitfunc = NULL;
    remove_argument (N, &argc, argv);
    fgauss_setup (argv[N]); // OK
    pgauss_setup (argv[N]); // OK 
    sgauss_setup (argv[N]);
    qgauss_setup (argv[N]); // OK
    rgauss_setup (argv[N]); 
    qfgauss_setup (argv[N]); // OK
    qrgauss_setup (argv[N]);
    trail_setup (argv[N]);
    pgauss_psf_setup (argv[N]);
    sgauss_psf_setup (argv[N]);
    qgauss_psf_setup (argv[N]);

    fgauss_pol_setup (argv[N]);
    rgauss_pol_setup (argv[N]); 

    if (fitfunc == NULL) {
      gprint (GP_ERR, "unknown function %s\n", argv[N]);
      FREE (Save);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: imfit <buffer> Xo Yo dX dY\n");
    gprint (GP_ERR, "options: [-save buffer] [-insert] [-sat value] [-gain value] [-rdnoise value] [-v] [-func option]\n");
    gprint (GP_ERR, "   (Xo,Yo) : center\n");
    gprint (GP_ERR, "   (dX,dY) : window size\n");
    FREE (Save);
    return (FALSE);
  }

  /* non-optional arguments */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) { FREE (Save); return (FALSE); }
  int Xo = atof (argv[2]);
  int Yo = atof (argv[3]);
  int dX = atof (argv[4]);
  int dY = atof (argv[5]);
  int Nx = buf[0].matrix.Naxis[0];
  int Ny = buf[0].matrix.Naxis[1];

  int sx = Xo - dX/2;
  int sy = Yo - dY/2;

  /* check if region is valid (center must be in range of image pixels) */
  if (Xo < 0) goto range;
  if (Yo < 0) goto range;
  if (Xo >= Nx) goto range;
  if (Yo >= Ny) goto range;

  // image value in DN
  // rdnoise in DN
  // sigma_DN^2 = sigma_e^2 / gain^2
  // sigma_e^2  = Ne
  // sigma_e^2  = DN * gain
  // sigma_DN^2 = DN * gain / gain^2 = DN / gain

  if (Insert) {
    float *Vi = (float *)buf[0].matrix.buffer;
    for (int j = 0; j < dY; j++) {
      for (int i = 0; i < dX; i++) {
	float vf = fitfunc ((float)(i+sx), (float)(j+sy), par, Npar, NULL);
	Vi[(i+sx)+(j+sy)*Nx] += vf;
      }
    }
    return TRUE;
  }

  /* convert array z[x,y] to x[i], y[i], z[i] */
  N = 0;
  int Npts = dX*dY;
  ALLOCATE_PTR (x,  opihi_flt, 2*Npts);
  ALLOCATE_PTR (y,  opihi_flt, 2*Npts);
  ALLOCATE_PTR (z,  opihi_flt, 2*Npts);
  ALLOCATE_PTR (dz, opihi_flt, 2*Npts);
  for (int j = 0; j < dY; j++) {
    if (j + sy < 0) continue;
    if (j + sy >= Ny) continue;
    float *V = (float *)(buf[0].matrix.buffer) + (j+sy)*buf[0].matrix.Naxis[0] + sx; 
    for (int i = 0; i < dX; i++) {
      if (i + sx < 0) continue;
      if (i + sx >= Nx) continue;
      if (*V > SatThreshold) goto next; // skip pixels above threshold
      if (!isfinite(*V)) goto next; // skip nan pixels
      dz[N] = (SQ(RDnoise) + MAX(0.0, *V/Gain)); // treat negative pixels as pure read noise
      if (dz[N] <= 0) goto next;
      dz[N] = 1.0 / dz[N];
      x[N] = i + sx;
      y[N] = j + sy;
      z[N] = *V;
      N++;
    next:
      V++;
    }
  }
  Npts = N;

  /* run fit routine */
  float ochisq = mrq2dinit (x, y, z, dz, Npts, par, Npar, fitfunc, VERBOSE);
  float dchisq = ochisq;
  float chisq  = ochisq;

  int Niter = 0;
  // for (int i = 0; (i < 25) && ((dchisq <= 0.0) || (dchisq > 0.01*(Npts - Npar))); i++) {

  // keep iterating if chisq is increasing or 
  for (Niter = 0; (Niter < 25) && ((Niter < minIter) || ((dchisq <= 0.0) || (dchisq > 0.1*(Npts - Npar)))); Niter++) {
    chisq = mrq2dmin (x, y, z, dz, Npts, par, Npar, fitfunc, VERBOSE);
    dchisq = ochisq - chisq;
    // fprintf (stderr, "%f -> %f : %f\n", ochisq, chisq, dchisq);
    ochisq = chisq;
  }  
  set_int_variable ("Niter",  Niter);

  /** create output image (keep in sky) **/
  if (Save) {
    Buffer *out;
    float *Vi, *Vo, vr, vf;

    if ((out = SelectBuffer (Save, ANYBUFFER, TRUE)) == NULL) { free (Save); return (FALSE); }
    free (out[0].header.buffer);
    free (out[0].matrix.buffer);

    strcpy (out[0].file, "(empty)");
    if (!CreateBuffer (out, 2*dX, 2*dY, -32, 0.0, 1.0)) { free (Save); return FALSE; }

    /* four panels: 1) raw image. 2) fit  3) raw - fit  4) absolute deviation */
    Vi = (float *)buf[0].matrix.buffer;
    Vo = (float *)out[0].matrix.buffer;
    for (int j = 0; j < dY; j++) {
      for (int i = 0; i < dX; i++) {
	vf = fitfunc ((float)(i+sx), (float)(j+sy), par, Npar, NULL);
	vr = Vi[(i+sx)+(j+sy)*Nx];
	Vo[(i   )+(j   )*2*dX] = vr;
	Vo[(i+dX)+(j   )*2*dX] = vf;
	Vo[(i   )+(j+dY)*2*dX] = vr - vf + *sky;
	Vo[(i+dX)+(j+dY)*2*dX] = fabs(vr-vf) + *sky;
      }
    }
    free (Save);
  }

  /* save parameters to opihi variables */
  imfit_cleanup ();

  set_variable ("ChiSq", chisq/(Npts - Npar));

  if (VERBOSE) {
    for (int i = 0; i < Npar; i++) {
      gprint (GP_ERR, "%g ", par[i]);
    }
    gprint (GP_ERR, "\n");
  }

  free (x);
  free (y);
  free (z);
  free (dz);
  free (par);
  free (fpar);

  mrq2dfree (Npar);
  return (TRUE);

range:
  gprint (GP_ERR, "region out of range\n");
  return (FALSE);
}

