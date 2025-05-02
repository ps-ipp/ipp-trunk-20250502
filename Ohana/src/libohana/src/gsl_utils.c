# include <ohana.h>
# include <gsl_utils.h>

/*-*-*-*-*-*-*-*-*-*-*-* Private Section *-*-*-*-*-*-*-*-*-*-*-*/

/* Chebyshev expansion for log(1 + x(t))/x(t)
 *
 * x(t) = (4t-1)/(2(4-t))
 * t(x) = (8x+1)/(2(x+2))
 * -1/2 < x < 1/2
 * -1 < t < 1
 */
static double log_1plusx_data[21] = {
  2.16647910664395270521272590407,
 -0.28565398551049742084877469679,
  0.01517767255690553732382488171,
 -0.00200215904941415466274422081,
  0.00019211375164056698287947962,
 -0.00002553258886105542567601400,
  2.9004512660400621301999384544e-06,
 -3.8873813517057343800270917900e-07,
  4.7743678729400456026672697926e-08,
 -6.4501969776090319441714445454e-09,
  8.2751976628812389601561347296e-10,
 -1.1260499376492049411710290413e-10,
  1.4844576692270934446023686322e-11,
 -2.0328515972462118942821556033e-12,
  2.7291231220549214896095654769e-13,
 -3.7581977830387938294437434651e-14,
  5.1107345870861673561462339876e-15,
 -7.0722150011433276578323272272e-16,
  9.7089758328248469219003866867e-17,
 -1.3492637457521938883731579510e-17,
  1.8657327910677296608121390705e-18
};

static cheb_series log_1plusx_cs = {
  log_1plusx_data,
  20,
  -1, 1,
  10
};

static inline double ohana_gsl_cheb_eval(const cheb_series *cs, const double x, double *err) {

  double d  = 0.0;
  double dd = 0.0;

  double y  = (2.0*x - cs->a - cs->b) / (cs->b - cs->a);
  double y2 = 2.0 * y;

  double e = 0.0;

  int j;
  for (j = cs->order; j >= 1; j--) {
    double temp = d;
    d = y2*d - dd + cs->c[j];
    e += fabs(y2*temp) + fabs(dd) + fabs(cs->c[j]);
    dd = temp;
  }

  { 
    double temp = d;
    d = y*d - dd + 0.5 * cs->c[0];
    e += fabs(y*temp) + fabs(dd) + 0.5 * fabs(cs->c[0]);
  }

  double value = d;
  if (err) *err = GSL_DBL_EPSILON * e + fabs(cs->c[cs->order]);

  return value;
}

/*** end chebyshev approximation code *******************************************/

double ohana_gsl_sf_log_1plusx (double x, double *err) {

  if (x <= -1.0) return NAN;

  if (fabs(x) < GSL_ROOT6_DBL_EPSILON) {
    const double c1 = -0.5;
    const double c2 =  1.0/3.0;
    const double c3 = -1.0/4.0;
    const double c4 =  1.0/5.0;
    const double c5 = -1.0/6.0;
    const double c6 =  1.0/7.0;
    const double c7 = -1.0/8.0;
    const double c8 =  1.0/9.0;
    const double c9 = -1.0/10.0;
    const double t  =  c5 + x*(c6 + x*(c7 + x*(c8 + x*c9)));
    double value = x * (1.0 + x*(c1 + x*(c2 + x*(c3 + x*(c4 + x*t)))));
    if (err) *err = GSL_DBL_EPSILON * fabs(value);
    return value;
  }

  if (fabs(x) < 0.5) {
    double t = 0.5*(8.0*x + 1.0)/(x+2.0);

    // log_1plusx_cs is a static cheb_series structure
    double c = ohana_gsl_cheb_eval (&log_1plusx_cs, t, err);

    double value = x * c;

    if (err) *err = fabs(x * *err); 

    return value;
  }

  double value = log(1.0 + x);

  if (err) *err = GSL_DBL_EPSILON * fabs(value);

  return value;
}

double ohana_gsl_sf_lnpoch(const double a, const double x, double *err) {

  if (a   <= 0.0) return NAN;
  if (a+x <= 0.0) return NAN;

  if (x == 0.0) {
    double value = 0.0;
    if (err) *err = 0.0;
    return value;
  }

  double value = ohana_gsl_lnpoch_pos (a, x, err);
  return value;
}

/* Assumes a>0 and a+x>0.
 */
double ohana_gsl_lnpoch_pos (const double a, const double x, double *err) {

  double absx = fabs(x);

  if (absx > 0.1*a || absx*log(MAX(a,2.0)) > 0.1) {
    if(a < GSL_SF_GAMMA_XMAX && a+x < GSL_SF_GAMMA_XMAX) {
      /* If we can do it by calculating the gamma functions
       * directly, then that will be more accurate than
       * doing the subtraction of the logs.
       */

      double g1_error = NAN;
      double g2_error = NAN;
      double *ptrerr = NULL;

      if (err) ptrerr = &g1_error;
      double g1_value = ohana_gsl_sf_gammainv(a, ptrerr);

      if (err) ptrerr = &g2_error;
      double g2_value = ohana_gsl_sf_gammainv(a+x, ptrerr);

      value = -log(g2_value / g1_value);

      if (err) {
	double tmp_err = (g1_error / fabs(g1_value)) + (g2_error / fabs(g2_value));
	tmp_err += 2.0 * GSL_DBL_EPSILON * fabs(value);
	*err = tmp_err;
      }
      return value;
    }

    /* Otherwise we must do the subtraction.
     */
    double lg1_error = NAN;
    double lg2_error = NAN;
    double *ptrerr = NULL;

    if (err) ptrerr = &lg1_error;
    double lg1_value = ohana_gsl_sf_lngamma(a, ptrerr);

    if (err) ptrerr = &lg2_error;
    double lg2_value = ohana_gsl_sf_lngamma(a+x, ptrerr);

    double value = lg2_value - lg1_value;

    if (err) {
      double tmp_err = lg2_error + lg1_error;
      tmp_err += 2.0 * GSL_DBL_EPSILON * fabs(value);
      *err = tmp_err;
    }
    return value;
  }

  if ((absx < 0.1*a) && (a > 15.0)) {
    /* Be careful about the implied subtraction.
     * Note that both a+x and and a must be
     * large here since a is not small
     * and x is not relatively large.
     * So we calculate using Stirling for Log[Gamma(z)].
     *
     *   Log[Gamma(a+x)/Gamma(a)] = x(Log[a]-1) + (x+a-1/2)Log[1+x/a]
     *                              + (1/(1+eps)   - 1) / (12 a)
     *                              - (1/(1+eps)^3 - 1) / (360 a^3)
     *                              + (1/(1+eps)^5 - 1) / (1260 a^5)
     *                              - (1/(1+eps)^7 - 1) / (1680 a^7)
     *                              + ...
     */
    const double eps = x/a;
    const double den = 1.0 + eps;
    const double d3 = den*den*den;
    const double d5 = d3*den*den;
    const double d7 = d5*den*den;
    const double c1 = -eps/den;
    const double c3 = -eps*(3.0+eps*(3.0+eps))/d3;
    const double c5 = -eps*(5.0+eps*(10.0+eps*(10.0+eps*(5.0+eps))))/d5;
    const double c7 = -eps*(7.0+eps*(21.0+eps*(35.0+eps*(35.0+eps*(21.0+eps*(7.0+eps))))))/d7;

    const double p8 = ohana_gsl_sf_pow_int(1.0+eps,8);
    const double c8 = 1.0/p8             - 1.0;  /* these need not   */
    const double c9 = 1.0/(p8*(1.0+eps)) - 1.0;  /* be very accurate */
    const double a4 = a*a*a*a;
    const double a6 = a4*a*a;
    const double ser_1 = c1 + c3/(30.0*a*a) + c5/(105.0*a4) + c7/(140.0*a6);
    const double ser_2 = c8/(99.0*a6*a*a) - 691.0/360360.0 * c9/(a6*a4);
    const double ser = (ser_1 + ser_2)/ (12.0*a);

    double term1 = x * log(a/M_E);
    double term2;

    double ln_1peps_error = NAN;
    double *ptr = err ? &ln_1peps_error : NULL;

    double ln_1peps_value = ohana_gsl_sf_log_1plusx(eps, ptr);  /* log(1 + x/a) */
    term2 = (x + a - 0.5) * ln_1peps_value;

    double value = term1 + term2 + ser;
    
    if (err) {
      double tmp_err = GSL_DBL_EPSILON*fabs(term1);
      tmp_err += fabs((x + a - 0.5)*ln_1peps_error);
      tmp_err += fabs(ln_1peps_value) * GSL_DBL_EPSILON * (fabs(x) + fabs(a) + 0.5);
      tmp_err += 2.0 * GSL_DBL_EPSILON * fabs(value);
      *err = tmp_err;
    }
    return value;
  }

  double poch_rel_error = NAN;
  double *ptr = err ? &poch_rel_error : NULL;
  double poch_rel_value = ohana_gsl_pochrel_smallx (a, x, ptr);

  double eps = x*poch_rel_value;

  double value = ohana_gsl_sf_log_1plusx(eps, NULL);

  if (err) {
    double tmp_err  = 2.0 * fabs(x * poch_rel_error / (1.0 + eps));
    tmp_err += 2.0 * GSL_DBL_EPSILON * fabs(value);
    *err = tmp_err;
  }

  return value;
}

/* ((a)_x - 1)/x in the "small x" region where
 * cancellation must be controlled.
 *
 * Based on SLATEC DPOCH1().
 */
/*
C When ABS(X) is so small that substantial cancellation will occur if
C the straightforward formula is used, we use an expansion due
C to Fields and discussed by Y. L. Luke, The Special Functions and Their
C Approximations, Vol. 1, Academic Press, 1969, page 34.
C
C The ratio POCH(A,X) = GAMMA(A+X)/GAMMA(A) is written by Luke as
C        (A+(X-1)/2)**X * polynomial in (A+(X-1)/2)**(-2) .
C In order to maintain significance in POCH1, we write for positive a
C        (A+(X-1)/2)**X = EXP(X*LOG(A+(X-1)/2)) = EXP(Q)
C                       = 1.0 + Q*EXPREL(Q) .
C Likewise the polynomial is written
C        POLY = 1.0 + X*POLY1(A,X) .
C Thus,
C        POCH1(A,X) = (POCH(A,X) - 1) / X
C                   = EXPREL(Q)*(Q/X + Q*POLY1(A,X)) + POLY1(A,X)
C
*/
double ohana_gsl_pochrel_smallx(const double a, const double x, double *err) {
  /*
   SQTBIG = 1.0D0/SQRT(24.0D0*D1MACH(1))
   ALNEPS = LOG(D1MACH(3))
   */
  const double SQTBIG = 1.0/(2.0*M_SQRT2*M_SQRT3*GSL_SQRT_DBL_MIN);
  const double ALNEPS = GSL_LOG_DBL_EPSILON - M_LN2;

  if (x == 0.0) {
    double value = ohana_gsl_sf_psi(a, err);
    return value;
  }

  const double bp   = (  (a < -0.5) ? 1.0-a-x : a );
  const int    incr = ( (bp < 10.0) ? 11.0-bp : 0 );
  const double b    = bp + incr;

  double var    = b + 0.5*(x-1.0);
  double alnvar = log(var);
  double q = x*alnvar;
  
  double poly1 = 0.0;
  
  if (var < SQTBIG) {
    const int nterms = (int)(-0.5*ALNEPS/alnvar + 1.0);
    const double var2 = (1.0/var)/var;
    const double rho  = 0.5 * (x + 1.0);
    double term = var2;
    double gbern[24];
    int k, j;

    gbern[1] = 1.0;
    gbern[2] = -rho/12.0;
    poly1 = gbern[2] * term;

    if (nterms > 20) {
      /* NTERMS IS TOO BIG, MAYBE D1MACH(3) IS BAD */
      /* nterms = 20; */
      value = NAN;
      if (err) *err = NAN;
      return value;
    }

    for (k = 2; k <= nterms; k++) {
      double gbk = 0.0;
      for (j = 1; j <= k; j++) {
	gbk += bern[k-j+1]*gbern[j];
      }
      gbern[k+1] = -rho*gbk/k;

      term  *= (2*k-2-x)*(2*k-1-x)*var2;
      poly1 += gbern[k+1]*term;
    }
  }

  double dexprl = ohana_gsl_sf_expm1(q);
  if (!isfinite(dexprl)) {
    double value = NAN;
    if (err) *err = NAN;
    return value;
  }

  dexprl = dexprl / q;
  poly1 *= (x - 1.0);
  double dpoch1 = dexprl * (alnvar + q * poly1) + poly1;

  int i;
  for (i = incr-1; i >= 0; i--) {
    /*
      C WE HAVE DPOCH1(B,X), BUT BP IS SMALL, SO WE USE BACKWARDS RECURSION
      C TO OBTAIN DPOCH1(BP,X).
    */
    double binv = 1.0/(bp+i);
    dpoch1 = (dpoch1 - binv) / (1.0 + x*binv);
  }

  if (bp == a) {
    double value = dpoch1;
    if (err) *err = 2.0 * GSL_DBL_EPSILON * (fabs(incr) + 1.0) * fabs(value);
    return value;
  }

  /*
    C WE HAVE DPOCH1(BP,X), BUT A IS LT -0.5.  WE THEREFORE USE A
    C REFLECTION FORMULA TO OBTAIN DPOCH1(A,X).
  */
  double sinpxx = sin(M_PI*x)/x;
  double sinpx2 = sin(0.5*M_PI*x);
  double t1 = sinpxx/tan(M_PI*b);
  double t2 = 2.0*sinpx2*(sinpx2/x);
  double trig  = t1 - t2;
  double value = dpoch1 * (1.0 + x*trig) + trig;

  if (err) {
    double tmp_err  = (fabs(dpoch1*x) + 1.0) * GSL_DBL_EPSILON * (fabs(t1) + fabs(t2));
    tmp_err += 2.0 * GSL_DBL_EPSILON * (fabs(incr) + 1.0) * fabs(value);
    *err = tmp_err;
  }
  return value;
}

/* functions I still need to define: */

ohana_gsl_sf_gammainv
ohana_gsl_sf_lngamma
ohana_gsl_sf_pow_int
ohana_gsl_sf_psi
ohana_gsl_sf_expm1
