# include "data.h"

/* local private functions */
opihi_flt florentzOD (opihi_flt, opihi_flt *, int, opihi_flt *);

# define GET_VAR(V,A) \
  c = get_variable (A); \
  if (c == NULL) { \
    gprint (GP_ERR, "missing fit parameter A\n"); \
    return (FALSE); \
  } \
  V = atof (c); \
  free (c);

int vlorentz (int argc, char **argv) {

  int i, N, Npts, Npar, Quiet;
  opihi_flt par[4], *v1, *v2, *dy, **covar;
  opihi_flt chisq, ochisq, dchisq;
  Vector *xvec, *yvec, *svec, *ovec;
  char *c, name[16];

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

//   Guess = FALSE;
//   if ((N = get_argument (argc, argv, "-guess"))) {
//     Guess = TRUE;
//     remove_argument (N, &argc, argv);
//   }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: vlorentz <x> <y> <dy> (out)\n");
    gprint (GP_ERR, "  <dy> may be the words 'con[stant]' or 'poi[sson]', in which case the error is constructed'\n");
    gprint (GP_ERR, " uses guesses: C0 (mean), C1 (sigma), C2 (norm), C3 (sky)\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  int Nsvec = strlen(argv[3]);

  if ((Nsvec >= 3) && (!strncasecmp ("poisson", argv[3], Nsvec) || !strncasecmp ("constant", argv[3], Nsvec))) {
    svec = SelectVector ("vlorentz_err", ANYVECTOR, TRUE);
    if (svec == NULL) return (FALSE);
    MatchVector (svec, xvec, OPIHI_FLT);
    v1 = svec[0].elements.Flt;
    v2 = yvec[0].elements.Flt;
    if (!strncasecmp ("poisson", argv[3], Nsvec)) {
      for (i = 0; i < svec[0].Nelements; i++) {
	v1[i] = (isfinite(v2[i]) && (v2[i] > 0)) ? sqrt(v2[i]) : 1.0;
      }
    } else {
      for (i = 0; i < svec[0].Nelements; i++) {
	v1[i] = 1.0;
      }
    }
  } else {
    if ((svec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  }

  CastVector (svec, OPIHI_FLT);

  Npts = xvec[0].Nelements;
  ResetVector (ovec, OPIHI_FLT, Npts);

  ALLOCATE (dy, opihi_flt, Npts);

  GET_VAR (par[0], "C0");
  GET_VAR (par[1], "C1");
  GET_VAR (par[2], "C2");
  GET_VAR (par[3], "C3");
  Npar = 4;

  // mrqmin takes the inverse variance (do not generate NANs)
  v1 = svec[0].elements.Flt;
  v2 = dy;
  for (i = 0; i < Npts; i++, v1++, v2++) {
      *v2 = (*v1 == 0.0) ? 0.0 : 1.0 / (*v1 * *v1);
  } 
  
  ochisq = mrqinit (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, florentzOD, !Quiet);
  dchisq = ochisq + 2*Npts;

  for (i = 0; (i < 20) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); i++) {
    chisq = mrqmin (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, florentzOD, !Quiet);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (!Quiet) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (!Quiet) gprint (GP_ERR, "%d iterations\n", i); 

  for (i = 0; i < Npts; i++) {
    ovec[0].elements.Flt[i] = florentzOD (xvec[0].elements.Flt[i], par, Npar, dy);
  }
  ovec[0].Nelements = Npts;
  /* set output *before* variable renomalization */

  covar = mrqcovar (Npar);
  for (i = 0; i < Npar; i++) {
    sprintf (name, "C%d", i);
    set_variable (name, par[i]);
    if (!Quiet) gprint (GP_ERR, "%d  %f  %f\n", i, par[i], sqrt(covar[i][i]));
    sprintf (name, "dC%d", i);
    set_variable (name, sqrt(covar[i][i]));
  }

  free (dy);
  mrqfree (Npar);
  return (TRUE);
}

/* pars: x_o, g, Io, sky */
opihi_flt florentzOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt z, f, L;

  opihi_flt xo = par[0];
  opihi_flt go = par[1];
  opihi_flt Io = par[2];
  opihi_flt So = par[3];

  z = SQ(x - xo) + SQ(go)/4.0;
  f = 1.0 / z;
  L = 0.5*Io*go*f/M_PI + So;

  dpar[0] = Io*go*SQ(f)*(x - xo) / M_PI;
  dpar[1] = 0.5*Io*(f - 0.5*SQ(f)*SQ(go)) / M_PI;
  dpar[2] = 0.5*go*f / M_PI;
  dpar[3] = 1;
  
  return (L);
}
