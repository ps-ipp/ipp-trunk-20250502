# include "data.h"

/* construct the natural spline for x, y in y2 */
void spline_construct_flt (float *x, float *y, int N, float *y2, float dyLower, float dyUpper) {

  int i;
  float dy, dx, *tmp;
  
  // spline is not valid with < 3 points
  if (N < 3) return;

  ALLOCATE (tmp, float, N);

  y2[0] = tmp[0] = 0.0;
  
  for (i = 1; i < N-1; i++) {
    dx = (x[i+0] - x[i-1]) / (x[i+1] - x[i-1]);
    dy = dx * y2[i-1] + 2.0;
    y2[i] = (dx - 1.0) / dy;
    tmp[i] = (y[i+1] - y[i+0]) / (x[i+1] - x[i+0]) - (y[i+0] - y[i-1]) / (x[i+0] - x[i-1]);
    tmp[i] = (6.0 * tmp[i] / (x[i+1] - x[i-1]) - dx*tmp[i-1]) / dy;
  }
  
  y2[N-1] = 0;
  for (i = N-2; i >= 1; i--)
    y2[i] = y2[i]*y2[i+1] + tmp[i];

  free (tmp);
}

/* evaluate spline for x, y, y2 at X */
float spline_apply_flt (float *x, float *y, float *y2, int N, float X) {

  int i, lo, hi;
  float dx, a, b, value;
  
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

/* construct the natural spline for x, y in y2 */
void spline_construct_dbl (opihi_flt *x, opihi_flt *y, int N, opihi_flt *y2, opihi_flt dyLower, opihi_flt dyUpper) {

  int i;
  opihi_flt dy, dx, *tmp;
  
  // spline is not valid with < 3 points
  if (N < 3) return;

  ALLOCATE (tmp, opihi_flt, N);

  if (isnan(dyLower)) {
    y2[0] = tmp[0] = 0.0;
  } else {
    y2[0] = -0.5;
    tmp[0]   = (3.0/(x[1]-x[0])) * ((y[1]-y[0])/(x[1]-x[0]) - dyLower);
  }
  
  for (i = 1; i < N-1; i++) {
    dx = (x[i+0] - x[i-1]) / (x[i+1] - x[i-1]);
    dy = dx * y2[i-1] + 2.0;
    y2[i] = (dx - 1.0) / dy;
    tmp[i] = (y[i+1] - y[i+0]) / (x[i+1] - x[i+0]) - (y[i+0] - y[i-1]) / (x[i+0] - x[i-1]);
    tmp[i] = (6.0 * tmp[i] / (x[i+1] - x[i-1]) - dx*tmp[i-1]) / dy;
  }

  if (isfinite(dyUpper)) {
    opihi_flt qn = 0.5;
    tmp[N-1] = (3.0/(x[N-1]-x[N-2])) * (dyUpper - (y[N-1]-y[N-2])/(x[N-1]-x[N-2]));
    y2[N-1] = (tmp[N-1] - (qn * tmp[N-2])) / ((qn * y2[N-2]) + 1.0);
  } else {
    y2[N-1] = 0;
  }
  
  for (i = N-2; i >= 0; i--) {
    y2[i] = y2[i]*y2[i+1] + tmp[i];
  }

  free (tmp);
}

/* evaluate spline for x, y, y2 at X */
opihi_flt spline_apply_dbl (opihi_flt *x, opihi_flt *y, opihi_flt *y2, int N, opihi_flt X) {

  int i, lo, hi;
  opihi_flt dx, a, b, value;
  
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
