# include "data.h"

void sort_opihi_flt_index (opihi_flt *X, int *IDX, int N) {

# define SWAPFUNC(A,B){ opihi_flt tmp; int itmp; 	\
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  itmp = IDX[A]; IDX[A] = IDX[B]; IDX[B] = itmp; \
}

// # define COMPARE(A,B)(X[A] < X[B])
# define COMPARE(A,B)((!isfinite(X[A]) && isfinite(X[B])) || (X[A] < X[B]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void sort_int_index (opihi_int *X, int *IDX, int N) {

# define SWAPFUNC(A,B){ opihi_int tmp; int itmp; 	\
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  itmp = IDX[A]; IDX[A] = IDX[B]; IDX[B] = itmp; \
}

# define COMPARE(A,B)(X[A] < X[B])
// # define COMPARE(A,B)((!isfinite(X[A]) && isfinite(X[B])) || (X[A] < X[B]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void sort_float_index (float *X, int *IDX, int N) {

# define SWAPFUNC(A,B){ float tmp; int itmp;		\
    tmp = X[A]; X[A] = X[B]; X[B] = tmp;		\
    itmp = IDX[A]; IDX[A] = IDX[B]; IDX[B] = itmp;	\
  }
// # define COMPARE(A,B)(X[A] < X[B])
# define COMPARE(A,B)((!isfinite(X[A]) && isfinite(X[B])) || (X[A] < X[B]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

