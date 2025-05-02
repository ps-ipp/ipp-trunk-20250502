# include "data.h"

double opihi_irls_mean_dbl (double *val, double *wgt, int N, int *Converged);
double weight_cauchy_square_dbl (double x2);

# define IRLS_TOLERANCE 1e-4

int virls (int argc, char **argv) {
  
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: virls (values) (sigmas)\n");
    gprint (GP_ERR, "NOTE: sigmas with abs values < %e will be truncated to that value\n", FLT_MIN);
    return (FALSE);
  }

  Vector *value, *sigma;

  if ((value = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sigma = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (value->Nelements != sigma->Nelements) {
    gprint (GP_ERR, "vectors are not the same length\n");
    return FALSE;
  }
  if (value->type != OPIHI_FLT) {
    gprint (GP_ERR, "%s is not a floating point vector\n", argv[1]);
    return FALSE;
  }
  if (sigma->type != OPIHI_FLT) {
    gprint (GP_ERR, "%s is not a floating point vector\n", argv[2]);
    return FALSE;
  }

  ALLOCATE_PTR (val, opihi_flt, value->Nelements);
  ALLOCATE_PTR (wgt, opihi_flt, value->Nelements);

  int N = 0;
  for (int i = 0; i < value->Nelements; i++) {
    if (!isfinite(value->elements.Flt[i])) continue;

    double s = sigma->elements.Flt[i];
    if (!isfinite(s)) continue;

    if (fabs(s) < FLT_MIN) s = FLT_MIN;

    val[N] = value->elements.Flt[i];
    wgt[N] = 1.0 / SQ(s);
    N ++;
  }

  int converged = TRUE;
  double Value = opihi_irls_mean_dbl (val, wgt, N, &converged);

  set_variable ("WTMEAN",     Value);
  set_int_variable ("NUSED", N);
  set_int_variable ("CONVERGED", converged);
  return (TRUE);
}

double opihi_irls_mean_dbl (double *val, double *wgt, int N, int *Converged) {

  // calculate weighted mean
  double S1 = 0.0, S2 = 0.0;
  for (int n = 0; n < N; n++) {
    S1 += wgt[n] * val[n];
    S2 += wgt[n];
  }
  double Value = S1 / S2;
  
  int converged = FALSE;
  for (int i = 0; (i < 10) && !converged; i++) {
    double ValueLast = Value;

    double S1 = 0.0, S2 = 0.0;

    // calculate weight modification based on distances (squared).
    // use modifier to calculate new weighted mean
    for (int n = 0; n < N; n++) {
      double dV = (val[n] - Value);
      double d2 = SQ(dV) * wgt[n];
      
      double Mod = weight_cauchy_square_dbl (d2);
      S1 += Mod * wgt[n] * val[n];
      S2 += Mod * wgt[n];
      fprintf (stderr, "%f %f : %f %f : %f\n", val[n], wgt[n], dV, sqrt(d2), Mod);
    }
    Value = S1 / S2;

    double delta = fabs(Value - ValueLast);
    if (delta < Value * IRLS_TOLERANCE) converged = TRUE;

    // XXX if the answer is close to zero, we might not converge
  }
  *Converged = converged;
  return Value;
}

# define CAUCY_FACTOR 1.0

double weight_cauchy_square_dbl (double x2) {
  double r2 = x2 / CAUCY_FACTOR;
  return (1.0 / (1.0 + r2));
}
