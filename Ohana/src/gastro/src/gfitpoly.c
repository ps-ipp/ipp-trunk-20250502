# include "gastro.h"

void gfitpoly (SStars *stars1, SStars *stars2, int N1, int N2, Coords *coords, double *Radius, double *DR, int *Nmatch) {
  
  int i, j, m, n, M, N;
  int first_j, last;
  off_t *tmpN1, *tmpN2;
  int NORDER, NTERM, NPARS, NPOWR;
  double **sum, **xsum, **ysum;
  double **matrix, **vector;
  double Dx, Dy, DD, dX, dY, d2X, d2Y;
  double radius, radius2;
  double *tmpX1, *tmpX2, *tmpY1, *tmpY2;
  double tX1, tX2, tY1, tY2;
  double xterm, yterm, term, max;

  NORDER = NPOLYTERMS;
  NPOWR = NORDER + 1;
  NTERM = 2*NORDER + 1;
  NPARS = (NORDER + 1)*(NORDER + 2) / 2;
  if (NPOLYTERMS < 2) {
    coords[0].Npolyterms = 0;
    {
      double nominal_det, measure_det, min_det, max_det, d1, d2, diffangle, tmp;
      
      nominal_det = CCD_PC1_1 * CCD_PC2_2 - CCD_PC1_2 * CCD_PC2_1;
      min_det = nominal_det / 1.05;
      max_det = nominal_det * 1.05;
      if (min_det > max_det) {
	tmp = max_det; max_det = min_det; min_det = tmp;
      }
      
      measure_det = coords[0].pc1_1*coords[0].pc2_2 - coords[0].pc1_2*coords[0].pc2_1;
      if ((measure_det > max_det) || (measure_det < min_det)) {
	fprintf (stderr, "absurd solution, not cartesian\n");
	*DR = 1e9;
      }
      d1 = hypot (coords[0].pc1_2, coords[0].pc1_1);
      d2 = hypot (coords[0].pc2_2, coords[0].pc2_1);
      diffangle = fabs (coords[0].pc2_1*coords[0].pc1_1 + coords[0].pc1_2*coords[0].pc2_2) / (d1*d2);
      if (diffangle > MAX_NONLINEAR) {
	fprintf (stderr, "absurd solution, not cartesian\n");
	*DR = 1e9;
      }
    }
    return;
  }

  fprintf (stderr, "\nattempting higher order fit\n");

  /* allocate space for star coords */
  ALLOCATE (tmpX1, double, N1);
  ALLOCATE (tmpY1, double, N1);
  ALLOCATE (tmpN1, off_t, N1);

  ALLOCATE (tmpX2, double, N2);
  ALLOCATE (tmpY2, double, N2);
  ALLOCATE (tmpN2, off_t, N2);

  /* assign and sort list */
  for (i = 0; i < N1; i++) {
    tmpX1[i] = stars1[i].X;
    tmpY1[i] = stars1[i].Y;
    tmpN1[i] = i;
  }
  if (N1 > 1) sort_coords_index (tmpX1, tmpY1, tmpN1, N1);
  for (i = 0; i < N2; i++) {
    tmpX2[i] = stars2[i].X;
    tmpY2[i] = stars2[i].Y;
    tmpN2[i] = i;
  }
  if (N2 > 1) sort_coords_index (tmpX2, tmpY2, tmpN2, N2);
  
  /* choose iteration ranges */
  radius = MINIMUM_RADIUS;
  radius2 = radius*radius;

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
  
  /* do the following loop twice.  
     on first pass, find the coeffs.
     on next pass, find just the residuals */

  dX = dY = d2X = d2Y = N = 0;
  for (last = 0; last < 2; last++) {
    if (last) {
      /* assign values based on determined coeffs */
      for (i = 0; i < N1; i++) {
	tmpX1[i] = tmpY1[i] = 0;
	yterm = 1;
	for (m = 0; m < NPOWR; m++) { 
	  xterm = 1;
	  for (n = 0; n < NPOWR - m; n++) {
	    tmpX1[i] += xterm*yterm*xsum[n][m];
	    tmpY1[i] += xterm*yterm*ysum[n][m];
	    xterm *= stars1[i].X;
	  }	
	  yterm *= stars1[i].Y;
	}
	tmpN1[i] = i;
      }
      if (N1 > 1) sort_coords_index (tmpX1, tmpY1, tmpN1, N1);
      dX = dY = d2X = d2Y = N = 0;
    }
    /* find matched stars */
    for (i = j = 0; (i < N1) && (j < N2); ) {  
      tX1 = tmpX1[i];
      tX2 = tmpX2[j];
      Dx = tX1 - tX2;
      if (Dx <= -2.0*radius) {
	i++;
	continue;
      }
      if (Dx >= 2.0*radius) {
	j++;
	continue;
      }
      /* in the right range */
      first_j = j;
      for (; (Dx > -2.0*radius) && (j < N2); j++) {
	tY1 = tmpY1[i];
	tX2 = tmpX2[j];
	tY2 = tmpY2[j];
	Dx = tX1 - tX2;
	Dy = tY1 - tY2;
	DD = Dx*Dx + Dy*Dy;
	/* stars matched */
	if (DD < radius2) {
	  if (last) {  /* calculate residuals */
	    dX += Dx;
	    dY += Dy;
	    d2X += Dx*Dx;
	    d2Y += Dy*Dy;
	    N  += 1.0; 
	  } else {    /* accumulate data for coeffs */
	    xterm = 1;
	    for (n = 0; n < NTERM; n++) {
	      yterm = 1;
	      for (m = 0; m < NTERM; m++) {
		term = xterm*yterm;
		if (n+m < NTERM) {
		  sum[n][m] += term;
		}
		if (n+m < NPOWR) {
		  xsum[n][m] += tX2*term;
		  ysum[n][m] += tY2*term;
		}
		yterm *= tY1;
	      }
	      xterm *= tX1;
	    }
	  }
	}
      }
      j = first_j;
      i++;
    }
    
    if (!last) { /* calculate polyterm coeffs */
      fprintf (stderr, "matched %.0f stars for polyterms\n", sum[0][0]);
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
      /* svd (matrix, NPARS, vector, 2);  */
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
	  fprintf (stderr, "RA x^%dy^%d: %10.4g    DEC x^%dy^%d: %10.4g \n", 
		   n, m, vector[i][0], n, m, vector[i][1]);
	}	
      }
    } else {
      fprintf (stderr, "%d stars matched for residuals\n", N);
      
      dX = sqrt(d2X/N - dX*dX/(N*N));  /* scatter in pixels in the X direction */
      dY = sqrt(d2Y/N - dY*dY/(N*N));  /* scatter in pixels in the Y direction */
      if (VERBOSE) fprintf (stderr, "scatter in pixels: %5.2f x %5.2f -- %5.2f\n", dX, dY, hypot(dX,dY) / sqrt(N));
      *DR = sqrt (SQ(dX*coords[0].cdelt1*3600.0) + SQ(dY*coords[0].cdelt1*3600.0));
      *Nmatch = N;

    }
  } 

  /* convert new terms to adjustments in coords and to polyterms */
  {
    double S1, S2, p11, p12, p21, p22;
    double a0, a1, a2, b0, b1, b2, det;
    double X, Y;
    int Np, Nv;
    
    S1 = coords[0].cdelt1;
    S2 = coords[0].cdelt2;
    p11 = coords[0].pc1_1;    p12 = coords[0].pc1_2;
    p21 = coords[0].pc2_1;    p22 = coords[0].pc2_2;
    
    /* get the correct vector entries for the linear terms */
    N = mk_vector (0, 0, NORDER);
    a0 = vector[N][0];  b0 = vector[N][1];
    N = mk_vector (1, 0, NORDER);
    a1 = vector[N][0];  b1 = vector[N][1];
    N = mk_vector (0, 1, NORDER);
    a2 = vector[N][0];  b2 = vector[N][1];

    det = 1.0 / (a1*b2 - a2*b1);

    InitCoords (coords, "DEC--PLY");

    coords[0].pc1_1 = p11*a1 + p12*b1*(S2/S1);
    coords[0].pc2_1 = p21*a1 + p22*b1*(S2/S1);
    
    coords[0].pc1_2 = p12*b2 + p11*a2*(S1/S2);
    coords[0].pc2_2 = p22*b2 + p21*a2*(S1/S2);
    
    X = (coords[0].crpix1 - a0);
    Y = (coords[0].crpix2 - b0);
    coords[0].crpix1 = det*(X*b2 - Y*a2);
    coords[0].crpix2 = det*(Y*a1 - X*b1);

    coords[0].Npolyterms = NORDER;

    /* generate higher order terms from vector */

    for (i = 0; i < NORDER + 1; i++) {
      for (j = 0; j < (NORDER - i + 1); j++) {
	if (i + j < 2) continue;
	Np = mk_polyterm (i, j, NORDER);
	Nv = mk_vector (i, j, NORDER);
	coords[0].polyterms[Np][0] = det*(vector[Nv][0]*b2  - vector[Nv][1]*a2);  /* x2 y0 */
	coords[0].polyterms[Np][1] = det*(vector[Nv][1]*a1  - vector[Nv][0]*b1);  /* x2 y0 */
      }
    }
  }

  coords[0].crval1 = ohana_normalize_angle (coords[0].crval1);

  {
    double nominal_det, measure_det, min_det, max_det, tmp;
    
    nominal_det = CCD_PC1_1 * CCD_PC2_2 - CCD_PC1_2 * CCD_PC2_1;
    min_det = nominal_det / 1.05;
    max_det = nominal_det * 1.05;
    if (min_det > max_det) {
      tmp = max_det; max_det = min_det; min_det = tmp;
    }
    
    measure_det = coords[0].pc1_1*coords[0].pc2_2 - coords[0].pc1_2*coords[0].pc2_1;
    if ((measure_det > max_det) || (measure_det < min_det)) {
      fprintf (stderr, "absurd solution, not cartesian: %f (%f - %f)\n", measure_det, max_det, min_det);
      *DR = 1e9;
    }
  }
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



/**********************

  vector vs polyterms:

  the variable 'vector' is similar to, but not exactly like polyterms. 
  vector[i][j] provides coeffs for all of the x,y terms, 
    polyterms[i][j] only provides coeffs for the terms or order > 2.

  vector[i][j] and polyterms[i][j] also use a slightly different order:

  vector[i][j] runs in this order:

                   n     m  norder = 3
  vector[0][0] * x^0 * y^0
  vector[1][0] * x^1 * y^0
  vector[2][0] * x^2 * y^0
  vector[3][0] * x^3 * y^0
  vector[4][0] * x^0 * y^1
  vector[5][0] * x^1 * y^1
  vector[6][0] * x^2 * y^1
  vector[7][0] * x^0 * y^2
  vector[8][0] * x^1 * y^2
  vector[9][0] * x^0 * y^3

  to generate the vector entry from n, m, norder:
  N = 0;
  for (i = 0; i < m; i++) {
    N += (norder - i + 1);
  }
  N += n;

  polyterms[i][j] runs in this order:

                      n     m
  polyterms[0][0] * x^2 * y^0 = vector[2][0]
  polyterms[1][0] * x^1 * y^1 = vector[5][0]
  polyterms[2][0] * x^0 * y^2 = vector[7][0]
  polyterms[3][0] * x^3 * y^0 = vector[3][0]
  polyterms[4][0] * x^2 * y^1 = vector[6][0]
  polyterms[5][0] * x^1 * y^2 = vector[8][0]
  polyterms[6][0] * x^0 * y^3 = vector[9][0]
  
  to generate the polyterms entry from n, m, norder:

  N = 0;
  nt = n + m;
  for (i = 2; i < nt; i++) {
    N += i + 1;
  }
  N += m;

  */
