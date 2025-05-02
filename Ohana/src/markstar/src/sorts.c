# include "markstar.h"

void sort_seq (double *X, int *S, int N) {

# define SWAPFUNC(A,B){ \
  double dtmp; dtmp = X[A]; X[A] = X[B]; X[B] = dtmp; }
  int    itmp; itmp = S[A]; S[A] = S[B]; S[B] = itmp; }
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
