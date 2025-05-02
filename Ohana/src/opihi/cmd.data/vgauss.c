# include "data.h"

/* local private functions */
opihi_flt fgaussOD (opihi_flt, opihi_flt *, int, opihi_flt *);
int vgauss_apply (int argc, char **argv);

# define GET_VAR(V,A) \
  c = get_variable (A); \
  if (c == NULL) { \
    gprint (GP_ERR, "missing fit parameter A\n"); \
    return (FALSE); \
  } \
  V = atof (c); \
  free (c);

int vgauss (int argc, char **argv) {

  int i, N, Npts, Npar, Quiet;
  opihi_flt par[4], *v1, *v2, *dy, **covar;
  opihi_flt chisq, ochisq, dchisq;
  Vector *xvec, *yvec, *svec, *ovec;
  char *c, name[16];

  if ((N = get_argument (argc, argv, "-apply"))) {
    remove_argument (N, &argc, argv);
    int status = vgauss_apply (argc, argv);
    return status;
  }

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
    gprint (GP_ERR, "USAGE: vgauss <x> <y> <dy> (out)\n");
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
    svec = SelectVector ("vgauss_err", ANYVECTOR, TRUE);
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
  // XXX Cast is failing.

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
  
  chisq = NAN;
  ochisq = mrqinit (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, fgaussOD, !Quiet);
  dchisq = ochisq + 2*Npts;

  for (i = 0; (i < 20) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); i++) {
    chisq = mrqmin (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, fgaussOD, !Quiet);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (!Quiet) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (!Quiet) gprint (GP_ERR, "%d iterations\n", i); 
  set_variable ("Chisq", chisq);
  set_variable ("ChisqNu", chisq / (float) (Npts - Npar));

  for (i = 0; i < Npts; i++) {
    ovec[0].elements.Flt[i] = fgaussOD (xvec[0].elements.Flt[i], par, Npar, dy);
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

int vgauss_apply (int argc, char **argv) {

  opihi_flt par[4];
  Vector *xvec, *ovec;
  char *c;

  // NOTE: -apply has already been stripped by vgauss call
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: vgauss -apply <x> <out>\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  CastVector (xvec, OPIHI_FLT);

  GET_VAR (par[0], "C0");
  GET_VAR (par[1], "C1");
  GET_VAR (par[2], "C2");
  GET_VAR (par[3], "C3");
  int Npar = 4;

  int Npts = xvec[0].Nelements;
  ResetVector (ovec, OPIHI_FLT, Npts);

  for (int i = 0; i < Npts; i++) {
    ovec[0].elements.Flt[i] = fgaussOD (xvec[0].elements.Flt[i], par, Npar, NULL);
  }
  ovec[0].Nelements = Npts;

  return TRUE;
}

/* pars: x_o, sigma, I, back */
opihi_flt fgaussOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt z, r, f;

  z = (x - par[0])/par[1];
  r = exp (-0.5*SQ(z));
  f = par[2]*r + par[3];

  if (dpar) {
    dpar[0] = par[2]*r*z/par[1];
    dpar[1] = par[2]*r*z*z/par[1];
    dpar[2] = r;
    dpar[3] = 1;
  }
  
  return (f);
}
