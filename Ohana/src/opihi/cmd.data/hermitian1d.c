# include "data.h"

int hermitian1d (int argc, char **argv) {
  
  int i, N, order, Nvec, Poly;
  opihi_int *inI;
  opihi_flt *out, *inF;
  opihi_flt mean, sigma, x;
  double nf, norm;
  Vector *xvec, *yvec;

  Poly = FALSE;
  if ((N = get_argument (argc, argv, "-poly"))) {
    Poly = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) goto usage;

  /* select input / output buffers */
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  Nvec = xvec[0].Nelements;
  ResetVector (yvec, OPIHI_FLT, Nvec);

  /* gaussian parameters */
  mean  = atof (argv[3]);
  sigma = atof (argv[4]);
  order = atoi (argv[5]);
  if (order < 0) goto usage;
  if (order > 10) goto usage;

  out = yvec[0].elements.Flt;
  inF = xvec[0].elements.Flt;
  inI = xvec[0].elements.Int;

  nf = exp(lgamma(order + 1));
  norm = 1.0 / sqrt(nf*sqrt(2*M_PI)*sigma);

  // a little sub-optimal : split this up with macros?
  for (i = 0; i < Nvec; i++, out++) {
    if (xvec[0].type == OPIHI_FLT) {
      x = (inF[i] - mean) / sigma;
    } else {
      x = (inI[i] - mean) / sigma;
    }
    *out = hermitian_polynomial (x, order);
    if (!Poly) {
      *out *= norm*exp (-0.25*x*x);
    }
  }
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: hermitian1d (x) (y) (mean) (sigma) (order)\n");
  gprint (GP_ERR, "  Note : only orders up to 10 are supported\n");
  return (FALSE);

}

// {\psi}_n(x) = \frac{1}{\sqrt{n! \, 2^n\sqrt{\pi}}}\, \mathrm{e}^{-x^2/2}H_n(x).\,\! 
