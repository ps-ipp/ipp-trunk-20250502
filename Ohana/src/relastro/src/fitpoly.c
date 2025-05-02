# include "relastro.h"

/* these functions support simultaneous 2D fits to
   x2 = \sum a_i,j x1^i y1^j 
   y2 = \sum b_i,j x1^i y1^j 

   the order of the fit (largest coefficient) is fixed to a single
   value for both x1,y1 terms and for x2,y2 fits

   the code is currently confusing because we limit to i+j <= order.
   this could be cleaner if we used masks and allowed i <= order, j <= order
*/

double **array_init (int Nx, int Ny) {

  int i;
  double **array;

  ALLOCATE (array, double *, Nx);
  for (i = 0; i < Nx; i++) {
    ALLOCATE (array[i], double, Ny);
    memset (array[i], 0, Ny*sizeof(double));
  }    
  return (array);
}

void array_free (double **array, int Nx) {

  int i;

  for (i = 0; i < Nx; i++) {
    free (array[i]);
  }    
  free (array);
}

// XXX define a fit structure and drop the file static variables?
CoordFit *fit_init (int order) {

  CoordFit *fit;

  ALLOCATE (fit, CoordFit, 1);

  fit[0].Npts   = 0;
  fit[0].Norder = order;
  fit[0].Nterms = order + 1;
  fit[0].Nsums  = 2*order + 1;
  fit[0].Nelems = SQ(order + 1);

  /* summing arrays for fit solution */

  // xsum[i][j] holds \sum (x2 wt x1^i y1^j)
  // ysum[i][j] holds \sum (y2 wt x1^i y1^j)
  fit[0].xsum = array_init (fit[0].Nterms, fit[0].Nterms);
  fit[0].ysum = array_init (fit[0].Nterms, fit[0].Nterms);

  // xfit[i][j] holds x2 coeff for x1^i y1^j
  // yfit[i][j] holds y2 coeff for x1^i y1^j
  fit[0].xfit = array_init (fit[0].Nterms, fit[0].Nterms);
  fit[0].yfit = array_init (fit[0].Nterms, fit[0].Nterms);

  // sum[i][j] holds \sum (wt x1^i y1^j)
  fit[0].sum = array_init (fit[0].Nsums, fit[0].Nsums);

  return (fit);
}

void fit_free (CoordFit *fit) {

  array_free (fit[0].xfit, fit[0].Nterms);
  array_free (fit[0].yfit, fit[0].Nterms);

  array_free (fit[0].sum, fit[0].Nsums);
  array_free (fit[0].xsum, fit[0].Nterms);
  array_free (fit[0].ysum, fit[0].Nterms);
  
  free (fit);
}
  
int array_print (double **matrix, int Nx, int Ny) {

  int i, j;
  for (i = 0; i < Nx; i++) {
    for (j = 0; j < Ny; j++) {
      fprintf (stderr, "%10.3e ", matrix[i][j]);
    }
    fprintf (stderr, "\n");
  }
  return TRUE;
}

int fit_print (CoordFit *fit) {

  fprintf (stderr, "Npts: %d, Norder: %d, Nterms: %d, Nsums: %d, Nelems: %d\n",
	   fit[0].Npts, fit[0].Norder, fit[0].Nterms, fit[0].Nsums, fit[0].Nelems);

  fprintf (stderr, "sums: \n");
  array_print (fit[0].sum, fit[0].Nsums, fit[0].Nsums);

  fprintf (stderr, "xsums: \n");
  array_print (fit[0].xsum, fit[0].Nterms, fit[0].Nterms);

  fprintf (stderr, "ysums: \n");
  array_print (fit[0].ysum, fit[0].Nterms, fit[0].Nterms);

  fprintf (stderr, "xfits: \n");
  array_print (fit[0].xfit, fit[0].Nterms, fit[0].Nterms);

  fprintf (stderr, "yfits: \n");
  array_print (fit[0].yfit, fit[0].Nterms, fit[0].Nterms);
  
  return TRUE;
}

// XXX use implicit masks as below or explicit masks (with function to set?)
// XXX eg, add a global mask to this file and 
void fit_add (CoordFit *fit, double x1, double y1, double x2, double y2, double wt) {

  int ix, iy;
  double xterm, yterm, term;

  xterm = 1;
  for (ix = 0; ix < fit[0].Nsums; ix++) {
    yterm = 1;
    for (iy = 0; iy < fit[0].Nsums; iy++) {
      term = xterm*yterm*wt;
      fit[0].sum[ix][iy] += term;
      if ((iy < fit[0].Nterms) && (ix < fit[0].Nterms)) {
	fit[0].xsum[ix][iy] += x2*term;
	fit[0].ysum[ix][iy] += y2*term;
      }
      yterm *= y1;
    }
    xterm *= x1;
  }
  fit[0].Npts ++;
}

/* convert the xsum,ysum,sum terms into vector,matrix and solve */
int fit_eval (CoordFit *fit) {

  int i, j, ix, iy, jx, jy;
  double **matrix, **vector;

  if (fit[0].Npts == 0) {
    fprintf (stderr, "warning: no valid pts\n");
    return (FALSE);
  }

  // matrix, vector hold the final linear system
  matrix = array_init (fit[0].Nelems, fit[0].Nelems);
  vector = array_init (fit[0].Nelems, 2);

  /* remap the xsum,ysum terms into the vector */
  for (i = 0; i < fit[0].Nelems; i++) {
    ix = i % fit[0].Nterms;
    iy = i / fit[0].Nterms;
    vector[i][0] = fit[0].xsum[ix][iy];
    vector[i][1] = fit[0].ysum[ix][iy];

    for (j = 0; j < fit[0].Nelems; j++) {
      jx = j % fit[0].Nterms;
      jy = j / fit[0].Nterms;
      matrix[i][j] = fit[0].sum[ix+jx][iy+jy];
    }

    // mask the terms not represented by the Coords terms
    if (ix + iy > fit[0].Norder) {
      for (j = 0; j < fit[0].Nelems; j++) {
	matrix[i][j] = 0.0;
	matrix[j][i] = 0.0;
      }
      vector[i][0] = 0.0;
      vector[i][1] = 0.0;
      matrix[i][i] = 1.0;
    }      
  }

  for (i = 0; FALSE && (i < fit[0].Nelems); i++) {
    ix = i % fit[0].Nterms;
    iy = i / fit[0].Nterms;
    fprintf (stderr, "x2 : x^%dy^%d: %10.4g    y2 : x^%dy^%d: %10.4g \n", 
    ix, iy, vector[i][0], ix, iy, vector[i][1]);
  }	
  
  if (!dgaussjordan_pivot (matrix, vector, fit[0].Nelems, 2, 1e-16)) {
    array_free (matrix, fit[0].Nelems);
    array_free (vector, fit[0].Nelems);
    return (FALSE);
  }

  for (i = 0; FALSE && i < fit[0].Nelems; i++) {
    ix = i % fit[0].Nterms;
    iy = i / fit[0].Nterms;
    fprintf (stderr, "x2 : x^%dy^%d: %10.4g    y2 : x^%dy^%d: %10.4g \n", 
    ix, iy, vector[i][0], ix, iy, vector[i][1]);
  }	

  /* remap the vector terms into xfit,yfit */
  for (i = 0; i < fit[0].Nelems; i++) {
    ix = i % fit[0].Nterms;
    iy = i / fit[0].Nterms;
    fit[0].xfit[ix][iy] = vector[i][0];
    fit[0].yfit[ix][iy] = vector[i][1];
  }

  array_free (matrix, fit[0].Nelems);
  array_free (vector, fit[0].Nelems);
  return (TRUE);
}

void fit_apply (CoordFit *fit, double *x2, double *y2, double x1, double y1) {
  
  int ix, iy;
  double xterm, yterm, term;
  double x, y;

  x = 0.0;
  y = 0.0;

  xterm = 1;
  for (ix = 0; ix < fit[0].Nterms; ix++) {
    yterm = 1;
    for (iy = 0; iy < fit[0].Nterms; iy++) {
      term = xterm*yterm;
      x += fit[0].xfit[ix][iy]*term;
      y += fit[0].yfit[ix][iy]*term;
      yterm *= y1;
    }
    xterm *= x1;
  }
  *x2 = x;
  *y2 = y;
}

// Nx, Ny is the number of terms in x and in y
double **poly2d_dx (double **poly, int Nx, int Ny) {

  int i, j, Nxout, Nyout;
  double **dpoly;

  Nxout = Nx - 1;
  Nyout = Ny;

  // poly[i][j] holds coeff for x1^i y1^j
  dpoly = array_init (Nxout, Nyout);

  for (i = 0; i < Nxout; i++) {
    for (j = 0; j < Nyout; j++) {
      dpoly[i][j] = poly[i+1][j] * (i+1);
    }
  }
  return dpoly;
}

// Nx, Ny is the number of terms in x and in y
double **poly2d_dy (double **poly, int Nx, int Ny) {

  int i, j, Nxout, Nyout;
  double **dpoly;

  Nxout = Nx;
  Nyout = Ny - 1;

  // poly[i][j] holds coeff for x1^i y1^j
  dpoly = array_init (Nxout, Nyout);

  for (i = 0; i < Nxout; i++) {
    for (j = 0; j < Nyout; j++) {
      dpoly[i][j] = poly[i][j+1] * (j+1);
    }
  }
  return dpoly;
}

// Nx, Ny is the number of terms in x and in y
double **poly2d_copy (double **poly, int Nx, int Ny) {

  int i, j;
  double **out;

  // poly[i][j] holds coeff for x1^i y1^j
  out = array_init (Nx, Ny);

  for (i = 0; i < Nx; i++) {
    for (j = 0; j < Ny; j++) {
      out[i][j] = poly[i][j];
    }
  }
  return out;
}

double poly2d_eval (double **poly, int Nx, int Ny, double x, double y) {

  int i, j;
  double xsum, ysum, sum;

  sum = 0;
  xsum = ysum = 1.0;

  for (i = 0; i < Nx; i++) {
    ysum = xsum;
    for (j = 0; j < Ny; j++) {
      sum += poly[i][j] * ysum;
      ysum *= y;
    }
    xsum *= x;
  }
  return (sum);
}

/* linear portion of fit : NORDER is 1 */
/* this should only apply to the polynomial, not the projection terms */
/* compare with psastro supporting code */
// this code will work for linear (Npolyterm == 0 or 1) and linear + map fits
int fit_apply_coords (CoordFit *fit, Coords *coords, int keepRef) {

  double Xo, Yo, R1, R2;
  CoordFit *modfit;

  if (keepRef) {
    // adjust crpix1,2 as needed:
    // L = a[0][0] + a[1][0]x^1 y^0 + a[0][1] x^0 y^1 + ...
    // L = pc1_1*cd1*(x - cp1) + pc1_2*cd2*(y - cp2) + ...

    if (!CoordsGetCenter (fit, 0.001, &Xo, &Yo)) {
      fprintf (stderr, "failed to modify model\n");
      return (FALSE);
    }
    coords[0].crpix1 = Xo;
    coords[0].crpix2 = Yo;
    /* we do not modify crval1,2: these are kept at the default values */
  
    // resulting fit should have zero constant terms
    modfit = CoordsSetCenter (fit, Xo, Yo);
  } else {
    // L = a[0][0] + a[1][0]x^1 y^0 + a[0][1] x^0 y^1 + ...
    // P = cd1*x, Q = cd2*y
    // L = pc1_1*P + pc1_2*Q + ...

    /* modify crval1,2: these are kept at the default values */

    coords[0].crpix1 = 0.0;
    coords[0].crpix2 = 0.0;
    modfit = fit;
  }

  // set cdelt1, cdelt2
  coords[0].cdelt1 = hypot (modfit[0].xfit[1][0], modfit[0].xfit[0][1]);
  coords[0].cdelt2 = hypot (modfit[0].yfit[1][0], modfit[0].yfit[0][1]);
  R1 = 1 / coords[0].cdelt1;
  R2 = 1 / coords[0].cdelt2;

  // set pc1_1, pc1_2, pc2_1, pc2_2 (cd1,cd2 = 1.0)
  coords[0].pc1_1 = modfit[0].xfit[1][0] * R1;
  coords[0].pc1_2 = modfit[0].xfit[0][1] * R2;
  coords[0].pc2_1 = modfit[0].yfit[1][0] * R1;
  coords[0].pc2_2 = modfit[0].yfit[0][1] * R2;

  // set the polyterm elements 
  if (coords->Npolyterms > 1) {
    coords[0].polyterms[0][0] = modfit[0].xfit[2][0]*R1*R1;
    coords[0].polyterms[1][0] = modfit[0].xfit[1][1]*R1*R2;
    coords[0].polyterms[2][0] = modfit[0].xfit[0][2]*R2*R2;

    coords[0].polyterms[0][1] = modfit[0].yfit[2][0]*R1*R1;
    coords[0].polyterms[1][1] = modfit[0].yfit[1][1]*R1*R2;
    coords[0].polyterms[2][1] = modfit[0].yfit[0][2]*R2*R2;
  }

  // I need to validate Norder
  if (coords->Npolyterms > 2) {
    coords[0].polyterms[3][0] = modfit[0].xfit[3][0]*R1*R1*R1;
    coords[0].polyterms[4][0] = modfit[0].xfit[2][1]*R1*R1*R2;
    coords[0].polyterms[5][0] = modfit[0].xfit[1][2]*R1*R2*R2;
    coords[0].polyterms[6][0] = modfit[0].xfit[0][3]*R2*R2*R2;

    coords[0].polyterms[3][1] = modfit[0].yfit[3][0]*R1*R1*R1;
    coords[0].polyterms[4][1] = modfit[0].yfit[2][1]*R1*R1*R2;
    coords[0].polyterms[5][1] = modfit[0].yfit[1][2]*R1*R2*R2;
    coords[0].polyterms[6][1] = modfit[0].yfit[0][3]*R2*R2*R2;
  }
  
  if (keepRef) {
    fit_free (modfit);
  } else {
    fit_apply (fit, &coords[0].crval1, &coords[0].crval2, coords[0].crpix1, coords[0].crpix2); 
  }
  /* keep the order and type from initial values */
  
  if (isnan(coords[0].crval1)) {
    return FALSE;
  }
  if (isnan(coords[0].crval2)) {
    return FALSE;
  }
  if (isnan(coords[0].crpix1)) {
    return FALSE;
  }
  if (isnan(coords[0].crpix2)) {
    return FALSE;
  }

  return (TRUE);
}

