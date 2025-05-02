# include "data.h"

/* local private functions */
opihi_flt fmaxwellOD (opihi_flt, opihi_flt *, int, opihi_flt *);

# define GET_VAR(V,A) \
  c = get_variable (A); \
  if (c == NULL) { \
    gprint (GP_ERR, "missing fit parameter A\n"); \
    return (FALSE); \
  } \
  V = atof (c); \
  free (c);

int vmaxwell (int argc, char **argv) {

  int i, N, Npts, Npar, Quiet;
  opihi_flt par[5], *v1, *v2, *dy, **covar;
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

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: vmaxwell <x> <y> <dy> (out)\n");
    gprint (GP_ERR, " uses guesses: C0 (mean), C1 (sigma), C2 (norm), C3 (sky), C4 (ref)\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((svec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 
  REQUIRE_VECTOR_FLT (svec, FALSE); 

  Npts = xvec[0].Nelements;
  ALLOCATE (dy, opihi_flt, Npts);
  ResetVector (ovec, OPIHI_FLT, Npts);

  GET_VAR (par[0], "C0");
  GET_VAR (par[1], "C1");
  GET_VAR (par[2], "C2");
  GET_VAR (par[3], "C3");
  GET_VAR (par[4], "C4");
  Npar = 5;
  /* careful of variable renomalization */

  v1 = svec[0].elements.Flt;
  v2 = dy;
  for (i = 0; i < Npts; i++, v1++, v2++) *v2 = 1.0 / (*v1 * *v1);
  
  ochisq = mrqinit (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, fmaxwellOD, !Quiet);
  dchisq = ochisq + 2*Npts;

  for (i = 0; (i < 30) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); i++) {
    chisq = mrqmin (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, fmaxwellOD, !Quiet);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (!Quiet) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (!Quiet) gprint (GP_ERR, "%d iterations\n", i); 

  for (i = 0; i < Npts; i++) {
    ovec[0].elements.Flt[i] = fmaxwellOD (xvec[0].elements.Flt[i], par, Npar, dy);
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

/* pars: x_o, -0.5/sigma^2, I, back, ref */
// f = C3 + C2*(x - C4)^2 * exp(-0.5*(x - C0)^2 / C1^2)
opihi_flt fmaxwellOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  opihi_flt z, r, f;

  z = (x - par[0])/par[1];
  r = SQ(x - par[4])*exp (-0.5*SQ(z));
  f = par[2]*r + par[3];

  dpar[0] = par[2]*r*z/par[1];
  dpar[1] = par[2]*r*z*z/par[1];
  dpar[2] = r;
  dpar[3] = 1;
  dpar[4] = -par[2]*(x - par[4])*exp(-0.5*SQ(z));
  
  return (f);

}
