# include "imfit.h"

opihi_flt trailTD (opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *);
void  trailCL ();

// fitted parameters:
# define PAR_X      0
# define PAR_Y      1
# define PAR_THETA  2
# define PAR_LENGTH 3
# define PAR_I0     4
# define PAR_SKY    5

// fixed parameters:
# define PAR_SIGMA  0

void trail_setup (char *name) {

  if (strcmp(name, "trail")) return;

  fitfunc = trailTD;
  imfit_cleanup = trailCL;
  Npar  = 6;
  Nfpar = 1;

  /* allocate free and fixed parameters */
  ALLOCATE (par, opihi_flt, MAX (Npar, 1));
  bzero (par, Npar*sizeof(opihi_flt));
  ALLOCATE (fpar, opihi_flt, MAX (Nfpar, 1));
  bzero (fpar, Nfpar*sizeof(opihi_flt));

  par[PAR_X      ] = get_variable_default ("Xg",      0.0);
  par[PAR_Y      ] = get_variable_default ("Yg",      0.0);
  par[PAR_THETA  ] = get_variable_default ("Tg",      0.0);
  par[PAR_LENGTH ] = get_variable_default ("Lg",     10.0);
  par[PAR_I0     ] = get_variable_default ("Zpk", 10000.0);
  par[PAR_SKY    ] = get_variable_default ("Sg",      0.0);
  sky = &par[PAR_SKY];

  fpar[PAR_SIGMA ] = get_variable_default ("Wg",      2.0);
}

void trailCL () {
  set_variable ("Xg",  par[PAR_X     ]);
  set_variable ("Yg",  par[PAR_Y     ]);
  set_variable ("Tg",  par[PAR_THETA ]);
  set_variable ("Lg",  par[PAR_LENGTH]);
  set_variable ("Zpk", par[PAR_I0    ]);
  set_variable ("Sg",  par[PAR_SKY   ]);

  set_variable ("Wg",  fpar[PAR_SIGMA]);
}

/* trailed gaussian -- x, y, Sigma, Theta, Length, I, sky */
opihi_flt trailTD (opihi_flt x, opihi_flt y, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt X = x - par[PAR_X];
  opihi_flt Y = y - par[PAR_Y];
  
  opihi_flt S2 = 2.0 * SQ(fpar[PAR_SIGMA]);

  opihi_flt ST = sin(RAD_DEG*par[PAR_THETA]);
  opihi_flt CT = cos(RAD_DEG*par[PAR_THETA]);

  opihi_flt Zp = (X*CT + Y*ST + 0.5*par[PAR_LENGTH]) / sqrt(S2);
  opihi_flt Zm = (X*CT + Y*ST - 0.5*par[PAR_LENGTH]) / sqrt(S2);

  opihi_flt Ep = erf(Zp);
  opihi_flt Em = erf(Zm);

  opihi_flt Rxy = Y*CT - X*ST;
  opihi_flt Gxy = exp(-Rxy*Rxy/S2);

  opihi_flt Pxy = Gxy * (Ep - Em);
  opihi_flt f = Pxy * par[PAR_I0] + par[PAR_SKY];

  if (dpar != NULL) {
    dpar[PAR_SKY]    = 1.0;
    dpar[PAR_I0]     = Pxy;

    float dGdR = -2.0 * Rxy * Gxy / S2; // -R Gxy / (2 Sigma^2)

    // are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
    // since X = X - par[PAR_X], dFoo/dXo = -dFoo/dX
    float dRdX = +ST;
    float dRdY = -CT;
    float dRdT = (-Y*ST - X*CT)*RAD_DEG;
    // note PAR_THETA is in degrees

    float dGdX = dGdR * dRdX;
    float dGdY = dGdR * dRdY;
    float dGdT = dGdR * dRdT;
    // dGdL is 0.0 because dRdL is 0.0 (R is not a function of L)

    // are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
    float dZpdX = -CT / sqrt(S2);
    float dZmdX = dZpdX; // float dZmdX = -CT / sqrt(S2); dZmdX = dZpdX

    float dZpdY = -ST / sqrt(S2); // float dZmdY = -ST / sqrt(S2); dZmdY = dZpdY
    float dZmdY = dZpdY;

    float dZpdL = +0.5 / sqrt(S2);
    float dZmdL = -0.5 / sqrt(S2);

    // note PAR_THETA is in degrees
    float dZpdT = (-X*ST + Y*CT) * RAD_DEG / sqrt(S2);
    float dZmdT = dZpdT; // dZpdT = dZmdT

    float dEdZp = exp (-Zp*Zp) * M_2_SQRTPI;
    float dEdZm = exp (-Zm*Zm) * M_2_SQRTPI;

    float dEpdX = dEdZp * dZpdX;
    float dEmdX = dEdZm * dZmdX;

    float dEpdY = dEdZp * dZpdY;
    float dEmdY = dEdZm * dZmdY;

    float dEpdL = dEdZp * dZpdL;
    float dEmdL = dEdZm * dZmdL;

    float dEpdT = dEdZp * dZpdT;
    float dEmdT = dEdZm * dZmdT;

    float dPdX = dGdX * (Ep - Em) + Gxy * (dEpdX - dEmdX);
    float dPdY = dGdY * (Ep - Em) + Gxy * (dEpdY - dEmdY);

    // dGdL is 0.0 because dRdL is 0.0
    float dPdL = Gxy * (dEpdL - dEmdL);
    float dPdT = dGdT * (Ep - Em) + Gxy * (dEpdT - dEmdT);

    dpar[PAR_X]      = par[PAR_I0] * dPdX;
    dpar[PAR_Y]      = par[PAR_I0] * dPdY;

    dpar[PAR_LENGTH] = par[PAR_I0] * dPdL;
    dpar[PAR_THETA]  = par[PAR_I0] * dPdT;
  }

  return (f);
}
