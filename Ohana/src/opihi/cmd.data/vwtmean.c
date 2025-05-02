# include "data.h"

double opihi_wt_mean_dbl (double *val, double *wgt, int N, double *Sigma);

int vwtmean (int argc, char **argv) {
  
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: vwtmean (values) (sigmas)\n");
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

  double Sigma = NAN;
  double Value = opihi_wt_mean_dbl (val, wgt, N, &Sigma);

  set_variable ("WTMEAN",     Value);
  set_variable ("WTSIGMA",    Sigma);
  set_int_variable ("NUSED", N);
  return (TRUE);
}

double opihi_wt_mean_dbl (double *val, double *wgt, int N, double *Sigma) {

  // calculate weighted mean
  double S1 = 0.0, S2 = 0.0;
  for (int n = 0; n < N; n++) {
    S1 += wgt[n] * val[n];
    S2 += wgt[n];
  }
  double Value = S1 / S2;
  *Sigma = 1.0 / S2;
  
  return Value;
}

