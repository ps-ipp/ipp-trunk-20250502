# include "data.h"  /* only needed for the ALLOCATE def */

/* need to pass in a function of the form:
   funcs (x, a, Npar, dyda) 
   returns f (x) for Npar parameters a, also df/da at x 
   dy carries 1/sig^2 
*/

static opihi_flt **alpha, **talpha;
static opihi_flt **beta, **tbeta;
static opihi_flt *partry, *dyda;
static opihi_flt ochisq, lambda;

opihi_flt mrqcof (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, opihi_flt **ta, opihi_flt **tb, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *)) {

  int k, j, i;
  opihi_flt ymodel, ydiff, wt, chisq;

  for (j = 0; j < Npar; j++) {
    for (k = 0; k <= j; k++) ta[j][k] = 0.0;
    tb[j][0] = 0.0;
  }

  chisq = 0.0;
  for (i = 0; i < Npts; i++) {

    ymodel = funcs (x[i], par, Npar, dyda);
    ydiff = ymodel - y[i];
    chisq += SQ(ydiff) * dy[i];

    // fprintf (stderr, "%f %f - %f : %f -> %f\n", x[i], y[i], ymodel, dy[i], chisq);

    for (j = 0; j < Npar; j++) {
      wt = dyda[j] * dy[i];
      for (k = 0; k <= j; k++) ta[j][k] += wt * dyda[k];
      tb[j][0] += wt * ydiff;
    }
  }

  for (j = 1; j < Npar; j++)
    for (k = 0; k < j; k++) 
      ta[k][j] = ta[j][k];

  return (chisq);

}

opihi_flt mrqmin (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE) {

  int j, k;
  opihi_flt chisq;
  opihi_flt rho, dX, dL;

  /* set up test matrixes for this run */
  for (j = 0; j < Npar; j++) {
    for (k = 0; k < Npar; k++) talpha[j][k] = alpha[j][k];
    talpha[j][j] = alpha[j][j] * (1.0 + lambda);
    tbeta[j][0] = beta[j][0];
  }

  dgaussjordan (talpha, tbeta, Npar, 1);

  for (j = 0; j < Npar; j++) partry[j] = par[j] - tbeta[j][0];

  /* get linear model prediction */
  dL = 0;
  for (j = 0; j < Npar; j++) {
      dL += 0.5*lambda*SQ(tbeta[j][0]) + tbeta[j][0]*beta[j][0];
  }

  chisq = mrqcof (x, y, dy, Npts, partry, Npar, talpha, tbeta, funcs);
  if (VERBOSE) { 
    gprint (GP_ERR, "chisq: %f  ", chisq);
    gprint (GP_ERR, "lambda: %f  ", lambda);
    for (j = 0; j < Npar; j++) {
      gprint (GP_ERR, "%f ", partry[j]);
    }
    gprint (GP_ERR, "\n");
  }

  /* compare linear model with actual */
  dX = ochisq - chisq;
  rho = dX / dL;

  /* if good, save temp values */
  if ((chisq > 1e-3) && (rho > -1e-6)) {
    lambda *= 0.1;
    ochisq = chisq;
    for (j = 0; j < Npar; j++) {
      for (k = 0; k < Npar; k++) alpha[j][k] = talpha[j][k];
      beta[j][0] = tbeta[j][0];
      par[j] = partry[j];
    }
  } else {
    lambda *= 10.0;
    chisq = ochisq;
  }

  return (chisq);

}

opihi_flt mrqinit (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE) {

  int i;

  ALLOCATE (dyda, opihi_flt, Npar);
  ALLOCATE (partry, opihi_flt, Npar);
  ALLOCATE (alpha, opihi_flt *, Npar);
  ALLOCATE (beta, opihi_flt *, Npar);
  ALLOCATE (talpha, opihi_flt *, Npar);
  ALLOCATE (tbeta, opihi_flt *, Npar);
  for (i = 0; i < Npar; i++) {
    ALLOCATE (alpha[i], opihi_flt, Npar);
    ALLOCATE (beta[i], opihi_flt, Npar);
    ALLOCATE (talpha[i], opihi_flt, Npar);
    ALLOCATE (tbeta[i], opihi_flt, Npar);
  }
  
  lambda = 0.01;
  
  ochisq = mrqcof (x, y, dy, Npts, par, Npar, alpha, beta, funcs);
  if (VERBOSE) {
    gprint (GP_ERR, "chisq: %f  ", ochisq);
    gprint (GP_ERR, "lambda: %f  ", lambda);
    for (i = 0; i < Npar; i++) {
      gprint (GP_ERR, "%f ", par[i]);
    }
    gprint (GP_ERR, "\n");
  }
  return (ochisq);
}

/* don't invoke this in the middle of a run, only near the end */ 
opihi_flt **mrqcovar (int Npar) {
  dgaussjordan (alpha, beta, Npar, 1);
  return (alpha);
} 

void mrqfree (int Npar) {

  int i;

  for (i = 0; i < Npar; i++) {
    free (alpha[i]);
    free (talpha[i]);
    free (beta[i]);
    free (tbeta[i]);
  }
  free (alpha);
  free (talpha);
  free (beta);
  free (tbeta);
  free (partry);
  free (dyda);
}
