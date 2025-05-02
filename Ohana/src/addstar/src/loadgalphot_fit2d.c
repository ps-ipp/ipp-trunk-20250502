# include "addstar.h"
# include "loadgalphot.h"
# define VERY_VERBOSE 1

Fit2D *fit2d_init (int order) {

  int i;

  Fit2D *fit;
  ALLOCATE_ZERO (fit, Fit2D, 1);

  // allocate static arrays
  fit->order = order;
  fit->nterm = order + 1;
  fit->wterm = fit->nterm*(fit->nterm + 1)/2;
  fit->mterm = 2*order + 1;

  fit->ClipNiter = 1;
  fit->ClipNsigma = 0.0;

  /* allocate the summation matrices */
  ALLOCATE (fit->s, double *, fit->mterm);
  ALLOCATE (fit->b, double *, fit->wterm);
  ALLOCATE (fit->B, double,   fit->wterm);
  ALLOCATE (fit->c, double *, fit->wterm);
  ALLOCATE (fit->C, double *, fit->wterm);
  for (i = 0; i < fit->wterm; i++) {
    ALLOCATE_ZERO (fit->c[i], double, fit->wterm);
    ALLOCATE_ZERO (fit->C[i], double, fit->wterm);
    ALLOCATE_ZERO (fit->b[i], double, 1);
  }
  for (i = 0; i < fit->mterm; i++) {
    ALLOCATE_ZERO (fit->s[i], double, fit->mterm);
  }
  ALLOCATE_ZERO (fit->Cii, double, fit->wterm);
  return fit;
}

int fit2d_reset (Fit2D *fit) {

  int i;

  for (i = 0; i < fit->wterm; i++) {
    memset (fit->c[i], 0, fit->wterm*sizeof(double));
    memset (fit->b[i], 0, sizeof(double));
  }
  for (i = 0; i < fit->mterm; i++) {
    memset (fit->s[i], 0, fit->mterm*sizeof(double));
  }
  fit->c00 = 0.0;
  fit->c10 = 0.0;
  fit->c20 = 0.0;
  fit->c01 = 0.0;
  fit->c11 = 0.0;
  fit->c02 = 0.0;

  fit->mean = 0.0;
  fit->sigma = 0.0;
  return TRUE;
}

void fit2d_free (Fit2D *fit) {

  int i;

  for (i = 0; i < fit->wterm; i++) {
    free (fit->C[i]);
    free (fit->c[i]);
    free (fit->b[i]);
  }
  free (fit->b);
  free (fit->B);
  free (fit->c);
  free (fit->C);

  for (i = 0; i < fit->mterm; i++) {
    free (fit->s[i]);
  }
  free (fit->s);
  free (fit);
}

// unweighted 2D 2nd order fit (6 free parameters)
int fit2d (Fit2D *fit, float *xval, float *yval, float *zval, float *zfit, char *mask, int Npts) {
  
  int N, i, j, nx, ny, K, k, n, Npt, status;

  double mean = 0.0;
  double sigma = 0.0;
  double maxsigma = 0.0;

  float *x, *y, *z, *zf;

  for (N = 0; N < fit->ClipNiter; N++) {
    fit2d_reset (fit);

    // pointers which loop over datapoints
    x = xval;
    y = yval;
    z = zval;

    /* add up the x,y values */
    for (i = 0; i < Npts; i++, x++, y++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y) || !finite(*z)) continue;
      double Y = 1.0;
      for (ny = 0; ny < fit->mterm; ny++) {
	double X = Y;
	for (nx = 0; nx < fit->mterm - ny; nx++) {
	  fit->s[nx][ny] += X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    // pointers which loop over datapoints
    x = xval;
    y = yval;
    z = zval;

    /* add up the z values */
    for (i = 0; i < Npts; i++, x++, y++, z++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y) || !finite(*z)) continue;
      double Y = *z;
      for (j = 0, ny = 0; ny < fit->nterm; ny++) {
	double X = Y;
	for (nx = 0; nx < fit->nterm - ny; nx++, j++) {
	  fit->b[j][0] += X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    /* re-sort mterm x mterm matrix to wterm matrix */
    for (k = j = 0; j < fit->nterm; j++) {
      for (i = 0; i < fit->nterm - j; i++, k++) {
	for (K = ny = 0; ny < fit->nterm; ny++) {
	  for (nx = 0; nx < fit->nterm - ny; nx++, K++) {
	    fit->c[K][k] = fit->s[nx+i][ny+j];
	  }
	}
      }
    }

# if (VERY_VERBOSE)
    for (i = 0; i < fit->wterm; i++) {
      for (j = 0; j < fit->wterm; j++) {
	fit->C[i][j] = fit->c[i][j];
      }
      fit->B[i] = fit->b[i][0];
    }
# endif

    // invert the c,b matrix equation
    status = dgaussjordan (fit->c, fit->b, fit->wterm, 1);
    if (!status) {
# if (VERY_VERBOSE)
      for (i = 0; i < fit->wterm; i++) {
	for (j = 0; j < fit->wterm; j++) {
	  fprintf (stderr, "%10.3e ", fit->C[i][j]);
	}
	fprintf (stderr, " : %10.3e\n", fit->B[i]);
      }
# endif
      return FALSE;
    }

    /* the b[][0] terms are in the following order:
       y^0 x^0, y^0 x^1, ... y^0 x^N
       y^1 x^0, y^1 x^1, ... y^1 x^N
       ...
       y^N x^0, y^N x^1, ... y^N x^N
    */

    /* generate fitted values */
    x  = xval;
    y  = yval;
    zf = zfit;
    for (n = 0; n < Npts; n++, x++, y++, zf++) {
      if (!finite(*x) || !finite(*y) || !finite(*z)) continue;
      *zf = 0;
      double Y = 1;
      for (i = ny = 0; ny < fit->nterm; ny++) {
	double X = Y;
	for (nx = 0; nx < fit->nterm - ny; nx++, i++) {
	  *zf += fit->b[i][0]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    /* measure fit residual scatter */
    x  = xval;
    y  = yval;
    z  = zval;
    zf = zfit;
    float dZ  = 0.0;
    float dZ2 = 0.0;
    for (i = Npt = 0; i < Npts; i++, x++, y++, z++, zf++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y) || !finite(*z)) continue;
      float dZi = (*z - *zf);
      dZ  += dZi;
      dZ2 += SQ(dZi);
      Npt ++;
    }
    mean  = dZ / Npt;
    sigma = sqrt (fabs(dZ2/Npt - SQ(mean)));
    maxsigma = fit->ClipNsigma * sigma;

    if (VERBOSE) gprint (GP_ERR, "mean: %g, sigma: %g, maxsigma: %g\n", mean, sigma, maxsigma);

    /* mask outlier points */
    x  = xval;
    y  = yval;
    z  = zval;
    zf = zfit;
    int Nmask = 0;
    for (i = 0; fabs(fit->ClipNsigma) > 0.001 && (i < Npts); i++, x++, y++, z++, zf++) {
      float dZi = (*z - *zf);
      if (fabs(dZi) > maxsigma) {
	mask[i] = TRUE;
	Nmask ++;
      } else {
	mask[i] = FALSE;
      }	
    }
    if (VERBOSE) {
      gprint (GP_ERR, "pass: %d, Nmask: %d\n", N, Nmask);
    }
  }
    
  // save the fit
  for (i = 0; i < fit->wterm; i++) {
    fit->Cii[i] = fit->b[i][0];
  }
  switch (fit->order) {
    case 0:
      fit->c00 = fit->Cii[0];
      break;
    case 1:
      fit->c00 = fit->Cii[0];
      fit->c10 = fit->Cii[1];
      fit->c01 = fit->Cii[2];
      break;
    case 2:
      fit->c00 = fit->Cii[0];
      fit->c10 = fit->Cii[1];
      fit->c20 = fit->Cii[2];
      fit->c01 = fit->Cii[3];
      fit->c11 = fit->Cii[4];
      fit->c02 = fit->Cii[5];
      break;
    default:
      myAbort("invalid order");
  }

  fit->mean  = mean;
  fit->sigma = sigma;

  return (TRUE);
}
