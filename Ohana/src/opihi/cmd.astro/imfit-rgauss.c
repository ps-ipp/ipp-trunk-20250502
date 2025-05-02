# include "imfit.h"

opihi_flt rgaussTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  rgaussCL ();

void rgauss_setup (char *name) {

  if (strcmp(name, "rgauss")) return;

  fitfunc = rgaussTD;
  imfit_cleanup = rgaussCL;
  Npar =  8;
  Nfpar = 0;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[2] = 2.35 / get_variable_default ("SXg", 2.0);
  par[3] = 2.35 / get_variable_default ("SYg", 2.0);
  par[4] = get_variable_default ("SXYg", 0);
  par[5] = get_variable_default ("Zpk", 10000);
  par[6] = get_variable_default ("Sg", 0.0); // sky
  par[7] = get_variable_default ("Sr", 2.0);

//  fpar[0] = get_variable_default ("Npow", 2.25);

  sky = &par[6];
}

void rgaussCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  2.35 / par[2]);
  set_variable ("SYg",  2.35 / par[3]);
  set_variable ("SXYg", par[4]);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
  set_variable ("Sr",   par[7]);
}

/* rgauss: (1 + z + z^alpha)^(-1) -- x, y, sx, sy, sxy, I, sky, sr */
opihi_flt rgaussTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X = x - par[0];
  opihi_flt Y = y - par[1];
  
  opihi_flt px = par[2]*X;
  opihi_flt py = par[3]*Y;

  opihi_flt z = 0.5*SQ(px) + 0.5*SQ(py) + par[4]*X*Y;

  opihi_flt p = pow(z,par[7] - 1.0);
  opihi_flt r = 1.0 / (1 + z + z*p);
  opihi_flt f = par[5]*r + par[6];

  if (dpar != NULL) {
    opihi_flt t = par[5]*SQ(r);
    opihi_flt q = t*(1 + par[7]*p);

    dpar[0] = +q*(par[2]*px + par[4]*Y);
    dpar[1] = +q*(par[3]*py + par[4]*X);
    dpar[2] = -q*px*X;
    dpar[3] = -q*py*Y;
    dpar[4] = -q*X*Y;
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

