# include "imclean.h"

int find_group (SMPData *stars, char *mark, int Npts, int i, double *ANGLE) {
 
  int j, N, bin, Ndegbin;
  unsigned short int *A;
  double Xo, Yo, dX, dY, angle, limit;

   /* assign some parameter values */
  if (mark[i]) return (FALSE);

  Ndegbin = NBINS / 180.0;
  ALLOCATE (A, unsigned short int, NBINS + 1);
  bzero (A, (NBINS+1)*sizeof(short));

  /* look for points concentrated in an angle bin */
  Xo = stars[i].X;
  Yo = stars[i].Y;
  N = 0;
  /* points east */
  for (j = i + 1; (j < Npts) && ((dX = stars[j].X - Xo) < RADIUS); j++) {
    dY = stars[j].Y - Yo;
    if (fabs(dY) < RADIUS) {
      angle = atan2 (dY,dX);
      if (!finite(angle)) continue;  /* only NaN if dD = dR = 0 */
      if (angle < 0) angle += M_PI;
      bin = angle*DEG_RAD*Ndegbin;
      A[bin] ++;
      N ++;
    }
  }
  /* points west */
  for (j = i - 1; (j >= 0) && ((dX = Xo - stars[j].X) < RADIUS); j--) {
    if (mark[j]) continue;
    dY = stars[j].Y - Yo;
    if (fabs(dY) < RADIUS) {
      angle = atan2 (dY,dX);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      bin = angle*DEG_RAD*Ndegbin;
      A[bin] ++;	
      N ++;
    }
  }
  if (N < 5) {
    free (A);
    return (FALSE);
  }
  limit = MAX (5, NSIGMA*sqrt((double)(N)/(double)(NBINS)));
  for (j = 0; j < NBINS; j++) {
    if (A[j] > limit) {
      *ANGLE = j / (DEG_RAD*Ndegbin);
      fprintf (stderr, "group: %f (%f %f)\n", *ANGLE, Xo, Yo);
      free (A);
      return (TRUE);
    }
  }
  free (A);
  return (FALSE);
}
  
