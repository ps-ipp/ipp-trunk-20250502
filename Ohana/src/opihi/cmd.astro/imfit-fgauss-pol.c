# include "imfit.h"

/** fgaussPol : a real 2D Gaussian **/

opihi_flt fgaussPolTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void      fgaussPolCL ();

void fgauss_pol_setup (char *name) {

  if (strcmp(name, "fgauss-pol")) return;

  fitfunc = fgaussPolTD;
  imfit_cleanup = fgaussPolCL;
  Npar = 7;
  Nfpar = 0;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[5] = get_variable_default ("Zpk", 10000);
  par[6] = get_variable_default ("Sg", 0.0);

  opihi_flt Sxx = get_variable_default ("SXg", 2.0) / 2.35; // convert FWHM to sigma / sqrt(2) (Sxx)
  opihi_flt Syy = get_variable_default ("SYg", 2.0) / 2.35; // convert FWHM to sigma / sqrt(2) (Syy)
  opihi_flt Sxy = get_variable_default ("SXYg", 0);

  // z = (x^2 + y^2) / R^2 + (x^2 - y^2) / T^2 + x y / Q : NOTE Q is not squared to allow positive and negative values

  par[2] = 2.0/sqrt((1.0/SQ(Sxx) + 1.0/SQ(Syy))); // par[2] = R
  par[3] = 2.0/sqrt((1.0/SQ(Sxx) - 1.0/SQ(Syy))); // par[3] = T
  par[4] = 1.0 / Sxy;				  // par[4] = Q

  sky = &par[6];
}

void fgaussPolCL () {
  opihi_flt Sxx = par[2]*par[3] / (2.0*sqrt(SQ(par[3]) + SQ(par[2])));
  opihi_flt Syy = par[2]*par[3] / (2.0*sqrt(SQ(par[3]) - SQ(par[2])));
  opihi_flt Sxy = 1.0 / par[4];

  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  Sxx * 2.35 / sqrt(2.0));
  set_variable ("SYg",  Syy * 2.35 / sqrt(2.0));
  set_variable ("SXYg", Sxy);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
}

/* real 2D gaussian -- x, y, sx, sy, sxy, I, sky */
opihi_flt fgaussPolTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X = x - par[0];
  opihi_flt Y = y - par[1];
  
  opihi_flt P_R = (SQ(X) + SQ(Y)) / SQ(par[2]);
  opihi_flt P_T = (SQ(X) - SQ(Y)) / SQ(par[3]);
  opihi_flt P_Q = X * Y / par[4];

  opihi_flt z = P_R + P_T + P_Q;

  opihi_flt r = exp (-z);
  opihi_flt q = par[5]*r;
  opihi_flt f = q + par[6];

  if (dpar != NULL) {
    dpar[0] = +q*(2*X/SQ(par[2]) + 2*X/SQ(par[3]) + Y/par[4]);
    dpar[1] = +q*(2*Y/SQ(par[2]) - 2*Y/SQ(par[3]) + X/par[4]);

    dpar[2] = 2*q*P_R/par[2];
    dpar[3] = 2*q*P_T/par[3];
    dpar[4] =   q*P_Q/par[4];

    dpar[5] = +r;
    dpar[6] = +1;
  }
  return (f);
}
