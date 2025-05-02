void lubksb (float **a, int n, int *indx, float *b) {

  int i, j, ii, kk;
  float sum;

  ii = -1;
  for (i = 0; i < n; i++) {
    kk = indx[i];
    sum = b[kk];
    b[kk] = b[i];
    if (ii != -1) {
      for (j = ii; j < i; j++) {
	sum -= a[i][j]*b[j];
      }
    } else {
      if (sum != 0) ii = i;
    }
    b[i] = sum;
  }

  for (i = n-1; i >= 0; i--) {
    sum=b[i];
    if (i < n-1) {
      for (j = i+1; j < n; j++) {
	sum -= a[i][j]*b[j];
      }
    }
    b[i] = sum/a[i][i];
  }

}
