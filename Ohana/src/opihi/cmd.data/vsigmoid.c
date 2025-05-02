# include "data.h"

typedef opihi_flt FitFunc (opihi_flt, opihi_flt *, int, opihi_flt *);

/* local private functions */
opihi_flt fsigmoidOD (opihi_flt, opihi_flt *, int, opihi_flt *);
opihi_flt rsigmoidOD (opihi_flt, opihi_flt *, int, opihi_flt *);

# define GET_VAR(V,A) {					\
    char *c = get_variable (A);				\
    if (c == NULL) {					\
      gprint (GP_ERR, "missing fit parameter A\n");	\
      return (FALSE);					\
    }							\
    V = atof (c);					\
    free (c); }

// XXX this fitting function works but fails if the input vectors have NANs
int vsigmoid (int argc, char **argv) {

  int N;
  Vector *xvec, *yvec, *svec, *ovec;

  FitFunc *FUNC = fsigmoidOD;
  if ((N = get_argument (argc, argv, "-r"))) {
    FUNC = rsigmoidOD;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-rev"))) {
    FUNC = rsigmoidOD;
    remove_argument (N, &argc, argv);
  }

  int Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: vsigmoid [-q] [-r] [-rev] <x> <y> <dy> (out)\n");
    gprint (GP_ERR, "  <dy> may be the words 'con[stant]' or 'poi[sson]', in which case the error is constructed'\n");
    gprint (GP_ERR, " uses guesses: C0 (center), C1 (sigma), C2 (top), C3 (bottom)\n");
    gprint (GP_ERR, " forward sigmoid : bottom + top / (1 + exp(-z)), z = (x - center) / sigma\n");
    gprint (GP_ERR, " -r / -rev : reverse sigmoid : bottom + top / (1 + exp(z))\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  int Nsvec = strlen(argv[3]);

  if ((Nsvec >= 3) && (!strncasecmp ("poisson", argv[3], Nsvec) || !strncasecmp ("constant", argv[3], Nsvec))) {
    svec = SelectVector ("vsigmoid_err", ANYVECTOR, TRUE);
    if (svec == NULL) return (FALSE);
    MatchVector (svec, xvec, OPIHI_FLT);
    opihi_flt *v1 = svec[0].elements.Flt;
    opihi_flt *v2 = yvec[0].elements.Flt;
    if (!strncasecmp ("poisson", argv[3], Nsvec)) {
      for (int i = 0; i < svec[0].Nelements; i++) {
	v1[i] = (isfinite(v2[i]) && (v2[i] > 0)) ? sqrt(v2[i]) : 1.0;
      }
    } else {
      for (int i = 0; i < svec[0].Nelements; i++) {
	v1[i] = 1.0;
      }
    }
  } else {
    if ((svec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  }

  CastVector (svec, OPIHI_FLT);
  // XXX Cast is failing.

  int Npts = xvec[0].Nelements;
  ResetVector (ovec, OPIHI_FLT, Npts);

  ALLOCATE_PTR (dy, opihi_flt, Npts);

  opihi_flt par[4];
  GET_VAR (par[0], "C0");
  GET_VAR (par[1], "C1");
  GET_VAR (par[2], "C2");
  GET_VAR (par[3], "C3");
  int Npar = 4;

  // mrqmin takes the inverse variance (do not generate NANs)
  opihi_flt *v1 = svec[0].elements.Flt;
  opihi_flt *v2 = dy;
  for (int i = 0; i < Npts; i++, v1++, v2++) {
      *v2 = (*v1 == 0.0) ? 0.0 : 1.0 / (*v1 * *v1);
  } 
  
  opihi_flt ochisq = mrqinit (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, FUNC, !Quiet);
  opihi_flt dchisq = ochisq + 2*Npts;

  int Niter;
  for (Niter = 0; (Niter < 20) && ((dchisq > 0.1*(Npts - Npar)) || (dchisq <= 0.0)); Niter++) {
    opihi_flt chisq = mrqmin (xvec[0].elements.Flt, yvec[0].elements.Flt, dy, Npts, par, Npar, FUNC, !Quiet);
    dchisq = ochisq - chisq;
    ochisq = chisq;
    if (!Quiet) gprint (GP_ERR, "dchisq: %f, Ndof: %d\n", dchisq, Npts - Npar);
  }  
  if (!Quiet) gprint (GP_ERR, "%d iterations\n", Niter); 

  for (int i = 0; i < Npts; i++) {
    ovec[0].elements.Flt[i] = FUNC (xvec[0].elements.Flt[i], par, Npar, dy);
  }
  ovec[0].Nelements = Npts;
  /* set output *before* variable renomalization */

  opihi_flt **covar = mrqcovar (Npar);
  for (int i = 0; i < Npar; i++) {
    char name[16];
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

/* pars: x_o, sigma, top, bottom */
opihi_flt rsigmoidOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  double z = (x - par[0])/par[1];
  double r = exp (z);
  double s = 1 + r;
  double q = 1 / s;
  double f = par[2]*q + par[3];

  // df/dp0 = df/dq * dq/dr * dr/dz * dz/dp0
  // df/dq  = par[2]
  // dq/ds  = -s^-2 = -q/s
  // ds/dr  = 1
  // dr/dz  = r
  // dz/dp0 = -1 / par[1]
  // dz/dp1 = -z / par[1]

  // df/dp0 = par[2]*(-q/s)*r*(-1/par[1]) = par[2]*q*r/(s*par[1])
  // df/dp1 = par[2]*(-q/s)*r*(-z/par[1]) = par[2]*q*r*z/(s*par[1])

  dpar[0] = par[2]*q*r/(s*par[1]); 
  dpar[1] = dpar[0]*z;
  dpar[2] = q;
  dpar[3] = 1;
  
  return (f);
}

/* pars: x_o, sigma, top, bottom */
opihi_flt fsigmoidOD (opihi_flt x, opihi_flt *par, int Npar, opihi_flt *dpar) {
  OHANA_UNUSED_PARAM(Npar);

  double z = (x - par[0])/par[1];
  double r = exp (-z);
  double s = 1 + r;
  double q = 1 / s;
  double f = par[2]*q + par[3];

  // df/dp0 = df/dq * dq/dr * dr/dz * dz/dp0
  // df/dq  = par[2]
  // dq/ds  = -s^-2 = -q/s
  // ds/dr  = 1
  // dr/dz  = -r
  // dz/dp0 = -1 / par[1]
  // dz/dp1 = -z / par[1]

  // df/dp0 = -par[2]*(-q/s)*r*(-1/par[1]) = par[2]*q*r/(s*par[1])
  // df/dp1 = -par[2]*(-q/s)*r*(-z/par[1]) = par[2]*q*r*z/(s*par[1])

  dpar[0] = -par[2]*q*r/(s*par[1]); 
  dpar[1] = dpar[0]*z;
  dpar[2] = q;
  dpar[3] = 1;
  
  return (f);
}
