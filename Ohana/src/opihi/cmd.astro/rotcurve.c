# include "astro.h"

int rotcurve (int argc, char **argv) {
  
  int i, j, X, Y, n, Ncurve;
  float *Vin, *Vout, *Vmask;
  int nx, ny, Nx, Ny, N;
  double L, dL, Lo, V, Vo, dV, Bo, dB, Do, dD;
  double xo, yo, Xo, Yo;
  double sl, cl, wo, Ro, Rs, wr, r, d, min;
  double R[100], T[100], W[100];
  FILE *f;
  Buffer *in, *out, *mask;

  min = -1000;
  if ((N = get_argument (argc, argv, "-min"))) {
    remove_argument (N, &argc, argv);
    min = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: rotcurve in out mask curve.txt\n");
    return (FALSE);
  }

  f = fopen (argv[4], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't find rotation curve data file %s\n", argv[4]);
    return (FALSE);
  }
  for (i = 0; fscanf (f, "%lf %lf", &R[i], &T[i]) != EOF; i++) {
    W[i] = T[i] / R[i];
  }  
  fclose (f);
  Ncurve = i;

  if ((in   = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out  = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((mask = SelectBuffer (argv[3], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  Nx = out[0].matrix.Naxis[0];
  Ny = out[0].matrix.Naxis[1];
  nx = mask[0].matrix.Naxis[0];
  ny = mask[0].matrix.Naxis[1];
  if ((Nx != nx) && (Ny != ny)) {
    gprint (GP_ERR, "output and mask must have same dimensions\n");
    return (FALSE);
  }
  nx = in[0].matrix.Naxis[0];
  ny = in[0].matrix.Naxis[1];

  /* we expect the input image to have units of velocity, lattitude, and longitude */
  gfits_scan (&in[0].header, "CRVAL1", "%lf", 1, &Vo);
  gfits_scan (&in[0].header, "CDELT1", "%lf", 1, &dV);
  gfits_scan (&in[0].header, "CRPIX1", "%lf", 1, &xo);
  gfits_scan (&in[0].header, "CRVAL2", "%lf", 1, &Bo);
  gfits_scan (&in[0].header, "CDELT2", "%lf", 1, &dB);
  gfits_scan (&in[0].header, "CRPIX2", "%lf", 1, &yo);
  gfits_scan (&in[0].header, "CRVAL3", "%lf", 1, &L);
  Vo *= 0.001;
  dV *= 0.001;

  /* we expect the output image to have units of longitude and distance */
  gfits_scan (&out[0].header, "CRVAL1", "%lf", 1, &Lo);
  gfits_scan (&out[0].header, "CDELT1", "%lf", 1, &dL);
  gfits_scan (&out[0].header, "CRPIX1", "%lf", 1, &Xo);
  gfits_scan (&out[0].header, "CRVAL2", "%lf", 1, &Do);
  gfits_scan (&out[0].header, "CDELT2", "%lf", 1, &dD);
  gfits_scan (&out[0].header, "CRPIX2", "%lf", 1, &Yo);

  L = ohana_normalize_angle (L);

  X = (L - Lo) / dL + Xo;
  if ((X >= Nx) || (X < 0)) {
    gprint (GP_ERR, "X out of range\n");
    return (FALSE);
  }
  gprint (GP_ERR, "L: %f (%d)\n", L, X);

  cl = cos (L*RAD_DEG);
  sl = sin (L*RAD_DEG);
  wo = 25.0;
  Ro = 10.0;
  Rs = Ro*sl;
  /* this method depends on wr monotonically decreasing */

  Vin  = (float *)in[0].matrix.buffer;
  Vout = (float *)out[0].matrix.buffer;
  Vmask = (float *)mask[0].matrix.buffer;
  for (j = 0; j < ny; j++) {
    for (i = 0; i < nx; i++, Vin++) {
      if (*Vin <= min) continue;
      V = (i - xo) * dV + Vo;
      wr = V/Rs + wo;
      for (n = 0; (n < Ncurve) && (wr < W[n]); n++);
      if ((n == 0) || (n == Ncurve)) {
	continue;
      }
      r = (wr - W[n]) *  (R[n-1] - R[n]) / (W[n-1] - W[n]) + R[n];
      // fr = (Ro/r);
      if (r < fabs(Rs)) { /* can't be on rotation curve */
	continue;
      }
      if (r < Ro)
	d = Ro*cl - sqrt(r*r - Rs*Rs);
      else 
	d = Ro*cl + sqrt(r*r - Rs*Rs);
      Y = (d - Do) / dD + Yo;
      if ((Y < Ny) && (Y >= 0)) {
	Vout[Y*Nx + X] += *Vin;
	Vmask[Y*Nx + X] += 1.0;
      }
    }
  }


  return (TRUE);

} 
