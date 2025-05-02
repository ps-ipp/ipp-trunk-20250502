# include "markrock.h"

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
  limit = 10.0*sqrt((double)(M_PI*RADIUS*RADIUS*catstats[0].density)/(double)(NBINS));
  Ndegbin = NBINS / 180.0;
  ALLOCATE (A, unsigned short int, NBINS)
  bzero (A, NBINS*sizeof(short));

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
      if (!finite(angle)) 
	continue;
      if (angle < 0) angle += M_PI;
      bin = 1 + angle*DEG_RAD*Ndegbin;
      A[bin] ++;
      if (A[bin] > limit) {
	*ANGLE = (bin - 1.0) / (DEG_RAD*Ndegbin);
	return (TRUE);
      }
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
      if (!finite(angle)) 
	continue;
      if (angle < 0) angle += M_PI;
      bin = 1 + angle*DEG_RAD*Ndegbin;
      A[bin] ++;	
      if (A[bin] > limit) {
	*ANGLE = (bin - 1.0) / (DEG_RAD*Ndegbin);
	return (TRUE);
      }
      N ++;
    }
  }
  return (FALSE);
}
  
