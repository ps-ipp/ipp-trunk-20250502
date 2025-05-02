# include "astro.h"

float Nsigma = 4.0;
float sigma = 4.0 / 2.35; // FWHM = 4 pixels, 1 arcsec on PS1 GPC1
float calc_forcedphot (Buffer *buf, float x, float y);
float norm = 2.0;

int forcedphot (int argc, char **argv) {

  int N = 0;
  Buffer *buf = NULL;
  Vector *output = NULL;
  Vector *xvec = NULL;
  Vector *yvec = NULL;

  int VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-nsigma"))) {
    remove_argument (N, &argc, argv);
    Nsigma  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if ((N = get_argument (argc, argv, "-sigma"))) {
    remove_argument (N, &argc, argv);
    sigma  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  int FLUXNORMAL = TRUE;
  if ((N = get_argument (argc, argv, "-unit-normal"))) {
    FLUXNORMAL = FALSE;
    remove_argument (N, &argc, argv);
  }

  // XXX this should be the output name of a variable for 
  char *outname = NULL;
  if ((N = get_argument (argc, argv, "-output"))) {
    remove_argument (N, &argc, argv);
    outname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else {
    outname = strcreate ("fflux");
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: psfqf (buffer) x y [-sigma sigma] [-nsigma nsigma] [-output outname]\n");
    gprint (GP_ERR, "  measures forced-photometry using defined PSF for supplied coordinate(s)\n");
    gprint (GP_ERR, "  supplied coordinates may be scalar or vector(s)\n");
    gprint (GP_ERR, "  output values are saved in $fflux or vector fflux, or supplied name\n");
    goto escape;
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto escape;

  if (FLUXNORMAL) {
    norm = 2.0;
  } else {
    norm = 1.0 / (2.0 * M_PI * SQ(sigma));
  }

  // Scalar coordinate input:
  double Xin = NAN;
  double Yin = NAN;
  if (SelectScalar (argv[2], &Xin)) {
    if (!SelectScalar (argv[3], &Yin)) {
      gprint (GP_ERR, "syntax error: mixed vector and scalars for (x y)\n");
      goto escape;
    }
    float flux = calc_forcedphot (buf, Xin, Yin);
    if (VERBOSE) gprint (GP_LOG, "forced flux at %10.6f %10.6f = %f\n", Xin, Yin, flux);
    set_variable (outname, flux);
    free (outname);
    return TRUE;
  }

  // Vector coordinate input:
  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;
  if (xvec[0].Nelements != yvec[0].Nelements) {
    fprintf (stderr, "mis-matched vector lengths\n");
    goto escape;
  }
  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 
  
  if ((output = SelectVector (outname, ANYBUFFER, TRUE)) == NULL) goto escape;
  
  if (output) ResetVector (output, OPIHI_FLT, xvec->Nelements);

  for (int i = 0; i < xvec->Nelements; i++) {
    Xin = xvec->elements.Flt[i];
    Yin = yvec->elements.Flt[i];
    float flux = calc_forcedphot (buf, Xin, Yin);
    output->elements.Flt[i] = flux;
  }
  if (VERBOSE) gprint (GP_LOG, "forced flux calculated for %d points (output is %s)\n", output->Nelements, output->name);
  
  free (outname);
  return (TRUE);

 escape:
  free (outname);
  return FALSE;
}

// XXX things to consider:
// if source is totally off image, flux should be nan, not 0.0
// if any pixel is nan, skip or nan the source?
float calc_forcedphot (Buffer *buf, float x, float y) {

  int Nx = buf->matrix.Naxis[0];
  int Ny = buf->matrix.Naxis[1];

  int Xmin = x - Nsigma*sigma;
  int Xmax = x + Nsigma*sigma;
  int Ymin = y - Nsigma*sigma;
  int Ymax = y + Nsigma*sigma;

  float *bvalue = (float *)buf->matrix.buffer;
  float f1 = -0.5 / SQ(sigma);

  float sum = 0.0;
  for (int iy = Ymin; iy <= Ymax; iy++) {
    if (iy < 0) continue;
    if (iy >= Ny) continue;
    for (int ix = Xmin; ix <= Xmax; ix++) {
      if (ix < 0) continue;
      if (ix >= Nx) continue;
      float r = SQ(ix - x) + SQ(iy - y);
      
      float gvalue = bvalue[ix + iy*Nx]*exp(f1*r);
      sum += gvalue;
    }
  }
  sum *= norm;
  return sum;
}
