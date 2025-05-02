# include "data.h"

/* local private functions */
opihi_flt fellipseOD (opihi_flt theta, opihi_flt *par, int Npar, opihi_flt *dpar);

// enum {PAR_X0, PAR_Y0, PAR_RX, PAR_RY, PAR_T0, PAR_P0};
enum {PAR_RMIN, PAR_EPSILON, PAR_PHI};

# define GET_VAR(V,A) { \
  char *c; \
  c = get_variable (A); \
  if (c == NULL) { \
    gprint (GP_ERR, "missing fit parameter A\n"); \
    return (FALSE); \
  } \
  V = atof (c); \
  free (c); }

int vellipse (int argc, char **argv) {

  int i, N, Npts, Npar, Quiet;
  opihi_flt par[6], *pos, *dpos, *theta, **covar;
  opihi_flt chisq, ochisq, dchisq;
  // opihi_flt Rmaj;
  opihi_flt Xmin, Xmax, Ymin, Ymax;
  Vector *xvec, *yvec, *Xvec, *Yvec, *tvec;

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: vellipse <xobs> <yobs> <xfit> <yfit> (theta)\n");
    // gprint (GP_ERR, " uses guesses: E_X0, E_Y0, E_RMAJ, E_RMIN, E_THETA\n");
    gprint (GP_ERR, " uses guesses: E_RMAJ, E_RMIN, E_PHI\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Xvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Yvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((tvec = SelectVector (argv[5], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 

  // GET_VAR (par[PAR_X0], "E_X0");
  // GET_VAR (par[PAR_Y0], "E_Y0");
  // GET_VAR (par[PAR_RX], "E_RMAJ");
  // GET_VAR (par[PAR_RY], "E_RMIN");
  // GET_VAR (par[PAR_T0], "E_THETA");
  // GET_VAR (par[PAR_P0], "E_PHI");
  // Npar = 6;

  // we need to generate a vector of alternating x,y values and the independent variable theta:
  Xmax = Ymax = Xmin = Ymin = 0.0;
  ALLOCATE (pos,   opihi_flt, 2*xvec[0].Nelements); 
  ALLOCATE (dpos,  opihi_flt, 2*xvec[0].Nelements); 
  ALLOCATE (theta, opihi_flt, 2*xvec[0].Nelements); 
  for (i = 0; i < xvec[0].Nelements; i++) {
    pos[2*i+0]   = xvec[0].elements.Flt[i];
    pos[2*i+1]   = yvec[0].elements.Flt[i];
    Xmin = MIN (Xmin, xvec[0].elements.Flt[i]);
    Xmax = MAX (Xmax, xvec[0].elements.Flt[i]);
    Ymin = MIN (Ymin, yvec[0].elements.Flt[i]);
    Ymax = MAX (Ymax, yvec[0].elements.Flt[i]);
    dpos[2*i+0]  = 1.0;
    dpos[2*i+1]  = 1.0;
    theta[2*i+0] = atan2(yvec[0].elements.Flt[i], xvec[0].elements.Flt[i]);
    theta[2*i+1] = theta[2*i+0];
    // fprintf (stderr, "x,y: %f, %f -> %f, %f\n", yvec[0].elements.Flt[i], xvec[0].elements.Flt[i], yvec[0].elements.Flt[i] - par[PAR_Y0], xvec[0].elements.Flt[i] - par[PAR_X0]);
    // fprintf (stderr, "x,y: %f, %f -> %f, %f\n", yvec[0].elements.Flt[i], xvec[0].elements.Flt[i], yvec[0].elements.Flt[i] - par[PAR_Y0], xvec[0].elements.Flt[i] - par[PAR_X0]);
    // fprintf (stderr, "theta: %f - %f : %f\n", DEG_RAD*theta[2*i+0], tvec[0].elements.Flt[i], DEG_RAD*theta[2*i+0] - tvec[0].elements.Flt[i]);
  }
  Npts = 2*xvec[0].Nelements;

  // basic guess from range, PHI in degrees, converted to radians
  // GET_VAR (Rmaj,          "E_RMAJ");
  // GET_VAR (par[PAR_RMIN], "E_RMIN");
  // GET_VAR (par[PAR_PHI],  "E_PHI");
  par[PAR_PHI] = 0.0;
  par[PAR_EPSILON] = 0.5;
  par[PAR_RMIN] = 0.25*((Xmax - Xmin) + (Ymax - Ymin));
  Npar = 3;

  ochisq = mrqinit (theta, pos, dpos, Npts, par, Npar, fellipseOD, !Quiet);
  dchisq = ochisq + 2*Npts;
  chisq  = ochisq + dchisq;

  for (i = 0; (i < 20) && (chisq > 1e-3) && ((dchisq > 0.001*(Npts - Npar)) || (dchisq <= 0.0)); i++) {
    chisq = mrqmin (theta, pos, dpos, Npts, par, Npar, fellipseOD, !Quiet);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (!Quiet) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (!Quiet) gprint (GP_ERR, "%d iterations\n", i); 

  ResetVector (Xvec, OPIHI_FLT, xvec[0].Nelements);
  ResetVector (Yvec, OPIHI_FLT, xvec[0].Nelements);
  for (i = 0; i < xvec[0].Nelements; i++) {
    Xvec[0].elements.Flt[i] = fellipseOD (theta[2*i+0], par, Npar, dpos);
    Yvec[0].elements.Flt[i] = fellipseOD (theta[2*i+1], par, Npar, dpos);
  }
  Xvec[0].Nelements = xvec[0].Nelements;
  Yvec[0].Nelements = xvec[0].Nelements;

  covar = mrqcovar (Npar);
  //set_variable ("E_X0",   par[PAR_X0]);
  //set_variable ("E_Y0",   par[PAR_Y0]);
  //set_variable ("E_RMAJ", par[PAR_RX]);
  //set_variable ("E_RMIN", par[PAR_RY]);
  //set_variable ("E_T0",   par[PAR_T0]);
  //set_variable ("E_P0",   par[PAR_P0]);

  set_variable ("E_RMAJ", par[PAR_RMIN] / par[PAR_EPSILON]);
  set_variable ("E_RMIN", par[PAR_RMIN]);
  set_variable ("E_PHI",  DEG_RAD*par[PAR_PHI]);

  // set_variable ("dE_X0",   sqrt(covar[PAR_X0][PAR_X0]));
  // set_variable ("dE_Y0",   sqrt(covar[PAR_Y0][PAR_Y0]));
  // set_variable ("dE_RMAJ", sqrt(covar[PAR_RX][PAR_RX]));
  // set_variable ("dE_RMIN", sqrt(covar[PAR_RY][PAR_RY]));
  // set_variable ("dE_T0",   sqrt(covar[PAR_T0][PAR_T0]));
  // set_variable ("dE_P0",   sqrt(covar[PAR_P0][PAR_P0]));

  set_variable ("dE_RMAJ", sqrt(covar[PAR_EPSILON][PAR_EPSILON]*covar[PAR_RMIN][PAR_RMIN]));
  set_variable ("dE_RMIN", sqrt(covar[PAR_RMIN][PAR_RMIN]));
  set_variable ("dE_PHI",  DEG_RAD*sqrt(covar[PAR_PHI][PAR_PHI]));

  // if (!Quiet) gprint (GP_ERR, "Xo : %f +/- %f\n", par[PAR_X0], sqrt(covar[PAR_X0][PAR_X0]));
  // if (!Quiet) gprint (GP_ERR, "Yo : %f +/- %f\n", par[PAR_Y0], sqrt(covar[PAR_Y0][PAR_Y0]));
  // if (!Quiet) gprint (GP_ERR, "RX : %f +/- %f\n", par[PAR_RX], sqrt(covar[PAR_RX][PAR_RX]));
  // if (!Quiet) gprint (GP_ERR, "RY : %f +/- %f\n", par[PAR_RY], sqrt(covar[PAR_RY][PAR_RY]));
  // if (!Quiet) gprint (GP_ERR, "To : %f +/- %f\n", par[PAR_T0], sqrt(covar[PAR_T0][PAR_T0]));
  // if (!Quiet) gprint (GP_ERR, "Po : %f +/- %f\n", par[PAR_P0], sqrt(covar[PAR_P0][PAR_P0]));

  if (!Quiet) gprint (GP_ERR, "RMAJ : %f +/- %f\n", par[PAR_RMIN] / par[PAR_EPSILON], 0.0);
  if (!Quiet) gprint (GP_ERR, "RMIN : %f +/- %f\n", par[PAR_RMIN], sqrt(covar[PAR_RMIN][PAR_RMIN]));
  if (!Quiet) gprint (GP_ERR, "PHI  : %f +/- %f\n", DEG_RAD*par[PAR_PHI],  sqrt(covar[PAR_PHI][PAR_PHI]));

  free (pos);
  free (dpos);
  free (theta);

  mrqfree (Npar);
  return (TRUE);
}

# if (0)
/**
 * the full chisq is built of two associated sums over coordinates:
 * chisq = sum ((X_obs - X_fit(t))^2 + (Y_obs - Y_fit(t))^2)
 * 
 * the independent variable is Theta:
 * X_fit = X0 + RX*cos(theta)*cos(phi) - RY*sin(theta)*sin(phi)
 * Y_fit = Y0 + RX*cos(theta)*sin(phi) + RY*sin(theta)*cos(phi)
 *
 * alternating calls to fellipseOD refer alternatively to X or Y
 *
 * parameters: Xo, Yo (center), RX, RY (axes), To (angle), Po (phase)
 */

opihi_flt fellipseOD (opihi_flt theta, opihi_flt *par, int Npar, opihi_flt *dpar) {
  
  static int pass = 0;

  opihi_flt csphi = cos(par[PAR_P0]);
  opihi_flt snphi = sin(par[PAR_P0]);

  opihi_flt dtheta = theta - par[PAR_T0];
  opihi_flt cstht = cos(dtheta);
  opihi_flt sntht = sin(dtheta);

  // value is X
  if (pass == 0) {
    pass = 1;

    opihi_flt value = par[PAR_X0] + par[PAR_RX]*cstht*csphi - par[PAR_RY]*sntht*snphi;

    dpar[PAR_X0] = 1.0;
    dpar[PAR_Y0] = 0.0;
    dpar[PAR_RX] = +cstht*csphi;
    dpar[PAR_RY] = -sntht*snphi;
    dpar[PAR_P0] = -par[PAR_RX]*cstht*snphi - par[PAR_RY]*sntht*csphi;
    dpar[PAR_T0] = -par[PAR_RX]*sntht*csphi - par[PAR_RY]*cstht*snphi;

    return (value);
  }  

  // value is Y
  if (pass == 1) {
    pass = 0;

    opihi_flt value = par[PAR_Y0] + par[PAR_RX]*cstht*snphi + par[PAR_RY]*sntht*csphi;

    dpar[PAR_X0]  = 0.0;
    dpar[PAR_Y0]  = 1.0;
    dpar[PAR_RX]  = +cstht*snphi;
    dpar[PAR_RY]  = +sntht*csphi;
    dpar[PAR_P0]  = +par[PAR_RX]*cstht*csphi - par[PAR_RY]*sntht*snphi;
    dpar[PAR_T0]  = -par[PAR_RX]*sntht*snphi + par[PAR_RY]*cstht*csphi;
    return (value);
  }  
  abort();
}

# endif

/**
 * the full chisq is built of two associated sums over coordinates:
 * chisq = sum ((X_obs - X_fit(t))^2 + (Y_obs - Y_fit(t))^2)
 * 
 * the independent variable is theta (angle from 0,0 to point):
 * X_fit = RMIN*R*cos(theta)
 * Y_fit = RMIN*R*sin(theta)
 * R = 1 / sqrt(sin^2(theta-phi) + epsilon^2 * cos^2(theta-phi))
 *
 * alternating calls to fellipseOD refer alternatively to X or Y
 *
 * RMIN, EPSILON, PHI
 */

// XXX NOTE that PHI is defined with the wrong sign, should fix this...
opihi_flt fellipseOD (opihi_flt alpha, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);
  
  static int pass = 0;

  opihi_flt cs_alpha = cos(alpha);
  opihi_flt sn_alpha = sin(alpha);

  opihi_flt cs_phi = cos(alpha - par[PAR_PHI]);
  opihi_flt sn_phi = sin(alpha - par[PAR_PHI]);

  opihi_flt r     = 1.0 / sqrt(SQ(sn_phi) + SQ(par[PAR_EPSILON]*cs_phi));
  opihi_flt r3    = pow(r, 3.0);
  opihi_flt drdE  = -0.5 * r3 * SQ(cs_phi) * 2.0 * par[PAR_EPSILON];
  opihi_flt drdP  = -0.5 * r3 * (SQ(par[PAR_EPSILON]) - 1) * 2.0 * cs_phi * sn_phi;

  // value is X
  if (pass == 0) {
    pass = 1;

    opihi_flt value = par[PAR_RMIN]*cs_alpha*r;

    dpar[PAR_RMIN]    = r*cs_alpha;
    dpar[PAR_EPSILON] = par[PAR_RMIN]*cs_alpha*drdE;
    dpar[PAR_PHI]     = 4.0*par[PAR_RMIN]*cs_alpha*drdP;

    return (value);
  }  

  // value is Y
  if (pass == 1) {
    pass = 0;

    opihi_flt value = par[PAR_RMIN]*sn_alpha*r;

    dpar[PAR_RMIN]    = r*sn_alpha;
    dpar[PAR_EPSILON] = par[PAR_RMIN]*sn_alpha*drdE;
    dpar[PAR_PHI]     = 4.0*par[PAR_RMIN]*sn_alpha*drdP;

    return (value);
  }  
  abort();
}

