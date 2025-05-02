# include "mosastro.h"

static int NTERM, NPOWER, NPARS, NORDER, Npts;
static double **sum, **xsum, **ysum;
static double **matrix, **vector;

void fit_init (int order) {

  int i;

  Npts  = 0;
  NORDER = order;
  NPOWER = NORDER + 1;
  NTERM = 2*NORDER + 1;
  NPARS = (NORDER + 1)*(NORDER + 2) / 2;

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

# define SCALE 1.0
void fit_add (double x1, double y1, double x2, double y2) {

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
      if (n+m < NPOWER) {
	xsum[n][m] += x2*term;
	ysum[n][m] += y2*term;
      }
      yterm *= y1/SCALE;
    }
    xterm *= x1/SCALE;
  }
  Npts ++;
}

/** I am renormalizing here by the max pivots to keep gaussj sane **
 ** would not be needed if the fit used scaled ind. variables **/
void fit_eval () {

  int i, j, n, m, M, N;

  if (Npts == 0) {
    fprintf (stderr, "warning: no valid pts\n");
  }

  i = 0;
  for (m = 0; m < NPOWER; m++) {
    for (n = 0; n < NPOWER - m; n++, i++) {
      vector[i][0] = xsum[n][m];
      vector[i][1] = ysum[n][m];
    }	
  }
  j = 0;
  for (M = 0; M < NPOWER; M++) {
    for (N = 0; N < NPOWER - M; N++, j++) {
      i = 0;
      for (m = 0; m < NPOWER; m++) {
	for (n = 0; n < NPOWER - m; n++, i++) {
	  matrix[i][j] = sum[n+N][m+M];
	}	
      }
    }
  }       
# if (0)
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
# endif

  dgaussjordan (matrix, vector, NPARS, 2); 

# if (0)
  i = 0;
  for (m = 0; m < NPOWER; m++) {
    for (n = 0; n < NPOWER - m; n++, i++) {
      fprintf (stderr, "RA x^%dy^%d: %10.4g    DEC x^%dy^%d: %10.4g \n", 
	       n, m, vector[i][0], n, m, vector[i][1]);
    }	
  }
# endif
}

/* linear portion of fit : NORDER is 1 */
void fit_apply_coords (Coords *coords) {

  int i, j, Np, Nv, N;
  double c11, c12;
  double c21, c22;
  double R;

  InitCoords (coords, "DEC--PLY");

  /* update the higher order terms */
  if (NORDER > 1) {
    for (i = 0; i < NPOWER; i++) {
      for (j = 0; j < (NPOWER - i); j++) {
	if (i + j < 2) continue;
	Np = mkpolyterm (i, j);
	Nv = mkvector (i, j, NORDER);
	coords[0].polyterms[Np][0] = vector[Nv][0];
	coords[0].polyterms[Np][1] = vector[Nv][1];
      }
    }
  }

  /* get the correct vector entries for the linear terms */
  N = mkvector (0, 0, NORDER);
  coords[0].crval1 = vector[N][0];  
  coords[0].crval2 = vector[N][1];

  N = mkvector (1, 0, NORDER);
  c11 = vector[N][0];  
  c21 = vector[N][1];
  N = mkvector (0, 1, NORDER);
  c12 = vector[N][0];  
  c22 = vector[N][1];
  coords[0].cdelt1 = coords[0].cdelt2 = sqrt(fabs(c11*c22 - c12*c21));
  R = 1 / coords[0].cdelt1;

  coords[0].pc1_1  = c11*R;
  coords[0].pc2_1  = c21*R;
  coords[0].pc1_2  = c12*R;
  coords[0].pc2_2  = c22*R;

  coords[0].crpix1 = 0;
  coords[0].crpix2 = 0;

  coords[0].Npolyterms = NORDER;
}

/*
  if we have just linear terms, the following holds for crpix1,2:
  D = R / (coords[0].pc1_1*coords[0].pc2_2 - coords[0].pc1_2*coords[0].pc2_1);
  coords[0].crpix1 = D * (coords[0].pc1_2*c20 - coords[0].pc2_2*c10);
  coords[0].crpix2 = D * (coords[0].pc2_1*c10 - coords[0].pc1_1*c20);
*/

/* NORDER is order of gradient fit : Npolyterms is Norder + 1 */
void fit_apply_grads (Coords *distort, Coords *project, int term) {

  int i, j, Np, Nv1, Nv2;

  distort[0].Npolyterms = NORDER + 1;
  if (distort[0].Npolyterms > 1) strcpy (distort[0].ctype, "DEC--PLY");

  for (i = 0; i < NPOWER + 1; i++) {
    for (j = 0; j < (NPOWER + 1 - i); j++) {
      
      if (i + j < 2) continue;
      Np  = mkpolyterm (i, j);
      Nv1 = mkvector (i-1, j, NORDER);
      Nv2 = mkvector (i, j-1, NORDER);

      /** why do we have the negative sign? **/
      if (j == 0) {
	distort[0].polyterms[Np][term] = vector[Nv1][0] / i;
      }
      if (i == 0) {
	distort[0].polyterms[Np][term] = vector[Nv2][1] / j;
      }
      if ((i > 0) && (j > 0)) {
	distort[0].polyterms[Np][term] = 0.5*(vector[Nv1][0] / i + vector[Nv2][1] / j);
      }
    }
  }

  Nv1 = mkvector (0, 0, NORDER);
  if (term == 0) {
    project[0].pc1_1 = project[0].pc1_1 * (1 + vector[Nv1][0]);
    project[0].pc1_2 = project[0].pc1_2 * (1 + vector[Nv1][1]);
  } else {
    project[0].pc2_1 = project[0].pc2_1 * (1 + vector[Nv1][0]);
    project[0].pc2_2 = project[0].pc2_2 * (1 + vector[Nv1][1]);
  }
}

void fit_correct_grads (Gradients *in, Gradients *out, int term) {

  int i, k, m, n;
  double x, y, dx, dy, dz1, dz2;

  for (i = 0; i < in[0].Npts; i++) {
    
    dx = in[0].Lo[i];
    dy = in[0].Mo[i];
    dz1 = dz2 = 0.0;

    k = 0;
    x = y = 1;
    for (m = 0; m < NPOWER; m++) {
      x = y;
      for (n = 0; n < NPOWER - m; n++, k++) {
	dz1 += vector[k][0]*x;
	dz2 += vector[k][1]*x;
	x = x * dx / SCALE;
      }
      y = y * dy / SCALE;
    }

    out[0].Lo[i] = dx;
    out[0].Mo[i] = dy;
    if (term == 0) {
      out[0].dPdL[i] = in[0].dPdL[i] - dz1;
      out[0].dPdM[i] = in[0].dPdM[i] - dz2;
    } else {
      out[0].dQdL[i] = in[0].dQdL[i] - dz1;
      out[0].dQdM[i] = in[0].dQdM[i] - dz2;
    }
  }
}

int mkvector (int n, int m, int norder) {
  
  int i, N;
  
  N = 0;
  for (i = 0; i < m; i++) {
    N += (norder - i + 1);
  }
  N += n;
  return (N);
}

void fit_free () {

  int i;

  for (i = 0; i < NTERM; i++) {
    free (sum[i]);
    free (xsum[i]);
    free (ysum[i]);
  }
  free (sum);
  free (xsum);
  free (ysum);

  for (i = 0; i < NPARS; i++) {
    free (matrix[i]);
    free (vector[i]);
  }
  free (matrix);
  free (vector);
}
  
