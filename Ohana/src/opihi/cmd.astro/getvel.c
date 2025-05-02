# include "astro.h"

int getvel (int argc, char **argv) {
  
  int i, n, Ncurve;
  double L, V, Vo, dV, Bo, dB;
  double xo, yo;
  double sl, cl, wo, Ro, Rs, wr, r, fr, d, x;
  double R[100], T[100], W[100];
  FILE *f;
  Buffer *buf;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: rotcurve buf X Y curve.txt\n");
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

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  // int nx = buf[0].matrix.Naxis[0];
  // int ny = buf[0].matrix.Naxis[1];

  /* we expect the input image to have units of velocity, lattitude, and longitude */
  gfits_scan (&buf[0].header, "CRVAL1", "%lf", 1, &Vo);
  gfits_scan (&buf[0].header, "CDELT1", "%lf", 1, &dV);
  gfits_scan (&buf[0].header, "CRPIX1", "%lf", 1, &xo);
  gfits_scan (&buf[0].header, "CRVAL2", "%lf", 1, &Bo);
  gfits_scan (&buf[0].header, "CDELT2", "%lf", 1, &dB);
  gfits_scan (&buf[0].header, "CRPIX2", "%lf", 1, &yo);
  gfits_scan (&buf[0].header, "CRVAL3", "%lf", 1, &L);
  Vo *= 0.001;
  dV *= 0.001;

  L = ohana_normalize_angle (L);
  gprint (GP_ERR, "L: %f\n", L);

  cl = cos (L*RAD_DEG);
  sl = sin (L*RAD_DEG);
  wo = 25.0;
  Ro = 10.0;
  Rs = Ro*sl;
  x = atof (argv[2]);
  /* this method depends on wr monotonically decreasing */

  V = (x - xo) * dV + Vo;
  wr = V/Rs + wo;
  for (n = 0; (n < Ncurve) && (wr < W[n]); n++);
  if ((n == 0) || (n == Ncurve)) {
    gprint (GP_ERR, "velocity out of reasonable range\n");
    gprint (GP_ERR, "%f %f %f %f\n", V, wr, W[0], W[Ncurve-1]);
    return (TRUE);
  }
  r = (wr - W[n]) *  (R[n-1] - R[n]) / (W[n-1] - W[n]) + R[n];
  fr = (Ro/r);
  if (r < fabs(Rs)) { /* can't be on rotation curve */
    gprint (GP_ERR, "velocity out of reasonable range\n");
    gprint (GP_ERR, "%f %f %f %f %f %f %f\n", V, wr, W[0], W[Ncurve-1], r, fr, Rs);
    return (TRUE);
  }
  if (r < Ro)
    d = Ro*cl - sqrt(r*r - Rs*Rs);
  else 
    d = Ro*cl + sqrt(r*r - Rs*Rs);
  
  gprint (GP_ERR, "dist: %f, vel: %f\n", d, V);

  return (TRUE);

} 
