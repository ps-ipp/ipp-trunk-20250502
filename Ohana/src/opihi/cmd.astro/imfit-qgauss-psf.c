# include "imfit.h"

opihi_flt qgauss_psfTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  qgauss_psfCL ();

void qgauss_psf_setup (char *name) {

  if (strcmp(name, "qgauss_psf")) return;

  fitfunc = qgauss_psfTD;
  imfit_cleanup = qgauss_psfCL;
  Npar = 4;
  Nfpar = 5;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0]  = get_variable_default ("Xg", 0);
  par[1]  = get_variable_default ("Yg", 0);
  par[2]  = get_variable_default ("Zpk", 10000);
  par[3]  = get_variable_default ("Sg", 0.0);

  fpar[0] = 2.35 / get_variable_default ("SXg", 15.0);
  fpar[1] = 2.35 / get_variable_default ("SYg", 15.0);
  fpar[2] = get_variable_default ("SXYg", 0.0);
  fpar[3] = get_variable_default ("Sr", 1.0);
  fpar[4] = get_variable_default ("Npow", 2.25);

  sky = &par[3];
}

void qgauss_psfCL () {
  set_variable ("Xg",  par[0]);
  set_variable ("Yg",  par[1]);
  set_variable ("Zpk", par[2]);
  set_variable ("Sg",  par[3]);
}

/* one component, two slopes: (1 + z^M + z^N)^(-1) -- x, y, sx, sy, sxy, I, sky, sr */
opihi_flt qgauss_psfTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px, py;
  opihi_flt z, r, q, f;

  X = x - par[0];
  Y = y - par[1];
  
  px = fpar[0]*X;
  py = fpar[1]*Y;

  z = 0.5*SQ(px) + 0.5*SQ(py) + fpar[2]*X*Y;

  r = 1.0 / (1 + fpar[3]*z + pow(z,fpar[4]));
  f = par[2]*r + par[3];
  q = par[2]*SQ(r)*(fpar[3] + fpar[4]*pow(z,(fpar[4]-1)));

  if (dpar != NULL) {
    dpar[0] = q*(2*px*fpar[0] + fpar[2]*Y);
    dpar[1] = q*(2*py*fpar[1] + fpar[2]*X);
    dpar[2] = +r;
    dpar[3] = +1;
  }
  return (f);
}

