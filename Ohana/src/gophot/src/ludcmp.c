# include <ohana.h>

# define TINY 1.0e-20

void ludcmp (float **a, int n, int *indx, float *D) {

  float *vv;
  int i, j, k, imax;
  float aamax, sum, dum, d;

  ALLOCATE (vv, float, n);

  d = 1.0;
  for (i = 0; i < n; i++) {
    aamax = 0.0;
    for (j = 0; j < n; j++) {
      if (fabs (a[i][j]) > aamax) aamax = fabs (a[i][j]);
    }
    if (aamax == 0.0) {
      *D = 0;
      free (vv);
      return;
    }
    vv[i] = 1.0/aamax;
  }

  for (j = 0; j < n; j++) {
    for (i = 0; i < j; i++) {
      sum = a[i][j];
      for (k = 0; k < i; k++) {
	sum -= a[i][k]*a[k][j];
      }
      a[i][j] = sum;
    }
    aamax = 0.0;
    for (i = j; i < n; i++) {
      sum = a[i][j];
      for (k = 0; k < j; k++) {
	sum -= a[i][k]*a[k][j];
      }
      a[i][j] = sum;
      dum = vv[i]*fabs(sum);
      if (dum >= aamax) {
	imax = i;
	aamax = dum;
      }
    }
    if (j != imax) {
      for (k = 0; k < n; k++) {
	dum = a[imax][k];
	a[imax][k] = a[j][k];
	a[j][k] = dum;
      }
      d = -d;
      vv[imax] = vv[j];
    }
    indx[j] = imax;
    if (j != n-1) {
      if (a[j][j] == 0.0) a[j][j] = TINY;
      dum = 1.0 / a[j][j];
      for (i = j+1; i < n; i++) {
	a[i][j] = a[i][j]*dum;
      }
    }
  }
  if (a[n-1][n-1] == 0.0) a[n-1][n-1] = TINY;
  free (vv);

  *D = d;
  return;
}
