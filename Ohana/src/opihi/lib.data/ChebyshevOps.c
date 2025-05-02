# include "data.h"

/* this file contains functions to manage the median image data
 */

ChebyshevType **chebyshevs = NULL; /* book to store the list of all splines */
int     Nchebyshevs;   /* number of currently defined chebyshevs */
int     NCHEBYSHEVS;   /* number of currently allocated chebyshevs */

void InitChebyshevs () {
  Nchebyshevs = 0;
  NCHEBYSHEVS = 16;
  ALLOCATE (chebyshevs, ChebyshevType *, NCHEBYSHEVS); 
}

void FreeChebyshevs () {

  for (int i = 0; i < Nchebyshevs; i++) {
    FreeChebyshev (chebyshevs[i]);
  }
  FREE (chebyshevs);
}

void FreeChebyshev (ChebyshevType *chebyshev) {

  if (!chebyshev) return;

  FREE (chebyshev[0].name);
  FREE (chebyshev[0].A);
  free (chebyshev);
}

/* return the given chebyshev */
ChebyshevType *FindChebyshev (char *name) {

  if (!chebyshevs) return NULL;

  for (int i = 0; i < Nchebyshevs; i++) {
    if (!strcmp (chebyshevs[i][0].name, name)) {
      return (chebyshevs[i]);
    }
  }
  return (NULL);
}

/* make a new named chebyshev */
ChebyshevType *CreateChebyshev (char *name) {

  if (!chebyshevs) InitChebyshevs ();

  // do not create a chebyshev if one exists 
  ChebyshevType *chebyshev = FindChebyshev (name);
  if (chebyshev != NULL) {
    return NULL;
  }

  int N = Nchebyshevs;
  Nchebyshevs ++;
  CHECK_REALLOCATE (chebyshevs, ChebyshevType *, NCHEBYSHEVS, Nchebyshevs, 16);
  ALLOCATE (chebyshev, ChebyshevType, 1);

  chebyshev->name     = strcreate (name);
  chebyshev->zero[0]  = chebyshev->zero[1]  = NAN;
  chebyshev->scale[0] = chebyshev->scale[1] = NAN;
  chebyshev->order    = -1;
  chebyshev->dimen    = -1;
  chebyshev->A        = NULL;

  chebyshevs[N] = chebyshev;
  return (chebyshev);
}

/* delete a chebyshev */
int DeleteChebyshev (ChebyshevType *chebyshev) {

  if (!chebyshevs) return TRUE; // 'true' otherwise delete will abort if none yet defined

  /* find chebyshev in chebyshev list */
  int N = -1;
  for (int i = 0; i < Nchebyshevs; i++) {
    if (chebyshevs[i] == chebyshev) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (int i = N; i < Nchebyshevs - 1; i++) {
    chebyshevs[i] = chebyshevs[i + 1];
  }
  Nchebyshevs --;
  int NCHEBYSHEVS_2 = MAX (16, NCHEBYSHEVS / 2);
  if (Nchebyshevs < NCHEBYSHEVS_2) {
    NCHEBYSHEVS = NCHEBYSHEVS_2;
    REALLOCATE (chebyshevs, ChebyshevType *, NCHEBYSHEVS);
  }

  FreeChebyshev (chebyshev);
  return (TRUE);
}

/* list known chebyshevs */
void ListChebyshevs () {

  if (!chebyshevs) return;

  for (int i = 0; i < Nchebyshevs; i++) {
    gprint (GP_ERR, "%-15s %2d %2d (%15.8e %15.8e)", chebyshevs[i][0].name, chebyshevs[i][0].order, chebyshevs[i][0].dimen, chebyshevs[i][0].zero[0], chebyshevs[i][0].scale[0]);
    if (chebyshevs[i][0].dimen == 2) { gprint (GP_ERR, " (%15.8e %15.8e)", chebyshevs[i][0].zero[1], chebyshevs[i][0].scale[1]); } 
    gprint (GP_ERR, "\n");
  }
  return;
}

/**** below are technical operations ****/

int ChebyshevSetScale (ChebyshevType *cheb, Vector *vec, int dir) {

  if (dir >= 2) {
    gprint (GP_ERR, "invalid direction %d\n", dir);
    return FALSE;
  }

  // find the min and max of the vector
  opihi_flt minValue = NAN;
  opihi_flt maxValue = NAN;

  for (int i = 0; i < vec->Nelements; i++) {
    if (isnan(vec->elements.Flt[i])) continue;
    if (isnan(minValue)) { minValue = vec->elements.Flt[i]; }
    if (isnan(maxValue)) { maxValue = vec->elements.Flt[i]; }
    minValue = MIN(minValue, vec->elements.Flt[i]);
    maxValue = MAX(maxValue, vec->elements.Flt[i]);
  }
  if (minValue == maxValue) {
    gprint (GP_ERR, "insufficient data range to determine scale factors\n");
    return FALSE;
  }

  cheb->scale[dir] = 2.0 / (maxValue - minValue);
  cheb->zero[dir]  = 1 - cheb->scale[dir] * maxValue;
  return TRUE;
}

Vector *ChebyshevNormVector (ChebyshevType *cheb, Vector *xvec, int dir) {

  Vector *xNorm = InitVector();
  ResetVector (xNorm, OPIHI_FLT, xvec->Nelements);

  for (int i = 0; i < xvec->Nelements; i++) {
    xNorm->elements.Flt[i] = xvec->elements.Flt[i]*cheb->scale[dir] + cheb->zero[dir];
  }
  return xNorm;
}

Vector *ChebyshevPolyVector (Vector *Ti, Vector *vec, int order) {

  if (Ti == NULL) { Ti = InitVector(); }
  ResetVector (Ti, OPIHI_FLT, vec->Nelements);

  // quick but inefficient implementation
  switch (order) {
    case 0:
      for (int i = 0; i < vec->Nelements; i++) {                                          Ti->elements.Flt[i] = 1.0; } break;
    case 1:
      for (int i = 0; i < vec->Nelements; i++) {                                          Ti->elements.Flt[i] = vec->elements.Flt[i]; } break;
    case 2:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = 2.0*x2 - 1.0; } break;
    case 3:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = vec->elements.Flt[i]*(4.0*x2 - 3.0); } break;
    case 4:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = x2*(8.0*x2 - 8.0) + 1.0; } break;
    case 5:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = vec->elements.Flt[i] * (x2*(16.0*x2 - 20.0) + 5.0); } break;
    case 6:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = x2*(x2*(32.0*x2 - 48.0) + 18.0) - 1.0; } break;
    case 7:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = vec->elements.Flt[i] * (x2*(x2*(64.0*x2 - 112.0) + 56.0) - 7.0); } break;
    case 8:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = x2*(x2*(x2*(128.0*x2 - 256.0) + 160.0) - 32.0) + 1.0; } break;
    case 9:
      for (int i = 0; i < vec->Nelements; i++) { opihi_flt x2 = SQ(vec->elements.Flt[i]); Ti->elements.Flt[i] = vec->elements.Flt[i] *(x2*(x2*(x2*(256.0*x2 - 576.0) + 432.0) - 129.0) + 9.0); } break;
    default:
      gprint (GP_ERR, "Chebyshev orders higher than 9 are not yet coded\n");
      FREE (Ti);
      return NULL;
  }

  return Ti;
}


// we are supplied a vector of length vec->Nelements
// poly is an array of nterm (= order + 1) chebyshev polynomials
int ChebyshevPolyFit1D (ChebyshevType *cheb, Vector *vec, Vector **poly) {

  /* nterm is number of polynomial terms, starting at x^0 */
  int nterm = cheb->order + 1;

  ALLOCATE_PTR (b, double *, nterm);
  ALLOCATE_PTR (c, double *, nterm);
  for (int i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm); 
    ALLOCATE (b[i], double, 1);
    memset (c[i], 0, nterm*sizeof(double));
    memset (b[i], 0, sizeof(double));
  }

  opihi_flt *y = vec[0].elements.Flt;

  for (int i = 0; i < vec->Nelements; i++, y++) {
    if (!finite(*y)) continue;
    
    // only calculate the upper diagonal
    for (int j = 0; j < nterm; j++) {
      b[j][0] += *y * poly[j]->elements.Flt[i];
      for (int k = j; k < nterm; k++) {
	c[j][k] += poly[j]->elements.Flt[i]*poly[k]->elements.Flt[i];
      }
    }
  }

  // fill in the lower diagonal
  for (int j = 0; j < nterm; j++) {
    for (int k = 0; k < j; k++) {
      c[j][k] = c[k][j];
    }
  }

  if (!dgaussjordan (c, b, nterm, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  
  if (cheb->A) FREE (cheb->A);
  ALLOCATE (cheb->A, double, nterm);
  for (int j = 0; j < nterm; j++) {
    cheb->A[j] = b[j][0];
  }

  for (int i = 0; i < nterm; i++) {
    FREE (c[i]);
    FREE (b[i]);
  }
  FREE (b);
  FREE (c);

  return TRUE;
}

// we are supplied a vector of length vec->Nelements
// poly is an array of nterm (= order + 1) chebyshev polynomials
int ChebyshevPolyApplyFit1D (ChebyshevType *cheb, Vector *vec, Vector **poly) {

  int nterm = cheb->order + 1;

  for (int i = 0; i < vec->Nelements; i++) {
    opihi_flt value = 0.0;
    for (int j = 0; j < nterm; j++) {
      value += cheb->A[j] * poly[j]->elements.Flt[i];
    }
    vec->elements.Flt[i] = value;
  }

  return TRUE;
}

// we are supplied a vector of length vec->Nelements
// poly is an array of nterm (= order + 1) chebyshev polynomials
int ChebyshevPolyFit2D (ChebyshevType *cheb, Vector *vec, Vector **xPoly, Vector **yPoly) {

  /* nterm is number of polynomial terms, starting at x^0 */
  int nterm = SQ(cheb->order + 1);

  ALLOCATE_PTR (b, double *, nterm);
  ALLOCATE_PTR (c, double *, nterm);
  for (int i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm); 
    ALLOCATE (b[i], double, 1);
    memset (c[i], 0, nterm*sizeof(double));
    memset (b[i], 0, sizeof(double));
  }

  opihi_flt *y = vec[0].elements.Flt;

  for (int i = 0; i < vec->Nelements; i++, y++) {
    if (!finite(*y)) continue;
    
    // XXX can we only calculate the upper diagonal?
    int n = 0;
    for (int jx = 0; jx <= cheb->order; jx++) {
      for (int jy = 0; jy <= cheb->order; jy++) {
	
	opihi_flt vj = xPoly[jx]->elements.Flt[i] * yPoly[jy]->elements.Flt[i];
	b[n][0] += *y * vj;

	int m = 0;
	for (int kx = 0; kx <= cheb->order; kx++) {
	  for (int ky = 0; ky <= cheb->order; ky++) {
	    c[n][m] += vj * xPoly[kx]->elements.Flt[i]*yPoly[ky]->elements.Flt[i];
	    m++;
	  }
	}
	n++;
      }
    }
  }

  for (int i = 0; i < nterm; i++) {
    fprintf (stderr, "c: ");
    for (int j = 0; j < nterm; j++) {
      fprintf (stderr, "%e ", c[i][j]); 
    }
    fprintf (stderr, "  b : %e\n", b[i][0]);
  }

  if (!dgaussjordan (c, b, nterm, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  
  for (int i = 0; i < nterm; i++) {
    fprintf (stderr, "c: ");
    for (int j = 0; j < nterm; j++) {
      fprintf (stderr, "%e ", c[i][j]); 
    }
    fprintf (stderr, "  b : %e\n", b[i][0]);
  }

  if (cheb->A) FREE (cheb->A);
  ALLOCATE (cheb->A, double, nterm);
  for (int j = 0; j < nterm; j++) {
    cheb->A[j] = b[j][0];
  }

  for (int i = 0; i < nterm; i++) {
    FREE (c[i]);
    FREE (b[i]);
  }
  FREE (b);
  FREE (c);

  return TRUE;
}

// we are supplied a vector of length vec->Nelements
// poly is an array of nterm (= order + 1) chebyshev polynomials
int ChebyshevPolyApplyFit2D (ChebyshevType *cheb, Vector *vec, Vector **xPoly, Vector **yPoly) {

  for (int i = 0; i < vec->Nelements; i++) {
    opihi_flt value = 0.0;

    int n = 0;
    for (int jx = 0; jx <= cheb->order; jx++) {
      for (int jy = 0; jy <= cheb->order; jy++) {
	opihi_flt vj = xPoly[jx]->elements.Flt[i] * yPoly[jy]->elements.Flt[i];
	value += cheb->A[n] * vj;
	n ++;
      }
    }
    vec->elements.Flt[i] = value;
  }

  return TRUE;
}

