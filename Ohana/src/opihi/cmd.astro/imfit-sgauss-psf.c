# include "imfit.h"

opihi_flt sgauss_psfTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  sgauss_psfCL ();

void sgauss_psf_setup (char *name) {

  if (strcmp(name, "sgauss_psf")) return;

  fitfunc = sgauss_psfTD;
  imfit_cleanup = sgauss_psfCL;
  Npar = 4;
  Nfpar = 7;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[2] = get_variable_default ("Zpk", 10000);
  par[3] = get_variable_default ("Sg", 0.0);

  fpar[0] = 2.35 / get_variable_default ("SXg", 15.0);
  fpar[1] = 2.35 / get_variable_default ("SYg", 15.0);
  fpar[2] = get_variable_default ("SXYg", 0.0);
  fpar[3] = 2.35 / get_variable_default ("SXf", 15.0);
  fpar[4] = 2.35 / get_variable_default ("SYf", 15.0);
  fpar[5] = get_variable_default ("SXYf", 0.0);
  fpar[6] = get_variable_default ("Npow", 2.25);

  sky = &par[3];
}

void sgauss_psfCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("Zpk",  par[2]);
  set_variable ("Sg",   par[3]);
}

/* two components: (1 + z_1 + z_2^N)^(-1) -- x, y, sx1, sy1, sxy1, I, sky, sx2, sy2, sxy2 */
opihi_flt sgauss_psfTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px1, py1, px2, py2;
  opihi_flt z1, z2, r, q1, q2, f;

  X = x - par[0];
  Y = y - par[1];
  
  px1 = fpar[0]*X;
  py1 = fpar[1]*Y;
  px2 = fpar[3]*X;
  py2 = fpar[4]*Y;

  z1 = 0.5*SQ(px1) + 0.5*SQ(py1) + fpar[2]*X*Y;
  z2 = 0.5*SQ(px2) + 0.5*SQ(py2) + fpar[5]*X*Y;

  r = 1.0 / (1 + z1 + pow(z2,fpar[6]));
  f = par[2]*r + par[3];

  q1 = par[2]*SQ(r);
  q2 = par[2]*SQ(r)*fpar[6]*pow(z2,(fpar[6]-1));

  if (dpar != NULL) {
    dpar[0] = q1*(2*px1*fpar[0] + fpar[2]*Y) + q2*(2*px2*fpar[3] + fpar[5]*Y);
    dpar[1] = q1*(2*py1*fpar[1] + fpar[2]*X) + q2*(2*py2*fpar[4] + fpar[5]*X);
    dpar[2] = +r;
    dpar[3] = +1;
  }
  return (f);
}
