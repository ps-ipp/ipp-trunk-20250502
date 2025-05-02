# include "imfit.h"

opihi_flt vgaussTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  vgaussCL ();

void  vgauss_setup (char *name) {

  if (strcmp(name, "vgauss")) return;

  fitfunc = vgaussTD;
  cleanup = vgaussCL;
  Npar = 9;
  Nfpar = 0;

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[2] = 2.35 * sqrt(2.0) / get_variable_default ("SXg", 2.0);
  par[3] = 2.35 * sqrt(2.0) / get_variable_default ("SYg", 2.0);
  par[4] = 0.0;
  par[5] = get_variable_default ("Zpk", 10000);
  par[6] = get_variable_default ("Sg", 0.0);
  par[7] = 1;
  par[8] = 1;
  sky = &par[6];
}

/* pseudo 2D gaussian with opihi_flting 2nd and 3rd order terms -- x, y, sx, sy, sxy, I, sky, f1, f2 */
opihi_flt vgaussTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px, py;
  opihi_flt z, r, q, f, k;

  X = x - par[0];
  Y = y - par[1];
  
  px = par[2]*X;
  py = par[3]*Y;

  z = 0.5*SQ(px) + 0.5*SQ(py) + par[4]*X*Y;
  k = 0.5*z*z*(1 + par[8]*z/3);
  r = 1.0 / (1 + z + par[7]*k); /* ~ exp (-Z) */
  f = par[5]*r + par[6];
  q = par[5]*r*r*(1 + par[7]*z*(1 + par[8]*z/2));
  /* note difference from gaussian: q = par[5]*r */

  if (dpar != NULL) {
    dpar[0] = q*(2*px*par[2] + par[4]*Y);
    dpar[1] = q*(2*py*par[3] + par[4]*X);
    dpar[2] = -2*q*px*X;
    dpar[3] = -2*q*py*Y;
    dpar[4] = -q*X*Y;
    dpar[5] = +r;
    dpar[6] = +1;
    dpar[7] = -100*par[5]*r*r*k;
    dpar[8] = -100*par[5]*r*r*par[7]*(z*z*z)/6;
  }
  return (f);
}

int vgaussCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  2.35 * sqrt(2.0) / par[2]);
  set_variable ("SYg",  2.35 * sqrt(2.0) / par[3]);
  set_variable ("SXYg", par[4]);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
  set_variable ("SXf", par[7]);
  set_variable ("SYf", par[8]);
}
