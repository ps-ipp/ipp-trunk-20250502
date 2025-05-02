# include "imclean.h"

int find_line (SMPData *stars, char *mark, int Npts, int i, double *M, double *B, double Angle) {
  
  int j, N;
  double X, Y, X2, Y2, XY, m, b, det;
  double dX, dY, Xo, Yo, angle;
  char Flipped;
  
  Xo = stars[i].X;
  Yo = stars[i].Y;
  X = Xo;
  Y = Yo;
  X2 = Xo*Xo;
  Y2 = Yo*Yo;
  XY = Xo*Yo;
  N = 1;
  /* points to the right */
  for (j = i + 1; (j < Npts) && ((dX = stars[j].X - Xo) < RADIUS); j++) {
    if (mark[j]) continue;
    dY = stars[j].Y - Yo;
    if (fabs(dY) < RADIUS) {
      angle = atan2 (dY,dX);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      if (fabs(angle - Angle) < 2*RAD_DEG) {
	X += stars[j].X;
	Y += stars[j].Y;
	X2 += stars[j].X*stars[j].X;
	Y2 += stars[j].Y*stars[j].Y;
	N ++;
	XY += stars[j].X*stars[j].Y;
      }
    }
  }
  /* points to the left */
  for (j = i - 1; (j >= 0) && ((dX = Xo - stars[j].X) < RADIUS); j--) {
    if (mark[j]) continue;
    dY = stars[j].Y - Yo;
    if (fabs(dY) < RADIUS) {
      angle = atan2 (dY,dX);
      if (!finite(angle)) continue;
      if (angle < 0) angle += M_PI;
      if (fabs(angle - Angle) < 2*RAD_DEG) {
	X += stars[j].X;
	Y += stars[j].Y;
	X2 += stars[j].X*stars[j].X;
	Y2 += stars[j].Y*stars[j].Y;
	N ++;
	XY += stars[j].X*stars[j].Y;
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


void find_better_line (SMPData *stars, char *mark, int Npts, int i, double *M, double *B, int axis) {
  
  int j, N;
  double X, Y, X2, Y2, XY, m, b, det, dist;
  double delta, *path;
  int NLINE, *line;

  NLINE = 50;
  ALLOCATE (line, int, NLINE);
  ALLOCATE (path, double, NLINE);
  
  /* fit a line to points near line */
  X = Y = X2 = Y2 = XY = N = 0;
  m = *M;  b = *B;
  for (j = 0; (j < Npts); j++) {
    if (axis == 1) 
      delta = stars[j].X - m*stars[j].Y - b;
    else
      delta = stars[j].Y - m*stars[j].X - b;
    if (fabs(delta) < 2*TRAIL_WIDTH) {
      X += stars[j].X;
      Y += stars[j].Y;
      X2 += stars[j].X*stars[j].X;
      Y2 += stars[j].Y*stars[j].Y;
      XY += stars[j].X*stars[j].Y;
      line[N] = j;
      path[N] = hypot (stars[j].X - stars[line[0]].X, stars[j].Y - stars[line[0]].Y);
      N ++;
      if (N == NLINE - 1) {
	NLINE += 50;
	REALLOCATE (line, int, NLINE);
	REALLOCATE (path, double, NLINE);
      }
    }
  }

  if (N < NPTSINLINE) {
    free (line);
    return;
  }

  for (i = 0; i < N - NPTSINLINE + 1; i++) {
    j = i + NPTSINLINE - 1;
    dist = fabs (path[j] - path[i]);
    if ((j - i) / dist < MIN_DENSITY) continue;
    for (; (j < N) && (((j - i) / dist) > MIN_DENSITY); j++) {
      dist = fabs (path[j] - path[i]);
    }
    if ((j == N) && (((j - i) / dist) > MIN_DENSITY)) j++;
    j--;
    for (; i < j; i++) {
      mark[line[i]] = TRUE;
      stars[line[i]].dophot = 0;
    }
    i--;
  }
  free (line);
  free (path);

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

  fprintf (stderr, "%f %f %d\n", m, b, N);

  *M = m;
  *B = b;

}

void fix_total (SMPData *stars, int Nstars, Header *header) {
  
  int Ngood, i;

  Ngood = 0;
  for (i = 0; i < Nstars; i++) {
    if (stars[i].dophot != 0) Ngood ++;
  }

  gfits_modify (header, "NSTARS", "%d", 1, Ngood);
  
}
  
