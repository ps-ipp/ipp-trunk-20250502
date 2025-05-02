# include "gophot.h"

# define HALF 0.5

float skyfun_plane (int ix, int iy, float *a, float *fa) {
	
  int i;
  float value;

  fa[0] = 1.0;
  fa[1] = HALF*(ix - HALF*nfast)/(HALF*nfast);
  fa[2] = HALF*(iy - HALF*nslow)/(HALF*nslow);

  value = 0;
  for (i = 0; i < 1; i++) {
    value += a[i]*fa[i];
  }

  return (value);
}

/* this function uses C 0,N-1 for a[], fa[] */
