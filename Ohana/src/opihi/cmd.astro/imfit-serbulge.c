# include "imfit.h"

opihi_flt serbulgeTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  serbulgeCL ();

void serbulge_setup (char *name) {

  if (strcmp(name, "serbulge")) return;

  fitfunc = serbulgeTD;
  cleanup = serbulgeCL;
  Npar = 12;
  Nfpar = 0;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[0] = get_variable_default ("Xg", 0);
  par[1] = get_variable_default ("Yg", 0);
  par[2] = 2.35 * sqrt(2.0) / get_variable_default ("SXg", 2.0);
  par[3] = 2.35 * sqrt(2.0) / get_variable_default ("SYg", 2.0);
  par[4] = 0.0;
  par[5] = get_variable_default ("Zpk", 10000) / 2.0;

  par[6] = get_variable_default ("Sg", 0.0);

  par[7] = 2.35 * sqrt(2.0) / get_variable_default ("SXf", 15.0);
  par[8] = 2.35 * sqrt(2.0) / get_variable_default ("SYf", 15.0);
  par[9] = get_variable_default ("SXYf", 0.0);
  par[10] = get_variable_default ("Zpk", 10000) / 2.0;

  par[11] = get_variable_default ("Sr", 1.0);

  sky = &par[6];
}

/*                                  0  1    2   3   4      5    6     7   8   9       10  11 */
/* sersic galaxy model w/ bulge: -- x, y, (sx, sy, sxy)_1, I_1, sky, (sx, sy, sxy)_2, I_2, n */
/* exp (-b (r/r_e)^(1/n)) + pgauss (r) */
opihi_flt serbulgeTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px1, px2, py1, py2;
  opihi_flt z1, z2, r1, r2, t, q1, q2, f;

  X = x - par[0];
  Y = y - par[1];
  
  px1 = par[2]*X;
  py1 = par[3]*Y;
  px2 = par[7]*X;
  py2 = par[8]*Y;

  z1 = 0.5*SQ(px1) + 0.5*SQ(py1) + par[4]*X*Y;
  z2 = 0.5*SQ(px2) + 0.5*SQ(py2) + par[9]*X*Y;

  /* bulge component */
  r1 = 1.0 / (1 + z1 + 0.5*z1*z1*(1 + z1/3)); /* ~ exp (-Z) */

  /* disk component */
  t = pow (z2, par[11]);
  r2 = exp (-t);

  f = par[5]*r1 + par[10]*r2 + par[6];

  q1 = par[5]*r1*r1*(1 + z1 + 0.5*z1*z1);
  q2 = par[10]*r2*par[11]*pow(z2, par[11]-1);

  if (dpar != NULL) {
    dpar[0] = q1*(2*px1*par[2] + par[4]*Y) + q2*(2*px2*par[7] + par[9]*Y);
    dpar[1] = q1*(2*py1*par[3] + par[4]*X) + q2*(2*py2*par[8] + par[9]*X);
    dpar[2] = -2*q1*px1*X;
    dpar[3] = -2*q1*py1*Y;
    dpar[4] = -q1*X*Y;
    dpar[5] = +r1;
    dpar[6] = +1;
    dpar[7] = -2*q2*px2*X*50;
    dpar[8] = -2*q2*py2*Y*50;
    dpar[9] = -q2*X*Y*50;
    dpar[10] = +r2*50;
    dpar[11] = -q2*log(z2)*t*50;
  }
  return (f);
}

int serbulgeCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  2.35 * sqrt(2.0) / par[2]);
  set_variable ("SYg",  2.35 * sqrt(2.0) / par[3]);
  set_variable ("SXYg", par[4]);
  set_variable ("Zb",   par[5]);
  set_variable ("Sg",   par[6]);

  set_variable ("SXf",  2.35 * sqrt(2.0) / par[7]);
  set_variable ("SYf",  2.35 * sqrt(2.0) / par[8]);
  set_variable ("SXYf", par[9]);
  set_variable ("Zd",   par[10]);
  set_variable ("Sr",   par[11]);
}
