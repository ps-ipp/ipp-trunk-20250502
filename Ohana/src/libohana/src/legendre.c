# include <ohana.h>
# include <gsl_utils.h>

/* some useful functions for legendre polynomials, spherical harmonics, and vector spherical harmonics */

/* Some of the functions below are derived based on code from the gnu scientific library (gsl) v1.11

   The corresponding copyright from that code is given in doc/gsl.txt

   * From:
   * specfunc/legendre_poly.c
   * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002 Gerard Jungman
   * Author:  G. Jungman

   * adaptations by Eugene Magnier
 */

/* Calculate P_m^m(x) from the analytic result:
 *   P_m^m(x) = (-1)^m (2m-1)!! (1-x^2)^(m/2) , m > 0
 *            = 1 , m = 0
 * from gsl v1.11 specfunc/legendre_poly.c:
 */
static double legendre_Pmm(int m, double x) {

  if (m == 0) return 1.0;

  double value = 1.0;
  double factor = sqrt(1.0 - x) * sqrt(1.0 + x);
  double coeff = 1.0;

  int i;
  for (i = 1; i <= m; i++) {
    value *= -coeff * factor;
    coeff += 2.0;
  }
  return value;
}

/** these functions also return the accumulated floating-point error if requested  */

static double legendre_P0(double x, double *err) {

  double value = 1.0;
  if (err) *err = 0.0;

  return value;
}

static double legendre_P1(double x, double *err) {

  double value = x;
  if (err) *err = 0.0;

  return value;
}

static double legendre_P2(double x, double *err) {

  double value = 0.5*(3.0*x*x - 1.0);
  if (err) *err = GSL_DBL_EPSILON * (fabs(3.0*x*x) + 1.0);

  return value;
}

static double legendre_P3(double x, double *err) {

  double value = 0.5*x*(5.0*x*x - 3.0);
  if (err) *err = GSL_DBL_EPSILON * (fabs(value) + 0.5 * fabs(x) * (fabs(5.0*x*x) + 3.0));

  return value;
}

double legendre_Pl (int l, double x, double *err) { 

  double value;

  if (l < 0) return NAN;
  if (x < -1.0) return NAN;
  if (x > +1.0) return NAN;

  // pre-defined l-values:
  if (l == 0) {
    value = legendre_P0(x, err);
    return value;
  }
  if (l == 1) {
    value = legendre_P1(x, err);
    return value;
  }
  if (l == 2) {
    value = legendre_P2(x, err);
    return value;
  }
  if (l == 3) {
    value = legendre_P3(x, err);
    return value;
  }

  if (x == 1.0) {
    value = 1.0;
    if (err) *err = 0.0;
    return value;
  }
  if (x == -1.0) {
    result->val = (GSL_IS_ODD(l) ? -1.0 : 1.0);
    if (err) *err = 0.0;
    return value;
  }

  if (l < 100000) {
    /* upward recurrence: l P_l = (2l-1) z P_{l-1} - (l-1) P_{l-2} */

    double p_LM2 = 1.0;    /* P_0(x) */
    double p_LM1 = x;      /* P_1(x) */
    double p_L = p_LM1;

    double e_LM2 = GSL_DBL_EPSILON;
    double e_LM1 = fabs(x)*GSL_DBL_EPSILON;
    double e_L = e_LM1;

    int L;
    for (L = 2; L <= l; L++){
      p_L = (x*(2*L-1)*p_LM1 - (L-1)*p_LM2) / L;
      p_LM2 = p_LM1;
      p_LM1 = p_L;

      if (err) {
	e_L = 0.5*(fabs(x)*(2*L-1.0) * e_LM1 + (L-1.0)*e_LM2)/L;
	e_LM2 = e_LM1;
	e_LM1 = e_L;
      }
    }

    value = p_L;
    if (err) *err = e_L + l*fabs(p_L)*GSL_DBL_EPSILON;
    return value;
  }

  /* last resort : Asymptotic expansion.
   * [Olver, p. 473]
   */
# if (0)
  double u  = l + 0.5;
  double th = acos(x);
  gsl_sf_result J0;
  gsl_sf_result Jm1;
  int stat_J0  = gsl_sf_bessel_J0_e(u*th, &J0);
  int stat_Jm1 = gsl_sf_bessel_Jn_e(-1, u*th, &Jm1);
  double pre;
  double B00;
  double c1;

  /* B00 = 1/8 (1 - th cot(th) / th^2
   * pre = sqrt(th/sin(th))
   */
  if(th < GSL_ROOT4_DBL_EPSILON) {
    B00 = (1.0 + th*th/15.0)/24.0;
    pre = 1.0 + th*th/12.0;
  }
  else {
    double sin_th = sqrt(1.0 - x*x);
    double cot_th = x / sin_th;
    B00 = 1.0/8.0 * (1.0 - th * cot_th) / (th*th);
    pre = sqrt(th/sin_th);
  }

  c1 = th/u * B00;

  result->val  = pre * (J0.val + c1 * Jm1.val);
  result->err  = pre * (J0.err + fabs(c1) * Jm1.err);
  result->err += GSL_SQRT_DBL_EPSILON * fabs(result->val);

  return GSL_ERROR_SELECT_2(stat_J0, stat_Jm1);
# else
  fprintf (stderr, "legendre polynomials are not defined for l >= 10000\n");
  return NAN;
# endif
}

/* If l is large and m is large, then we have to worry
 * about overflow. Calculate an approximate exponent which
 * measures the normalization of this thing.
 */

double legendre_Plm (int l, int m, double x, double *err) {

  // check on the domain
  if (m < 0) return NAN;
  if (l < m) return NAN;
  if (x < -1.0) return NAN;
  if (x > +1.0) return NAN;

  const double dif = l-m;
  const double sum = l+m;
  const double t_d = ( dif == 0.0 ? 0.0 : 0.5 * dif * (log(dif)-1.0) );
  const double t_s = ( dif == 0.0 ? 0.0 : 0.5 * sum * (log(sum)-1.0) );
  const double exp_check = 0.5 * log(2.0*l+1.0) + t_d - t_s;

  if (exp_check < GSL_LOG_DBL_MIN + 10.0) return NAN;

  /* Account for the error due to the
   * representation of 1-x.
   */
  const double err_amp = 1.0 / (GSL_DBL_EPSILON + fabs(1.0-fabs(x)));

  /* P_m^m(x) and P_{m+1}^m(x) */
  double p_mm   = legendre_Pmm(m, x);
  double p_mmp1 = x * (2*m + 1) * p_mm;

  double value;

  if (l == m){
    value = p_mm;
    if (err) *err = err_amp * 2.0 * GSL_DBL_EPSILON * fabs(p_mm);
    return value;
  }

  if (l == m + 1) {
    value = p_mmp1;
    if (err) *err = err_amp * 2.0 * GSL_DBL_EPSILON * fabs(p_mmp1);
    return value;
  }

  /* upward recurrence: (l-m) P(l,m) = (2l-1) z P(l-1,m) - (l+m-1) P(l-2,m)
   * start at P(m,m), P(m+1,m)
   */

  double p_LM2 = p_mm;
  double p_LM1 = p_mmp1;
  double p_L = 0.0;

  int L;
  for (L = m + 2; L <= l; L++){
    p_L = (x*(2*L-1)*p_LM1 - (L+m-1)*p_LM2) / (L-m);
    p_LM2 = p_LM1;
    p_LM1 = p_L;
  }
  value = p_L;

  if (err) *err = err_amp * (0.5*(l-m) + 1.0) * GSL_DBL_EPSILON * fabs(p_L);
  
  return value;
}

double legendre_Plm_sphere(int l, int m, double x, double *err) {

  // check on the domain
  if (m < 0) return NAN;
  if (l < m) return NAN;
  if (x < -1.0) return NAN;
  if (x > +1.0) return NAN;

  if (m == 0) {

    double Pvalue = legendre_Pl (l, x, err);

    double pre = sqrt((2.0*l + 1.0)/(4.0*M_PI));

    double value  = pre * Pvalue;
    if (err) {
      double tmp_err = pre * *err;
      *err = tmp_err + 2.0 * GSL_DBL_EPSILON * fabs(value);
    }
    return value;
  }

  if (x == 1.0 || x == -1.0) {
    /* m > 0 here */
    double value = 0.0;
    if (err) *err = 0.0;
    return value;
  }


  /* m > 0 and |x| < 1 here */

  /* Starting value for recursion.
   * Y_m^m(x) = sqrt( (2m+1)/(4pi m) gamma(m+1/2)/gamma(m) ) (-1)^m (1-x^2)^(m/2) / pi^(1/4)
   */

  const double sgn = ( GSL_IS_ODD(m) ? -1.0 : 1.0);
  const double y_mmp1_factor = x * sqrt(2.0*m + 3.0);

  double y_mmp1, y_mmp1_err;

  double *errptr = NULL;
  double lncirc_err, lnpoch_err;

  // NOTE: only evaluate the error if requested
  if (err) errptr = &lncirc_err
  double lncirc = ohana_gsl_sf_log_1plusx(-x*x, errptr);

  if (err) errptr = &lnpoch_err
  double lnpoch = ohana_gsl_sf_lnpoch(m, 0.5, errptr);  /* Gamma(m+1/2)/Gamma(m) */

  double lnpre_val = -0.25*M_LNPI + 0.5 * (lnpoch + m*lncirc);
  double lnpre_err = NAN;
  
  if (err) {
    lnpre_err = 0.25*M_LNPI*GSL_DBL_EPSILON + 0.5 * (lnpoch_err + fabs(m)*lncirc_err);
  }

  /* Compute exp(ln_pre) with error term, avoiding call to gsl_sf_exp_err BJG */
  double ex_pre_val = exp(lnpre_val);

  double ex_pre_err = NAN;
  if (err) {
    ex_pre_err = 2.0*(sinh(lnpre_err) + GSL_DBL_EPSILON)*ex_pre_val;
  }

  double sr = sqrt((2.0+1.0/m)/(4.0*M_PI));
  double y_mm = sgn * sr * ex_pre_val;
  double y_mmp1 = y_mmp1_factor * y_mm;

  double y_mm_err = NAN;
  double y_mmp1_err = NAN;
  if (err) {
    y_mm_err  = 2.0 * GSL_DBL_EPSILON * fabs(y_mm) + sr * ex_pre_err;
    y_mm_err *= 1.0 + 1.0/(GSL_DBL_EPSILON + fabs(1.0-x));
    y_mmp1_err = fabs(y_mmp1_factor) * y_mm_err;
  }

  if (l == m){
    double value = y_mm;
    if (err) {
      *err = y_mm_err;
      *err += 2.0 * GSL_DBL_EPSILON * fabs(y_mm);
    }
    return value;
  }

  if (l == m + 1) {
    double value = y_mmp1;
    if (err) {
      *err  = y_mmp1_err;
      *err += 2.0 * GSL_DBL_EPSILON * fabs(y_mmp1);
    }
    return value;
  }

  double y_L = 0.0;
  double y_L_err = NAN;

  /* Compute Y_l^m, l > m+1, upward recursion on l. */
  int L;
  for(L=m+2; L <= l; L++){
    const double rat1 = (double)(L-m)/(double)(L+m);
    const double rat2 = (L-m-1.0)/(L+m-1.0);
    const double factor1 = sqrt(rat1*(2.0*L+1.0)*(2.0*L-1.0));
    const double factor2 = sqrt(rat1*rat2*(2.0*L+1.0)/(2.0*L-3.0));
    y_L = (x*y_mmp1*factor1 - (L+m-1.0)*y_mm*factor2) / (L-m);
    y_mm   = y_mmp1;
    y_mmp1 = y_L;

    if (err) {
      y_L_err = 0.5*(fabs(x*factor1)*y_mmp1_err + fabs((L+m-1.0)*factor2)*y_mm_err) / fabs(L-m);
      y_mm_err = y_mmp1_err;
      y_mmp1_err = y_L_err;
    }
  }

  value = y_L;
  if (err) *err = y_L_err + (0.5*(l-m) + 1.0) * GSL_DBL_EPSILON * fabs(y_L);

  return value;
}
