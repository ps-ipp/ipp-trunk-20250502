# include "astro.h"
# include <signal.h>

int flux (int argc, char **argv) {
  
  int i, j, k, xmin, ymin, xmax, ymax;
  double ax, ay, s, S, flux;
  double bx[5], by[5], x[5], y[5], bb[5];
  float *V;
  FILE *f;
  Buffer *buf;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: flux <buffer> (region)\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "file %s not found\n", argv[2]);
    return (FALSE);
  }

  xmin = buf[0].matrix.Naxis[0];
  xmax = 0;
  ymin = buf[0].matrix.Naxis[1];
  ymax = 0;
  for (i = 0; i < 4; i++) {
    if (fscanf (f, "%lf %lf", &x[i], &y[i]) != 2) {
      fprintf (stderr, "error reading coordinates from %s\n", argv[2]);
      fclose (f);
      return (FALSE);
    }
    xmin = MAX (0, MIN (xmin, x[i] - 1));
    ymin = MAX (0, MIN (ymin, y[i] - 1));
    xmax = MIN (MAX (xmax, x[i] + 1), buf[0].matrix.Naxis[0]);
    ymax = MIN (MAX (ymax, y[i] + 1), buf[0].matrix.Naxis[1]);
  }
  fclose (f);

  x[4] = x[0]; y[4] = y[0];
  for (i = 0; i < 4; i++) {
    bx[i] = x[i+1] - x[i];
    by[i] = y[i+1] - y[i];
  }
  bx[4] = bx[0]; by[4] = by[0];
  for (i = 0; i < 4; i++) {
    bb[i] = hypot (bx[i], by[i]) * SIGN (bx[i]*by[i+1] - bx[i+1]*by[i]);
  }
  gprint (GP_ERR, "%f %f %f %f\n", bb[0], bb[1], bb[2], bb[3]);

  /* this only works for convex contours --
   we have to add up the angles for concave contours */
  flux = 0;

  struct sigaction *old_sigaction = SetInterrupt();
  for (j = ymin; (j < ymax) && !interrupt; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + xmin; 
    for (i = xmin; (i < xmax) && !interrupt; i++, V++) {
      S = 1.0;
      for (k = 0; k < 4; k++) {
	ax = i - x[k];
	ay = j - y[k];
	s = (ay*bx[k] - ax*by[k]) / bb[k] + 0.5;
	/* s = b x a / |b|, with the correct sign (above) so inside is positive */
	s = MAX (0.0, MIN (1.0, s));  /* s is between 0.0 and 1.0 */
	S *= s;
      }
      flux += S * (*V);
    }
  }
  ClearInterrupt (old_sigaction);

  gprint (GP_LOG, "flux: %f\n", flux);
  set_variable ("FLUX", flux);
  return (TRUE);
}

