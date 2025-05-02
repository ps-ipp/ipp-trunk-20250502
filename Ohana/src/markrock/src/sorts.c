# include "markrock.h"

void sort_set (double *X, double *Y, int *T, int *S, int N) {

# define SWAPFUNC(A,B){ double dtmp; int itmp; \
  dtmp = X[A]; X[A] = X[B]; X[B] = dtmp; \
  dtmp = Y[A]; Y[A] = Y[B]; Y[B] = dtmp; \
  itmp = T[A]; T[A] = T[B]; T[B] = itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
