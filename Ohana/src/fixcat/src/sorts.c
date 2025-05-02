# include "markstar.h"

void sort_seq (double *X, int *S, int N) {

# define SWAPFUNC(A,B){ \
  double dtmp = X[A]; X[A] = X[B]; X[B] = dtmp; \
  int    itmp = Y[A]; Y[A] = Y[B]; Y[B] = itmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

