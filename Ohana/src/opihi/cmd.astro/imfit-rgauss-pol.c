# include "imfit.h"

opihi_flt rgaussPolTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void      rgaussPolCL ();

void rgauss_pol_setup (char *name) {

  if (strcmp(name, "rgauss-pol")) return;

  fitfunc = rgaussPolTD;
  imfit_cleanup = rgaussPolCL;
  Npar =  8;
  Nfpar = 0;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  // NOTE 1: SXg & SYg are FWHM, SXYg is the second moment calculated by starfuncs.c:get_aperture_stats
  // NOTE 2: this function (unlike imfit-rgauss.c) uses polarization parameters for the elliptical profile
  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[5] = get_variable_default ("Zpk", 10000);
  par[6] = get_variable_default ("Sg", 0.0); // sky
  par[7] = get_variable_default ("Sr", 2.0);

  opihi_flt Sxx = get_variable_default ("SXg", 2.0) * sqrt(2.0) / 2.35; // convert FWHM to sigma / sqrt(2) (Sxx)
  opihi_flt Syy = get_variable_default ("SYg", 2.0) * sqrt(2.0) / 2.35; // convert FWHM to sigma / sqrt(2) (Syy)
  opihi_flt Sxy = get_variable_default ("SXYg", 0);

  // z = (x^2 + y^2) / R^2 + (x^2 - y^2) / T^2 + x y / Q : NOTE Q is not squared to allow positive and negative values

  par[2] = 2.0/sqrt((1.0/SQ(Sxx) + 1.0/SQ(Syy))); // par[2] = R
  par[3] = 2.0/sqrt((1.0/SQ(Sxx) - 1.0/SQ(Syy))); // par[3] = T
  par[4] = 1.0 / Sxy;				  // par[4] = Q

  sky = &par[6];
}

void rgaussPolCL () {
  opihi_flt Sxx = par[2]*par[3] / sqrt(SQ(par[3]) + SQ(par[2]));
  opihi_flt Syy = par[2]*par[3] / sqrt(SQ(par[3]) - SQ(par[2]));
  opihi_flt Sxy = 1.0 / par[4];

  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  Sxx * 2.35 / sqrt(2.0));
  set_variable ("SYg",  Syy * 2.35 / sqrt(2.0));
  set_variable ("SXYg", Sxy);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
  set_variable ("Sr",   par[7]);
}

/* rgaussPol: (1 + z + z^alpha)^(-1) -- x, y, sx, sy, sxy, I, sky, sr */
opihi_flt rgaussPolTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X = x - par[0];
  opihi_flt Y = y - par[1];
  
  opihi_flt P_R = (SQ(X) + SQ(Y)) / SQ(par[2]);
  opihi_flt P_T = (SQ(X) - SQ(Y)) / SQ(par[3]);
  opihi_flt P_Q = X * Y / par[4];

  opihi_flt z = P_R + P_T + P_Q;

  opihi_flt p = pow(z,par[7] - 1.0);
  opihi_flt r = 1.0 / (1 + z + z*p);
  opihi_flt f = par[5]*r + par[6];

  if (dpar != NULL) {
    opihi_flt t = par[5]*SQ(r);
    opihi_flt q = t*(1 + par[7]*p);

    dpar[0] = +q*(2*X/SQ(par[2]) + 2*X/SQ(par[3]) + Y/par[4]);
    dpar[1] = +q*(2*Y/SQ(par[2]) - 2*Y/SQ(par[3]) + X/par[4]);

    dpar[2] = 2*q*P_R/par[2];
    dpar[3] = 2*q*P_T/par[3];
    dpar[4] =   q*P_Q/par[4];

    dpar[5] = +r;
    dpar[6] = +1;

    // this model derivative is undefined at z = 0.0, but the limit is zero as z -> 0.0
    dpar[7] = (z == 0.0) ? 0.0 : -t*log(z)*p*z;
  }
  return (f);
}

/******************************************************************************
 * this file defines the RGAUSS source shape model (XXX need a better name!).  Note that these
 * model functions are loaded by pmModelClass.c using 'include', and thus need no 'include'
 * statements of their own.  The models use a psVector to represent the set of parameters, with
 * the sequence used to specify the meaning of the parameter.  The meaning of the parameters
 * may thus vary depending on the specifics of the model.  All models which are used as a PSF
 * representations share a few parameters, for which # define names are listed in pmModel.h:

   power-law with fitted slope
   1 / (1 + z + z^alpha)

 * PM_PAR_SKY 0   - local sky : note that this is unused and may be dropped in the future
 * PM_PAR_I0 1    - central intensity
 * PM_PAR_XPOS 2  - X center of object
 * PM_PAR_YPOS 3  - Y center of object
 * PM_PAR_SXX 4   - X^2 term of elliptical contour (sqrt(2) / SigmaX)
 * PM_PAR_SYY 5   - Y^2 term of elliptical contour (sqrt(2) / SigmaY)
 * PM_PAR_SXY 6   - X*Y term of elliptical contour
 * PM_PAR_7   7   - power-law slope (alpha)
 *****************************************************************************/

