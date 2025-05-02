# include "markstar.h"

find_group (catstats, mark, Npts, i, ANGLE)
     CatStats catstats[];
     double *ANGLE;
     int i, Npts;
     char *mark;
{
 
  float *R, *D;
  int j, N, bin, Ndegbin;
  unsigned short int *A;
  double Ra, De, dR, dD, angle, limit;

   /* assign some parameter values */
  
  R = catstats[0].X;
  D = catstats[0].Y;
  Ndegbin = NBINS / 180.0;
  ALLOCATE (A, unsigned short int, NBINS + 1)
  bzero (A, (NBINS+1)*sizeof(short));

  /* look for points concentrated in an angle bin */
  if (mark[i]) return (FALSE);
  Ra = R[i];
  De = D[i];
  N = 0;
  /* points east */
  for (j = i + 1; (j < Npts) && (R[j] - Ra < RADIUS); j++) {
    if (mark[j]) continue;
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      angle = atan2 (dD,dR);
      if (!finite(angle)) continue;  /* only NaN if dD = dR = 0 */
      if (angle < 0) angle += M_PI;
      bin = angle*DEG_RAD*Ndegbin;
      A[bin] ++;
      N ++;
    }
  }
  /* points west */
  for (j = i - 1; (j >= 0) && (Ra - R[j] < RADIUS); j--) {
    if (mark[j]) continue;
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      angle = atan2 (dD,dR);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      bin = angle*DEG_RAD*Ndegbin;
      A[bin] ++;	
      N ++;
    }
  }
  limit = NSIGMA*sqrt((double)(N)/(double)(NBINS));
  for (j = 0; j < NBINS; j++) {
    if (A[j] > limit) {
      *ANGLE = j / (DEG_RAD*Ndegbin);
      return (TRUE);
    }
  }
  return (FALSE);
}
  
