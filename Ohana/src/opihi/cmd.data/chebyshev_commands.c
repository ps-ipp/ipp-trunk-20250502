# include "data.h"

int chebyshev_poly (int argc, char **argv) {

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: chebyshev poly (xvec) (yvec) (order)\n");
    gprint (GP_ERR, "       return requested chebyshev polynomial for given order\n");
    gprint (GP_ERR, "       xvec should be normalized (range of -1 to +1)\n");
    return FALSE;
  }

  Vector *xvec = NULL, *yvec = NULL;
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  int order = atoi(argv[3]);

  ResetVector (yvec, OPIHI_FLT, xvec->Nelements);

  ChebyshevPolyVector (yvec, xvec, order);

  return TRUE;
}

int chebyshev_fit1d (int argc, char **argv) {

  int N;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: chebyshev fit1d (name) (xv) (yv) (order)\n");
    gprint (GP_ERR, "       1D fit of xv to yv using given order\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    if (VERBOSE) gprint (GP_ERR, "chebyshev %s not found, creating it\n", argv[1]);
    cheb = CreateChebyshev (argv[1]);
  }

  // select the x,y vectors
  Vector *xvec = NULL, *yvec = NULL;
  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  cheb->dimen = 1;
  cheb->order = atoi (argv[4]);
  int nterm = cheb->order + 1;

  // check type and size:
  if (xvec->Nelements < nterm) {
    gprint (GP_ERR, "insufficient data to support the requested order\n");
    return FALSE;
  }
  if (xvec->Nelements != yvec->Nelements) {
    gprint (GP_ERR, "vector lengths %s (%d) and %s (%d) do not match\n", xvec->name, xvec->Nelements, yvec->name, yvec->Nelements);
    return FALSE;
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  ChebyshevSetScale (cheb, xvec, 0);

  // generate the normalized vector xNorm
  // use a local vector?
  Vector *xNorm = ChebyshevNormVector (cheb, xvec, 0);

  // generate the N cheb polynomials based on xNorm
  ALLOCATE_PTR (poly, Vector *, nterm);
  for (int i = 0; i < nterm; i++) {
    poly[i] = ChebyshevPolyVector (NULL, xNorm, i);
  }

  // fit the data to the cheb polynomials
  ChebyshevPolyFit1D (cheb, yvec, poly);

  for (int i = 0; i < nterm; i++) {
    fprintf (stderr, "%d : %e\n", i, cheb->A[i]);
  }

  // free local temporary variables
  for (int i = 0; i < nterm; i++) {
    FREE (poly[i]);
  }
  FREE (poly);
  FREE (xNorm);

  return TRUE;
}

int chebyshev_applyfit1d (int argc, char **argv) {

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: chebyshev applyfit1d (name) (xvec) (yfit)\n");
    gprint (GP_ERR, "       apply 1D fit to xvec position to yield yfit\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    gprint (GP_ERR, "chebyshev %s not found\n", argv[1]);
    return FALSE;
  }
  int nterm = cheb->order + 1;

  Vector *xvec = NULL, *yvec = NULL;
  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (yvec, OPIHI_FLT, xvec->Nelements);

  // generate the normalized vector xNorm
  Vector *xNorm = ChebyshevNormVector (cheb, xvec, 0);

  // generate the N cheb polynomials based on xNorm
  ALLOCATE_PTR (poly, Vector *, nterm);
  for (int i = 0; i < nterm; i++) {
    poly[i] = ChebyshevPolyVector (NULL, xNorm, i);
  }

  ChebyshevPolyApplyFit1D (cheb, yvec, poly);

  // free local temporary variables
  for (int i = 0; i < nterm; i++) {
    FREE (poly[i]);
  }
  FREE (poly);
  FREE (xNorm);

  return TRUE;
}

int chebyshev_fit2d (int argc, char **argv) {

  int N;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: chebyshev fit1d (name) (xv) (yv) (zv) (order)\n");
    gprint (GP_ERR, "       2D fit of (xv,yv) to zv using given order\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    if (VERBOSE) gprint (GP_ERR, "chebyshev %s not found, creating it\n", argv[1]);
    cheb = CreateChebyshev (argv[1]);
  }

  // select the x,y vectors
  Vector *xvec = NULL, *yvec = NULL, *zvec = NULL;
  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  cheb->dimen = 2;
  cheb->order = atoi (argv[5]);
  int nterm = cheb->order + 1;

  // check type and size:
  if (xvec->Nelements < nterm) {
    gprint (GP_ERR, "insufficient data to support the requested order\n");
    return FALSE;
  }
  if (xvec->Nelements != yvec->Nelements) {
    gprint (GP_ERR, "vector lengths %s (%d) and %s (%d) do not match\n", xvec->name, xvec->Nelements, yvec->name, yvec->Nelements);
    return FALSE;
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);

  ChebyshevSetScale (cheb, xvec, 0);
  ChebyshevSetScale (cheb, yvec, 1);

  // generate the normalized vector xNorm
  // use a local vector?
  Vector *xNorm = ChebyshevNormVector (cheb, xvec, 0);
  Vector *yNorm = ChebyshevNormVector (cheb, yvec, 1);

  // generate the N cheb polynomials based on xNorm, yNorm
  ALLOCATE_PTR (xPoly, Vector *, nterm);
  ALLOCATE_PTR (yPoly, Vector *, nterm);
  for (int i = 0; i < nterm; i++) {
    xPoly[i] = ChebyshevPolyVector (NULL, xNorm, i);
    yPoly[i] = ChebyshevPolyVector (NULL, yNorm, i);
  }

  // fit the data to the cheb polynomials
  ChebyshevPolyFit2D (cheb, zvec, xPoly, yPoly);

  int n = 0;
  for (int ix = 0; ix < nterm; ix++) {
    for (int iy = 0; iy < nterm; iy++) {
      fprintf (stderr, "%d %d : %e\n", ix, iy, cheb->A[n]);
      n ++;
    }
  }

  // free local temporary variables
  for (int i = 0; i < nterm; i++) {
    FREE (xPoly[i]);
    FREE (yPoly[i]);
  }
  FREE (xPoly);
  FREE (yPoly);
  FREE (xNorm);
  FREE (yNorm);

  return TRUE;
}

int chebyshev_applyfit2d (int argc, char **argv) {

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: chebyshev applyfit1d (name) (xvec) (yvec) (xfit)\n");
    gprint (GP_ERR, "       apply 1D fit to xvec,yvec positions to yield zfit\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    gprint (GP_ERR, "chebyshev %s not found\n", argv[1]);
    return FALSE;
  }
  int nterm = cheb->order + 1;

  Vector *xvec = NULL, *yvec = NULL, *zvec = NULL;
  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if (xvec->Nelements != yvec->Nelements) {
    gprint (GP_ERR, "vector lengths %s (%d) and %s (%d) do not match\n", xvec->name, xvec->Nelements, yvec->name, yvec->Nelements);
    return FALSE;
  }

  ResetVector (zvec, OPIHI_FLT, xvec->Nelements);

  // generate the normalized vector xNorm
  // use a local vector?
  Vector *xNorm = ChebyshevNormVector (cheb, xvec, 0);
  Vector *yNorm = ChebyshevNormVector (cheb, yvec, 1);

  // generate the N cheb polynomials based on xNorm, yNorm
  ALLOCATE_PTR (xPoly, Vector *, nterm);
  ALLOCATE_PTR (yPoly, Vector *, nterm);
  for (int i = 0; i < nterm; i++) {
    xPoly[i] = ChebyshevPolyVector (NULL, xNorm, i);
    yPoly[i] = ChebyshevPolyVector (NULL, yNorm, i);
  }


  ChebyshevPolyApplyFit2D (cheb, zvec, xPoly, yPoly);

  // free local temporary variables
  for (int i = 0; i < nterm; i++) {
    FREE (xPoly[i]);
    FREE (yPoly[i]);
  }
  FREE (xPoly);
  FREE (yPoly);
  FREE (xNorm);
  FREE (yNorm);

  return TRUE;
}

int chebyshev_scale (int argc, char **argv) {

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: chebyshev scale (name) (vector)\n");
    gprint (GP_ERR, "       determine scale factors for the names chebyshev based on the vector\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    gprint (GP_ERR, "chebyshev %s not found, creating it\n", argv[1]);
    cheb = CreateChebyshev (argv[1]);
  } else {
    gprint (GP_ERR, "resetting scale factors for %s\n", argv[1]);
  }

  Vector *vec = NULL;
  if ((vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  // check type and size:
  if (vec->type != OPIHI_FLT) {
    gprint (GP_ERR, "cannot generate chebyshev scale factors for non-float vector\n");
    return FALSE;
  }
  if (!vec->Nelements) {
    gprint (GP_ERR, "cannot generate chebyshev scale factors based on empty vector\n");
    return FALSE;
  }

  int dir = 0;
  ChebyshevSetScale (cheb, vec, dir);

  return TRUE;
}

int chebyshev_applyscale (int argc, char **argv) {

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: chebyshev applyscale (name) (vector) (output)\n");
    gprint (GP_ERR, "       re-scale vector for chebyshev\n");
    return FALSE;
  }

  ChebyshevType *cheb = FindChebyshev (argv[1]);
  if (!cheb) {
    gprint (GP_ERR, "chebyshev %s not found\n", argv[1]);
    return FALSE;
  }

  Vector *vec = NULL, *out = NULL;
  if ((vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((out = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  CastVector (vec, OPIHI_FLT);

  ResetVector (out, OPIHI_FLT, vec->Nelements);

  int dir = 0;

  for (int i = 0; i < vec->Nelements; i++) {
    if (!isfinite(vec->elements.Flt[i])) { out->elements.Flt[i] = NAN; continue; }
    out->elements.Flt[i] = cheb->scale[dir]*vec->elements.Flt[i] + cheb->zero[dir];
  }

  return TRUE;
}

