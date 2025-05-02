# include "relastro.h"

double factorial (int N) {

  int i;
  double F;

  F = 1;
  for (i = N; i > 1; i--) {
    F *= i;
  }
  return F;
}

// XXX use a separate structure for the map (x2,y2 = f,g(x1,y1)) ?

// given a 2D transformation -- L(x,y),M(x,y) -- find the coordinates x,y
// for which L,M = 0,0. tol is the allowed error on x,y.
int CoordsGetCenter (CoordFit *fit, double tol, double *xo, double *yo) {

  int i, Nx, Ny;
  double R, Xo, Yo, dPos, dPosRef;
  double **XdX, **XdY, **YdX, **YdY, **alpha, **beta;
  double **xfit, **yfit;

  xfit = fit[0].xfit;
  yfit = fit[0].yfit;

  // solve for Xo,Yo where L,M(Xo,Yo) = 0,0
  // start with linear solution for Xo,Yo:
  R  = (xfit[1][0]*yfit[0][1] - xfit[0][1]*yfit[1][0]);
  Xo = (yfit[0][0]*xfit[0][1] - xfit[0][0]*yfit[0][1])/R;
  Yo = (xfit[0][0]*yfit[1][0] - yfit[0][0]*xfit[1][0])/R;

  Nx = fit[0].Nterms;
  Ny = fit[0].Nterms;

  // iterate to actual solution: requires small non-linear terms
  if (fit[0].Norder > 1) {
    XdX = poly2d_dx (xfit, Nx, Ny);
    XdY = poly2d_dy (xfit, Nx, Ny);
    YdX = poly2d_dx (yfit, Nx, Ny);
    YdY = poly2d_dy (yfit, Nx, Ny);

    alpha = array_init (2, 2);
    beta  = array_init (2, 1);

    /* this loop uses the Newton-Raphson method to solve for Xo,Yo
     * it needs the high order terms to be small 
     * Xo,Yo are in pixels;
     */
    dPos = tol + 1;
    dPosRef = dPos;
    for (i = 0; (dPos > tol) && (i < 20); i++) {
      // NOTE: order for alpha is: [y][x]
      alpha[0][0] = poly2d_eval (XdX, Nx-1, Ny,   Xo, Yo);
      alpha[1][0] = poly2d_eval (XdY, Nx,   Ny-1, Xo, Yo);
      alpha[0][1] = poly2d_eval (YdX, Nx-1, Ny,   Xo, Yo);
      alpha[1][1] = poly2d_eval (YdY, Nx,   Ny-1, Xo, Yo);

      beta[0][0] = poly2d_eval (xfit, Nx, Ny, Xo, Yo);
      beta[1][0] = poly2d_eval (yfit, Nx, Ny, Xo, Yo);

      dgaussjordan (alpha, beta, 2, 1);

      Xo -= beta[0][0];
      Yo -= beta[1][0];

      dPos = hypot(beta[0][0], beta[1][0]);
      if (i == 0) {
	dPosRef = dPos;
      }
    }
    if (dPos > dPosRef) {
      fprintf (stderr, "*** warning : non-convergence in model conversion (mkpolyterm.c;73) *** \n");
      return FALSE;
    }
    array_free (alpha, 2);
    array_free (beta, 2);
    array_free (XdX, Nx - 1);
    array_free (XdY, Nx);
    array_free (YdX, Nx - 1);
    array_free (YdY, Nx);
  }
  *xo = Xo;
  *yo = Yo;
  return TRUE;
}

// convert a transformation L(x,y) to L'(x-xo,y-yo)
CoordFit *CoordsSetCenter (CoordFit *input, double Xo, double Yo) {

  int i, j, Nx, Ny;
  double **xPx, **yPx, **xPy, **yPy, **tmp;

  CoordFit *output;
  output = fit_init (input->Norder);

  /* given two equivalent polynomial representations L(x,y) = \sum_i \sum_j A_{i,j} x^i y^j
   * we can transform L(x,y) into L'(x-xo,y-yo) by taking the derivatives of both sides and 
   * noting that the constant term in each is the coefficient in the case of L(x,y) and is the 
   * value of L'(-xo,-yo) in the second case.
   */

  Nx = input->Nterms;
  Ny = input->Nterms;

  xPx = poly2d_copy (input->xfit, Nx, Ny);
  yPx = poly2d_copy (input->yfit, Nx, Ny);

  for (i = 0; i < input->Nterms; i++) {
    xPy = poly2d_copy (xPx, Nx, Ny);
    yPy = poly2d_copy (yPx, Nx, Ny);
    for (j = 0; j < input->Nterms; j++) {
      output->xfit[i][j] = poly2d_eval (xPy, Nx, Ny, Xo, Yo) / factorial(i) / factorial(j);
      output->yfit[i][j] = poly2d_eval (yPy, Nx, Ny, Xo, Yo) / factorial(i) / factorial(j);

      // take the next derivative wrt y, catch output (is NULL on last pass)
      if (Ny > 0) {
	tmp = poly2d_dy(xPy, Nx, Ny);
	array_free (xPy, Nx);
	xPy = tmp;

	tmp = poly2d_dy(yPy, Nx, Ny);
	array_free (yPy, Nx);
	yPy = tmp;
      } else {
	array_free (xPy, Nx);
	array_free (yPy, Nx);
      }

      Ny --;
    }
    Ny = input->Nterms;
    // take the next derivative wrt x, catch output (is NULL on last pass)
    if (Nx > 0) {
      tmp = poly2d_dx(xPx, Nx, Ny);
      array_free (xPx, Nx);
      xPx = tmp;
      tmp = poly2d_dx(yPx, Nx, Ny);
      array_free (yPx, Nx);
      yPx = tmp;
      Nx --;
    } else {
      array_free (xPx, Nx);
      array_free (yPx, Nx);
    }
  }
  return output;
}

/*
  Coords uses a rigid sequence for the coefficients:

  x^0 y^0 : crpix1,2 (but note this is applied before higher order terms)

  x^1 y^0 : pc1_1, pc2_1
  x^0 y^1 : pc1_2, pc2_2

  x^2 y^0 : polyterm[0][0,1]
  x^1 y^1 : polyterm[1][0,1]
  x^0 y^2 : polyterm[2][0,1]

  x^3 y^0 : polyterm[3][0,1]
  x^2 y^1 : polyterm[4][0,1]
  x^1 y^2 : polyterm[5][0,1]
  x^0 y^3 : polyterm[6][0,1]
*/
