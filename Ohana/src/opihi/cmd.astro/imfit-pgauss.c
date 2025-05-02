# include "imfit.h"

opihi_flt pgaussTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  pgaussCL ();

void  pgauss_setup (char *name) {

  if (strcmp(name, "pgauss")) return;

  fitfunc = pgaussTD;
  imfit_cleanup = pgaussCL;
  Npar = 7;
  Nfpar = 0;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0.0);
  par[1] = get_variable_default ("Yg", 0.0);
  par[2] = 2.35 / get_variable_default ("SXg", 2.0);
  par[3] = 2.35 / get_variable_default ("SYg", 2.0);
  par[4] = get_variable_default ("SXYg", 0.0);
  par[5] = get_variable_default ("Zpk", 10000);
  par[6] = get_variable_default ("Sg", 0.0);
  sky = &par[6];
}

void pgaussCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  2.35 / par[2]);
  set_variable ("SYg",  2.35 / par[3]);
  set_variable ("SXYg", par[4]);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
}

/* pseudo 2D gaussian -- x, y, sx, sy, sxy, I, sky */
opihi_flt pgaussTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px, py;
  opihi_flt z, r, q, f;

  X = x - par[0];
  Y = y - par[1];
  
  px = par[2]*X;
  py = par[3]*Y;

  z = 0.5*SQ(px) + 0.5*SQ(py) + par[4]*X*Y;
  r = 1.0 / (1 + z + 0.5*z*z*(1 + z/3)); /* ~ exp (-Z) */
  f = par[5]*r + par[6];
  q = par[5]*r*r*(1 + z + 0.5*z*z);
  /* note difference from gaussian: q = par[5]*r */

  if (dpar != NULL) {
    dpar[0] = q*(2*px*par[2] + par[4]*Y);
    dpar[1] = q*(2*py*par[3] + par[4]*X);
    dpar[2] = -2*q*px*X;
    dpar[3] = -2*q*py*Y;
    dpar[4] = -q*X*Y;
    dpar[5] = +r;
    dpar[6] = +1;
  }
  return (f);
}
