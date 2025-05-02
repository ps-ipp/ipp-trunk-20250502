# include "markstar.h"

find_line (catstats, mark, Npts, i, M, B, Angle)
     CatStats catstats[];
     double *M, *B, Angle;
     int i, Npts;
     char *mark;
{
  
  float *R, *D;
  int j, N;
  double X, Y, X2, Y2, XY, m, b, det;
  double dR, dD, Ra, De, angle;
  char Flipped;
  
  R = catstats[0].X;
  D = catstats[0].Y;

  /* fit a line to points near line */
  Ra = R[i];
  De = D[i];
  X = Ra;
  Y = De;
  X2 = Ra*Ra;
  Y2 = De*De;
  XY = Ra*De;
  N = 1;
  /* points to the east */
  for (j = i + 1; (j < Npts) && (R[j] - Ra < RADIUS); j++) {
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      angle = atan2 (dD,dR);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      if (fabs(angle - Angle) < RAD_DEG) {
	X += R[j];
	Y += D[j];
	X2 += R[j]*R[j];
	Y2 += D[j]*D[j];
	N ++;
	XY += R[j]*D[j];
      }
    }
  }
  /* points to the west */
  for (j = i - 1; !mark[i] && (j >= 0) && (Ra - R[j] < RADIUS); j--) {
    if (mark[j]) continue;
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      angle = atan2 (dD,dR);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      if (fabs(angle - Angle) < RAD_DEG) {
	X += R[j];
	Y += D[j];
	X2 += R[j]*R[j];
	Y2 += D[j]*D[j];
	N ++;
	XY += R[j]*D[j];
      }
    }
  }
  /* determine coeffs */
  Flipped = 0;
  det = 1.0 / (X2*N - X*X);
  m = det * (XY*N - X*Y);
  b = det * (X2*Y - XY*X);
  if (fabs(m) > 1.1) { /* use a line of R = m*D + b instead */
    /* fprintf (stderr, "high slope object: %f %f  -> ", m, b); */
    det = 1.0 / (Y2*N - Y*Y);
    m = det * (XY*N - X*Y);
    b = det * (Y2*X - XY*Y);
    Flipped = 1;
    /* fprintf (stderr, "%f %f\n", m, b); */
  }

  *M = m;
  *B = b;
  return (Flipped);

}


find_better_line (catstats, mark, Npts, i, M, B, axis)
     CatStats catstats[];
     double *M, *B;
     int i, Npts, axis;
     char *mark;
{
  
  float *R, *D;
  int j, N;
  double X, Y, X2, Y2, XY, m, b, det;
  double dR, dD, Ra, De, delta;
  
  R = catstats[0].X;
  D = catstats[0].Y;

  /* fit a line to points near line */
  Ra = R[i];
  De = D[i];
  X = Y = X2 = Y2 = XY = N = 0;
  m = *M;  b = *B;

  /* points to the east */
  for (j = i; (j < Npts) && (R[j] - Ra < RADIUS); j++) {
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      if (axis == 1) 
	delta = R[j] - m*D[j] - b;
      else
	delta = D[j] - m*R[j] - b;
      if (fabs(delta) < 2*TRAIL_WIDTH) {
	X += R[j];
	Y += D[j];
	X2 += R[j]*R[j];
	Y2 += D[j]*D[j];
	N ++;
	XY += R[j]*D[j];
      }
    }
  }
  /* points to the west */
  for (j = i - 1; (j >= 0) && (Ra - R[j] < RADIUS); j--) {
    dD = D[j] - De;
    if (fabs(dD) < RADIUS) {
      dR = R[j] - Ra;
      if (axis == 1) 
	delta = R[j] - m*D[j] - b;
      else
	delta = D[j] - m*R[j] - b;
      if (fabs(delta) < 2*TRAIL_WIDTH) {
	X += R[j];
	Y += D[j];
	X2 += R[j]*R[j];
	Y2 += D[j]*D[j];
	N ++;
	XY += R[j]*D[j];
      }
    }
  }
  /* determine coeffs */
  if (axis == 0) {
    det = 1.0 / (X2*N - X*X);
    m = det * (XY*N - X*Y);
    b = det * (X2*Y - XY*X);
  } else {
    det = 1.0 / (Y2*N - Y*Y);
    m = det * (XY*N - X*Y);
    b = det * (Y2*X - XY*Y);
  }

  /* fprintf (stderr, "%f %f %d\n", m, b, N); */

  *M = m;
  *B = b;

}
