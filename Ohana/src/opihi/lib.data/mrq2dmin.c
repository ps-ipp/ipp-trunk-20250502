# include "data.h"  /* only needed for the ALLOCATE def */

/* need to pass in a function of the form:
   funcs (x, t, a, Npar, dy/da) 
   returns y (x,t) for Npar parameters a along with dy/da at (x,t)
   dy carries 1/sig^2 
*/

# define VERY_VERBOSE 0

static opihi_flt **alpha, **talpha;
static opihi_flt **beta, **tbeta;
static opihi_flt *partry, *dyda;
static opihi_flt ochisq, lambda;

static opihi_flt *parmin = NULL;
static opihi_flt *parmax = NULL;

opihi_flt mrq2dcof (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, opihi_flt **ta, opihi_flt **tb, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *)) {

  int k, j, i;
  opihi_flt ydiff, wt, chisq;

  for (j = 0; j < Npar; j++) {
    for (k = 0; k <= j; k++) ta[j][k] = 0.0;
    tb[j][0] = 0.0;
  }

  chisq = 0.0;
  for (i = 0; i < Npts; i++) {

    ydiff = funcs (x[i], t[i], par, Npar, dyda) - y[i];
    chisq += SQ(ydiff) * dy[i];
    
    for (j = 0; j < Npar; j++) {
      wt = dyda[j] * dy[i];
      for (k = 0; k <= j; k++) ta[j][k] += wt * dyda[k];
      tb[j][0] += wt * ydiff;
    }
  }

  for (j = 1; j < Npar; j++)
    for (k = 0; k < j; k++)
      ta[k][j] = ta[j][k];
      
# if (VERY_VERBOSE)
  for (j = 0; j < Npar; j++) {
    for (k = 0; k < Npar; k++) {
      gprint (GP_ERR, "%9.3e  ", ta[j][k]);
    }
    gprint (GP_ERR, "    :   %9.3e  ", tb[j][0]);
    gprint (GP_ERR, "\n");
  }
# endif

  return (chisq);

}

opihi_flt mrq2dchi (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
		opihi_flt *par, int Npar, 
		opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *)) {

  int i;
  opihi_flt ydiff, chisq;

  chisq = 0.0;
  for (i = 0; i < Npts; i++) {
    ydiff = funcs (x[i], t[i], par, Npar, dyda) - y[i];
    chisq += SQ(ydiff) * dy[i];
  }
  return (chisq);
}

opihi_flt mrq2dmin (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE) {

  int j, k;
  opihi_flt chisq;

  /* set up test matrixes for this run */
  for (j = 0; j < Npar; j++) {
    for (k = 0; k < Npar; k++) talpha[j][k] = alpha[j][k];
    talpha[j][j] = alpha[j][j] * (1.0 + lambda);
    tbeta[j][0] = beta[j][0];
  }

  /* keep this test in here? */
  if (!dgaussjordan (talpha, tbeta, Npar, 1)) {
    lambda *= 10.0;
    return (ochisq);
  }

  for (j = 0; j < Npar; j++) {
    partry[j] = par[j] - tbeta[j][0];
    /*
    if (parmin != NULL) partry[j] = MAX (parmin[j], partry[j]);
    if (parmax != NULL) partry[j] = MIN (parmax[j], partry[j]);
    */
  }

  chisq = mrq2dcof (x, t, y, dy, Npts, partry, Npar, talpha, tbeta, funcs);
  if (VERBOSE) { 
    gprint (GP_ERR, "chisq: %f  ", chisq);
    gprint (GP_ERR, "lambda: %f  ", lambda);
    for (j = 0; j < Npar; j++) {
      gprint (GP_ERR, "%f ", partry[j]);
    }
    gprint (GP_ERR, "\n");
  }

  /* if good, save temp values */
  if (chisq < ochisq) {
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

int mrq2dlimits (opihi_flt *pmin, opihi_flt *pmax, int Npar) {

  int i;

  ALLOCATE (parmin, opihi_flt, Npar);
  ALLOCATE (parmax, opihi_flt, Npar);
  for (i = 0; i < Npar; i++) {
    parmin[i] = pmin[i];
    parmax[i] = pmax[i];
  }
  return (TRUE);
}

opihi_flt mrq2dinit (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE) {

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

  
//  lambda = 0.001;
  lambda = 1.0;
  
  ochisq = mrq2dcof (x, t, y, dy, Npts, par, Npar, alpha, beta, funcs);
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
opihi_flt **mrq2dcovar (int Npar) {

  dgaussjordan (alpha, beta, Npar, 1);
  return (alpha);

} 

void mrq2dfree (int Npar) {

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

  if (parmin != NULL) free (parmin);
  if (parmax != NULL) free (parmax);

}
