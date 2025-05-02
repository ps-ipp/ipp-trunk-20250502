# define GSL_DBL_EPSILON   2.2204460492503131e-16
# define GSL_LOG_DBL_MIN (-7.0839641853226408e+02)

# define GSL_ROOT6_DBL_EPSILON  2.4607833005759251e-03

# define GSL_SF_GAMMA_XMAX  171.0

#define GSL_SQRT_DBL_MIN   1.4916681462400413e-154
#define GSL_LOG_DBL_EPSILON   (-3.6043653389117154e+01)
#ifndef M_SQRT3
#define M_SQRT3    1.73205080756887729352744634151      /* sqrt(3) */
#endif
#ifndef M_LNPI
#define M_LNPI     1.14472988584940017414342735135      /* ln(pi) */
#endif

# define GSL_IS_ODD(n) ((n) & 1)

typedef struct {
  double *c;    /* coefficients                */
  int order;    /* order of expansion          */
  double a;     /* lower interval point        */
  double b;     /* upper interval point        */
  int order_sp; /* effective single precision order */
} cheb_series;

