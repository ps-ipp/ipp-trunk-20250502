# include "gastro2.h"

static int NTERM, NPOWR, NPARS, NORDER, Npts;
static double **sum, **xsum, **ysum;
static double **matrix, **vector;

void fit_init (int order) {

  int i;

  NORDER = order;
  NPOWR = NORDER + 1;
  NTERM = 2*NORDER + 1;
  NPARS = (NORDER + 1)*(NORDER + 2) / 2;
  Npts  = 0;

  /* allocate arrays for fit solution */
  ALLOCATE (sum, double *, NTERM);
  ALLOCATE (xsum, double *, NTERM);
  ALLOCATE (ysum, double *, NTERM);
  for (i = 0; i < NTERM; i++) {
    ALLOCATE (sum[i], double, NTERM);
    bzero (sum[i], NTERM*sizeof(double));
    ALLOCATE (xsum[i], double, NTERM);
    bzero (xsum[i], NTERM*sizeof(double));
    ALLOCATE (ysum[i], double, NTERM);
    bzero (ysum[i], NTERM*sizeof(double));
  }
  ALLOCATE (matrix, double *, NPARS);
  ALLOCATE (vector, double *, NPARS);
  for (i = 0; i < NPARS; i++) {
    ALLOCATE (matrix[i], double, NPARS);
    ALLOCATE (vector[i], double, 2);
    bzero (vector[i], 2*sizeof(double));
    bzero (matrix[i], NPARS*sizeof(double));
  }
}

/* */
void fit_add (double x1, double y1, double x2, double y2, double wt) {

  int n, m;
  double xterm, yterm, term;

  xterm = 1;
  for (n = 0; n < NTERM; n++) {
    yterm = 1;
    for (m = 0; m < NTERM; m++) {
      term = xterm*yterm;
      if (n+m < NTERM) {
	sum[n][m] += term;
      }
      if (n+m < NPOWR) {
	xsum[n][m] += x2*term;
	ysum[n][m] += y2*term;
      }
      yterm *= y1;
    }
    xterm *= x1;
  }
  Npts ++;

}

void fit_eval () {

  int i, j, n, m, M, N;
  double max;

  i = 0;
  for (m = 0; m < NPOWR; m++) {
    for (n = 0; n < NPOWR - m; n++, i++) {
      vector[i][0] = xsum[n][m];
      vector[i][1] = ysum[n][m];
    }	
  }
  j = 0;
  for (M = 0; M < NPOWR; M++) {
    for (N = 0; N < NPOWR - M; N++, j++) {
      i = 0;
      for (m = 0; m < NPOWR; m++) {
	for (n = 0; n < NPOWR - m; n++, i++) {
	  matrix[i][j] = sum[n+N][m+M];
	}	
      }
    }
  }       
  max = 0.0;
  for (i = 0; i < NPARS; i++) {
    for (j = 0; j < NPARS; j++) {
      max = MAX (max, fabs(matrix[i][j]));
    }
    max = MAX (max, fabs(vector[i][0]));
    max = MAX (max, fabs(vector[i][1]));
  }
  for (i = 0; i < NPARS; i++) {
    for (j = 0; j < NPARS; j++) {
      matrix[i][j] /= max;
    }
    vector[i][0] /= max;
    vector[i][1] /= max;
  }
  dgaussjordan (matrix, vector, NPARS, 2); 
  i = 0;
  for (m = 0; m < NPOWR; m++) {
    for (n = 0; n < NPOWR - m; n++, i++) {
      xsum[n][m] = vector[i][0];
      ysum[n][m] = vector[i][1];
    }	
  }
  i = 0;
  for (m = 0; m < NPOWR; m++) {
    for (n = 0; n < NPOWR - m; n++, i++) {
      if (VERBOSE) fprintf (stderr, "RA x^%dy^%d: %10.4g    DEC x^%dy^%d: %10.4g \n", n, m, vector[i][0], n, m, vector[i][1]);
    }	
  }
}

void fit_norm () { 

  xsum[0][0] = 0;
  xsum[1][0] = 1;
  xsum[0][1] = 0;

  ysum[0][0] = 0;
  ysum[1][0] = 0;
  ysum[0][1] = 1;
}

/* evaluate the fit at (X,Y) to yield (x,y) */
void fit_apply (double *x, double *y, double X, double Y) {

  int m, n;
  double xterm, yterm;
  double Xo, Yo;

  Xo = Yo = 0;
  yterm = 1;
  for (m = 0; m < NPOWR; m++) { 
    xterm = 1;
    for (n = 0; n < NPOWR - m; n++) {
      Xo += xterm*yterm*xsum[n][m];
      Yo += xterm*yterm*ysum[n][m];
      xterm *= X;
    }	
    yterm *= Y;
  }
  
  *x = Xo;
  *y = Yo;
}

/* evaluate the x-derivative of the fit at (X,Y) to yield (x,y) */
void fit_apply_dx (double *x, double *y, double X, double Y) {

  int m, n;
  double xterm, yterm;
  double Xo, Yo;

  Xo = Yo = 0;
  yterm = 1;
  for (m = 0; m < NPOWR; m++) { 
    xterm = 1;
    for (n = 1; n < NPOWR - m; n++) {
      Xo += n*xterm*yterm*xsum[n][m];
      Yo += n*xterm*yterm*ysum[n][m];
      xterm *= X;
    }	
    yterm *= Y;
  }
  
  *x = Xo;
  *y = Yo;
}

/* evaluate the y-derivative of the fit at (X,Y) to yield (x,y) */
void fit_apply_dy (double *x, double *y, double X, double Y) {

  int m, n;
  double xterm, yterm;
  double Xo, Yo;

  Xo = Yo = 0;
  yterm = 1;
  for (m = 1; m < NPOWR; m++) { 
    xterm = 1;
    for (n = 0; n < NPOWR - m; n++) {
      Xo += m*xterm*yterm*xsum[n][m];
      Yo += m*xterm*yterm*ysum[n][m];
      xterm *= X;
    }	
    yterm *= Y;
  }
  
  *x = Xo;
  *y = Yo;
}

/* measure the residual scatter in the fit */
double fit_scat (StarData *st, StarData *sr, Coords *coords) {

  int i;
  int Npair, *idx1, *idx2;
  double x, y, dx, dy, dX, dY, dX2, dY2, dR;
  
  Npair = pair_lists (&idx1, &idx2);

  dX = dY = dX2 = dY2 = 0;
  for (i = 0; i < Npair; i++) {

    /* projection this direction includes the error introduced by
       the interation on the nonlinear solution */
    RD_to_XY (&x, &y, sr[idx2[i]].R, sr[idx2[i]].D, coords);
    
    dx = x - st[idx1[i]].X;
    dy = y - st[idx1[i]].Y;
    
    dX += dx;
    dY += dy;
    dX2 += dx*dx;
    dY2 += dy*dy;
  }

  /* scatter is measured on the tangent plane in degrees */
  dX = dX / Npair;
  dY = dY / Npair;
  fprintf (stderr, "scatter: %f, %f\n", sqrt(dX2/Npair - dX*dX), sqrt(dY2/Npair - dY*dY));
  fprintf (stderr, "Npts: %d\n", Npair);

  dR = 0.5 * sqrt(fabs(dX2/Npair - dX*dX)) + 0.5 * sqrt (fabs(dY2/Npair - dY*dY));
  return (dR);
}

/* convert fit terms to coords and polyterms */
/**** what do we do with the value of ctype??? 
      can we leave it alone? ****/
int fit_adjust (Coords *coords) {

  int i;
  double a10, a01, a20, a11, a02, a30, a21, a12, a03;
  double b10, b01, b20, b11, b02, b30, b21, b12, b03;
  double Xo, Yo, det;
  double **A, **B, Fx, Fy;
    
  /* start with the linear solution for Xo,Yo */
  coords[0].cdelt1 = hypot (xsum[1][0], ysum[1][0]);
  coords[0].cdelt2 = hypot (xsum[0][1], ysum[0][1]);
  // coords[0].cdelt1 = coords[0].cdelt2 = 1.0;

  det = 1.0 / (xsum[1][0]*ysum[0][1] - xsum[0][1]*ysum[1][0]);
  Xo = det*(ysum[0][0]*xsum[0][1] - xsum[0][0]*ysum[0][1]);
  Yo = det*(xsum[0][0]*ysum[1][0] - ysum[0][0]*xsum[1][0]);

  coords[0].Npolyterms = NORDER;

  if (coords[0].Npolyterms > 1) {
    /* use the linear solution as a starting guess */
    /* solve for L(Xo,Yo) = 0, M(Xo,Yo) = 0 */
    /* this is the Newton-Raphson method - it needs the high order terms to be small */
    ALLOCATE (A, double *, 2);
    ALLOCATE (B, double *, 2);
    ALLOCATE (A[0], double, 2);
    ALLOCATE (A[1], double, 2);
    ALLOCATE (B[0], double, 1);
    ALLOCATE (B[1], double, 1);

    for (i = 0; i < 10; i++) {
      fit_apply (&Fx, &Fy, Xo, Yo);
      fit_apply_dx (&A[0][0], &A[0][1], Xo, Yo);
      fit_apply_dy (&A[1][0], &A[1][1], Xo, Yo);
      B[0][0] = -Fx;
      B[1][0] = -Fy;
      dgaussjordan (A, B, 2, 1);
      Xo += B[0][0]; 
      Yo += B[1][0];
    }
    free (A[0]); free (B[0]);
    free (A[1]); free (B[1]);
    free (A); free (B);
  }
  coords[0].crpix1 = Xo;
  coords[0].crpix2 = Yo;

  switch (coords[0].Npolyterms) {
    case 0:
    case 1:
      /* the linear solution can be analytically inverted */
      coords[0].pc1_1 = xsum[1][0] / coords[0].cdelt1;
      coords[0].pc1_2 = xsum[0][1] / coords[0].cdelt2;
      coords[0].pc2_1 = ysum[1][0] / coords[0].cdelt1;
      coords[0].pc2_2 = ysum[0][1] / coords[0].cdelt2;
      for (i = 0; i < 7; i++) {
	coords[0].polyterms[i][0] = coords[0].polyterms[i][1] = 0.0;
      }
      break;

    case 2:
      a10 = xsum[1][0] + 2.0*xsum[2][0]*Xo + xsum[1][1]*Yo;
      a01 = xsum[0][1] + 2.0*xsum[0][2]*Yo + xsum[1][1]*Xo;
      a20 = xsum[2][0];
      a11 = xsum[1][1];
      a02 = xsum[0][2];

      b10 = ysum[1][0] + 2.0*ysum[2][0]*Xo + ysum[1][1]*Yo;
      b01 = ysum[0][1] + 2.0*ysum[0][2]*Yo + ysum[1][1]*Xo;
      b20 = ysum[2][0];
      b11 = ysum[1][1];
      b02 = ysum[0][2];

      coords[0].pc1_1 = a10 / coords[0].cdelt1;
      coords[0].pc1_2 = a01 / coords[0].cdelt2;
      coords[0].pc2_1 = b10 / coords[0].cdelt1;
      coords[0].pc2_2 = b01 / coords[0].cdelt2;

      coords[0].polyterms[0][0] = a20 / SQ(coords[0].cdelt1);
      coords[0].polyterms[1][0] = a11 / (coords[0].cdelt1*coords[0].cdelt2);
      coords[0].polyterms[2][0] = a02 / SQ(coords[0].cdelt2);

      coords[0].polyterms[0][1] = b20 / SQ(coords[0].cdelt1);
      coords[0].polyterms[1][1] = b11 / (coords[0].cdelt1*coords[0].cdelt2);
      coords[0].polyterms[2][1] = b02 / SQ(coords[0].cdelt2);
      for (i = 3; i < 7; i++) {
	coords[0].polyterms[i][0] = coords[0].polyterms[i][1] = 0.0;
      }
      break;
      
    case 3:
      a10 = xsum[1][0] + 2*xsum[2][0]*Xo +   xsum[1][1]*Yo + 3*xsum[3][0]*Xo*Xo + 2*xsum[2][1]*Xo*Yo + xsum[1][2]*Yo*Yo;
      a01 = xsum[0][1] + 2*xsum[0][2]*Yo +   xsum[1][1]*Xo + 3*xsum[0][3]*Yo*Yo + 2*xsum[1][2]*Xo*Yo + xsum[2][1]*Xo*Xo;
      a20 = xsum[2][0] + 3*xsum[3][0]*Xo +   xsum[2][1]*Yo;
      a11 = xsum[1][1] + 2*xsum[2][1]*Xo + 2*xsum[1][2]*Yo;
      a02 = xsum[0][2] + 3*xsum[0][3]*Yo +   xsum[1][2]*Xo;
      a30 = xsum[3][0];
      a21 = xsum[2][1];
      a12 = xsum[1][2];
      a03 = xsum[0][3];

      b10 = ysum[1][0] + 2*ysum[2][0]*Xo +   ysum[1][1]*Yo + 3*ysum[3][0]*Xo*Xo + 2*ysum[2][1]*Xo*Yo + ysum[1][2]*Yo*Yo;
      b01 = ysum[0][1] + 2*ysum[0][2]*Yo +   ysum[1][1]*Xo + 3*ysum[0][3]*Yo*Yo + 2*ysum[1][2]*Xo*Yo + ysum[2][1]*Xo*Xo;
      b20 = ysum[2][0] + 3*ysum[3][0]*Xo +   ysum[2][1]*Yo;
      b11 = ysum[1][1] + 2*ysum[2][1]*Xo + 2*ysum[1][2]*Yo;
      b02 = ysum[0][2] + 3*ysum[0][3]*Yo +   ysum[1][2]*Xo;
      b30 = ysum[3][0];
      b21 = ysum[2][1];
      b12 = ysum[1][2];
      b03 = ysum[0][3];

      coords[0].pc1_1 = a10 / coords[0].cdelt1;
      coords[0].pc1_2 = a01 / coords[0].cdelt2;
      coords[0].pc2_1 = b10 / coords[0].cdelt1;
      coords[0].pc2_2 = b01 / coords[0].cdelt2;

      coords[0].polyterms[0][0] = a20 / SQ(coords[0].cdelt1);
      coords[0].polyterms[1][0] = a11 / (coords[0].cdelt1*coords[0].cdelt2);
      coords[0].polyterms[2][0] = a02 / SQ(coords[0].cdelt2);

      coords[0].polyterms[3][0] = a30 / (SQ(coords[0].cdelt1)*coords[0].cdelt1);
      coords[0].polyterms[4][0] = a21 / (SQ(coords[0].cdelt1)*coords[0].cdelt2);
      coords[0].polyterms[5][0] = a12 / (SQ(coords[0].cdelt2)*coords[0].cdelt1);
      coords[0].polyterms[6][0] = a03 / (SQ(coords[0].cdelt2)*coords[0].cdelt2);

      coords[0].polyterms[0][1] = b20 / SQ(coords[0].cdelt1);
      coords[0].polyterms[1][1] = b11 / (coords[0].cdelt1*coords[0].cdelt2);
      coords[0].polyterms[2][1] = b02 / SQ(coords[0].cdelt2);

      coords[0].polyterms[3][1] = b30 / (SQ(coords[0].cdelt1)*coords[0].cdelt1);
      coords[0].polyterms[4][1] = b21 / (SQ(coords[0].cdelt1)*coords[0].cdelt2);
      coords[0].polyterms[5][1] = b12 / (SQ(coords[0].cdelt2)*coords[0].cdelt1);
      coords[0].polyterms[6][1] = b03 / (SQ(coords[0].cdelt2)*coords[0].cdelt2);
      break;

    default:
      fprintf (stderr, "error: invalid order %d\n", coords[0].Npolyterms);
      exit (2);
  }

  coords[0].crval1 = ohana_normalize_angle (coords[0].crval1);

  /* test for valid solution */
  return (Npts);
}

int mk_polyterm (int n, int m, int norder) {
  
  int i, nt, N;
  
  N = 0;
  nt = n + m;
  for (i = 2; i < nt; i++) {
    N += i + 1;
  }
  N += m;
  return (N);
}

int mk_vector (int n, int m, int norder) {
  
  int i, N;
  
  N = 0;
  for (i = 0; i < m; i++) {
    N += (norder - i + 1);
  }
  N += n;
  return (N);
}

