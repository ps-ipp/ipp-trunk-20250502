# include "imfit.h"
# define FFACTOR 200
# define FSCALE 1.2

opihi_flt sgaussTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  sgaussCL ();

void sgauss_setup (char *name) {

  if (strcmp(name, "sgauss")) return;

  fitfunc = sgaussTD;
  imfit_cleanup = sgaussCL;
  Npar = 10;
  Nfpar = 2;

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
  par[6] = get_variable_default ("Sg", 0.0);
  par[7] = 2.35 / get_variable_default ("SXf", 15.0);
  par[8] = 2.35 / get_variable_default ("SYf", 15.0);
  par[9] = get_variable_default ("SXYf", 0.0);

  fpar[0] = get_variable_default ("Npow", 2.25);
  fpar[1] = get_variable_default ("Npin", 1.00); // drop this?

  sky = &par[6];
}

void sgaussCL () {
  set_variable ("Xg",   par[0]);
  set_variable ("Yg",   par[1]);
  set_variable ("SXg",  2.35 / par[2]);
  set_variable ("SYg",  2.35 / par[3]);
  set_variable ("SXYg", par[4]);
  set_variable ("Zpk",  par[5]);
  set_variable ("Sg",   par[6]);
  set_variable ("SXf", 2.35 / par[7]);
  set_variable ("SYf", 2.35 / par[8]);
  set_variable ("SXYf", par[9]);
}

/* two components: (1 + z_1 + z_2^N)^(-1) -- x, y, sx1, sy1, sxy1, I, sky, sx2, sy2, sxy2 */
opihi_flt sgaussTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X, Y, px1, py1, px2, py2;
  opihi_flt z1, z2, r, q1, q2, f, f1, f2;

  X = x - par[0];
  Y = y - par[1];
  
  px1 = par[2]*X;
  py1 = par[3]*Y;
  px2 = par[7]*X;
  py2 = par[8]*Y;

  z1 = 0.5*SQ(px1) + 0.5*SQ(py1) + par[4]*X*Y;
  z2 = 0.5*SQ(px2) + 0.5*SQ(py2) + par[9]*X*Y;

  r = 1.0 / (1 + z1 + pow(z2,fpar[0]));
  f = par[5]*r + par[6];

  q1 = par[5]*SQ(r);
  q2 = par[5]*SQ(r)*fpar[0]*pow(z2,(fpar[0]-1));

  if (dpar != NULL) {
    dpar[0] = q1*(2*px1*par[2] + par[4]*Y) + q2*(2*px2*par[7] + par[9]*Y);
    dpar[1] = q1*(2*py1*par[3] + par[4]*X) + q2*(2*py2*par[8] + par[9]*X);

    /* these fudge factors impede the growth of par[2] beyond par[7] */
    f1 = fabs(par[7]) / fabs(par[2]);
    f2 = (f1 < FSCALE) ? 1 : FFACTOR*(f1 - FSCALE) + 1;
    dpar[2] = -2*q1*px1*X*f2;

    f1 = fabs(par[8]) / fabs(par[3]);
    f2 = (f1 < FSCALE) ? 1 : FFACTOR*(f1 - FSCALE) + 1;
    dpar[3] = -2*q1*py1*Y*f2;

    dpar[4] = -q1*X*Y;
    dpar[5] = +r;
    dpar[6] = +1;
    dpar[7] = -2*q2*px2*X;
    dpar[8] = -2*q2*py2*Y;
    dpar[9] = -q2*X*Y;
  }
  return (f);
}

