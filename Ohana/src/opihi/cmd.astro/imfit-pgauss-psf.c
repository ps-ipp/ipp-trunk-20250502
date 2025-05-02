# include "imfit.h"

opihi_flt pgauss_psfTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  pgauss_psfCL ();

void  pgauss_psf_setup (char *name) {

  if (strcmp(name, "pgauss_psf")) return;

  fitfunc = pgauss_psfTD;
  imfit_cleanup = pgauss_psfCL;
  Npar = 4;
  Nfpar = 3;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[2] = get_variable_default ("Zpk", 10000);
  par[3] = get_variable_default ("Sg", 0.0);
  sky = &par[3];

  fpar[0] = 2.35 / get_variable_default ("SXg", 2.0);
  fpar[1] = 2.35 / get_variable_default ("SYg", 2.0);
  fpar[2] = get_variable_default ("SXYg", 0);
}

void pgauss_psfCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("Zpk",  par[2]);
  set_variable ("Sg",   par[3]);
}

/* pseudo 2D gaussian -- x, y, (sx), (sy), (sxy), I, sky */
opihi_flt pgauss_psfTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px, py;
  opihi_flt z, r, q, f;

  /* par -> fpar: (2,0), (3,1), (4,2) */

  X = x - par[0];
  Y = y - par[1];
  
  px = fpar[0]*X;
  py = fpar[1]*Y;

  z = 0.5*SQ(px) + 0.5*SQ(py) + fpar[2]*X*Y;
  r = 1.0 / (1 + z + 0.5*z*z*(1 + z/3)); /* ~ exp (-Z) */
  f = par[2]*r + par[3];
  q = par[2]*r*r*(1 + z + 0.5*z*z);
  /* note difference from gaussian: q = par[5]*r */

  if (dpar != NULL) {
    dpar[0] = q*(2*px*fpar[0] + fpar[2]*Y);
    dpar[1] = q*(2*py*fpar[1] + fpar[2]*X);
    dpar[2] = +r;
    dpar[3] = +1;
  }
  return (f);
}
