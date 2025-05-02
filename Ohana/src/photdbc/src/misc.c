# include "photdbc.h"

void sort_time (unsigned int *X, int *Y, int N) {

# define SWAPFUNC(A,B){ float tmp; \
  unsigned int utmp = X[A]; X[A] = X[B]; X[B] = utmp; \
  int          itmp = Y[A]; Y[A] = Y[B]; Y[B] = itmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
