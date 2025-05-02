# include "setastrom.h"

// astrometry correction functions:
// KH correction
// DCR correction

/* evaluate spline for x, y, y2 at X */
double spline_apply_dbl (double *x, double *y, double *y2, int N, double X) {

  int i, lo, hi;
  double dx, a, b, value;
  
  // saturate correction at high and low ends
  if (X < x[0]) return y[0];
  if (X > x[N-1]) return y[N-1];

  /* find correct element in array (x must be sorted) */
  lo = 0;
  hi = N-1;
  while (hi - lo > 1) {
    i = 0.5*(hi+lo);
    if (x[i] > X) {
      hi = i;
    } else {
      lo = i;
    }
  }

  /* error condition: duplicate abssisca */
  dx = x[hi] - x[lo];
  if (dx == 0.0) {
    return (HUGE_VAL);
  }

  /* evaluate spline */
  a = (x[hi] - X) / dx;
  b = (X - x[lo]) / dx;

  value = a*y[lo] + b*y[hi] + ((a*a*a - a)*y2[lo] + (b*b*b - b)*y2[hi])*(dx*dx) / 6.0;
  return (value);

}
