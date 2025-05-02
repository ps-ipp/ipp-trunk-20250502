# include "relastro.h"
# define PAR_TOOFEW 5

int BootstrapSaveUnmasked (FitAstromPoint *nomask, FitAstromPoint *points, int Npoints) {
  int i, N;

  // I need to draw Npoints random entries from 'points' with replacement:
  N = 0;
  for (i = 0; i < Npoints; i++) {
    if (points[i].mask) continue;
    nomask[N] = points[i];
    N ++;
  }
  return N;
}

int BootstrapResample (FitAstromPoint *sample, FitAstromPoint *points, int Npoints) {
  int i;

  // I need to draw Npoints random entries from 'points' with replacement:
  for (i = 0; i < Npoints; i++) {
    int N = Npoints * drand48();
    sample[i] = points[N];
  }
  return TRUE;
}

// calculate sigma values for the 5 fit parameters
int BootstrapRobustStats (FitAstromResult *result, FitAstromResult *fit, int Nfit, int mode) {

  // generate a histogram for the selected element
  double *values = NULL;
  ALLOCATE (values, double, Nfit);
  
  int i;

  for (i = 0; i < Nfit; i++) {
    switch (mode) {
      case FIT_RESULT_RA:
	values[i] = fit[i].Ro;
	break;
      case FIT_RESULT_DEC:
	values[i] = fit[i].Do;
	break;
      case FIT_RESULT_uR:
	values[i] = fit[i].uR;
	break;
      case FIT_RESULT_uD:
	values[i] = fit[i].uD;
	break;
      case FIT_RESULT_PLX:
	values[i] = fit[i].p;
	break;
      default:
	myAbort ("invalid option");
    }
  }

  dsort (values, Nfit);

# if (0)
  double median;
  if (Nfit % 2) {
    int Ncenter = Nfit / 2;
    median = values[Ncenter];
  } else {
    int Ncenter = Nfit / 2 - 1;
    median = 0.5*(values[Ncenter] + values[Ncenter + 1]);
  }
# endif

  double Slo = VectorFractionInterpolate (values, 0.158655, Nfit);
  double Shi = VectorFractionInterpolate (values, 0.841345, Nfit);
  double sigma = (Shi - Slo) / 2.0;

  switch (mode) {
    case FIT_RESULT_RA:
      // result->Ro = median;
      result->dRo = MAX(sigma, result->dRo);
      break;
    case FIT_RESULT_DEC:
      // result->Do = median;
      result->dDo = MAX(sigma, result->dDo);
      break;
    case FIT_RESULT_uR:
      // result->uR = median;
      result->duR = MAX(sigma, result->duR);
      break;
    case FIT_RESULT_uD:
      // result->uD = median;
      result->duD = MAX(sigma, result->duD);
      break;
    case FIT_RESULT_PLX:
      // result->p = median;
      result->dp = MAX(sigma, result->dp);
      break;
    default:
      myAbort ("invalid option");
  }
  free (values);

  return TRUE;
}

double VectorFractionInterpolate (double *values, float fraction, int Npts) {

  float F = fraction * Npts;
  int   N = fraction * Npts;

  if (N < 0        ) return NAN;
  if (N >= Npts - 2) return NAN;

  // interpolate between N,N+1
    
  double S = (F - N) * (values[N+1] - values[N]) + values[N];
  return S;
}
