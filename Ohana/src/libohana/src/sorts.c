# include <ohana.h>

/* various widely-used sort functions for specific sets of types */

void dsort (double *value, int N) {

# define SWAPFUNC(A,B){ double tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void fsort (float *value, int N) {

# define SWAPFUNC(A,B){ float tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void isort (int *value, int N) {

# define SWAPFUNC(A,B){ int tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void llsort (long long int *value, int N) {

# define SWAPFUNC(A,B){ long long int tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void dsortpair (double *X, double *Y, int N) {

# define SWAPFUNC(A,B){ double tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void fsortpair (float *X, float *Y, int N) {

# define SWAPFUNC(A,B){ float tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// sort two int vectors by first vector
void isortpair (int *X, int *Y, int N) {

# define SWAPFUNC(A,B){ int tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// sort two int vectors by first vector
void llsortpair (off_t *X, off_t *Y, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void dsortthree (double *X, double *Y, double *Z, int N) {

# define SWAPFUNC(A,B){ double tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
  tmp = Z[A]; Z[A] = Z[B]; Z[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void fsortthree (float *X, float *Y, float *Z, int N) {

# define SWAPFUNC(A,B){ float tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
  tmp = Z[A]; Z[A] = Z[B]; Z[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void dsortfour (double *X, double *Y, double *Z, double *W, int N) {

# define SWAPFUNC(A,B){ double tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
  tmp = Z[A]; Z[A] = Z[B]; Z[B] = tmp; \
  tmp = W[A]; W[A] = W[B]; W[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void fsortfour (float *X, float *Y, float *Z, float *W, int N) {

# define SWAPFUNC(A,B){ float tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
  tmp = Z[A]; Z[A] = Z[B]; Z[B] = tmp; \
  tmp = W[A]; W[A] = W[B]; W[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void isortfour (int *X, int *Y, int *Z, int *W, int N) {

# define SWAPFUNC(A,B){ int tmp; \
  tmp = X[A]; X[A] = X[B]; X[B] = tmp; \
  tmp = Y[A]; Y[A] = Y[B]; Y[B] = tmp; \
  tmp = Z[A]; Z[A] = Z[B]; Z[B] = tmp; \
  tmp = W[A]; W[A] = W[B]; W[B] = tmp; \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort the index of a vector (vector stays unsorted) */
void dsort_indexonly (double *X, off_t *S, off_t N) {
  
# define SWAPFUNC(A,B){ off_t itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)((!isfinite(X[S[A]]) && isfinite(X[S[B]])) || (X[S[A]] < X[S[B]]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort the index of a vector (vector stays unsorted) */
void dsort_int_indexonly (double *X, int *S, int N) {
  
# define SWAPFUNC(A,B){ int itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)((!isfinite(X[S[A]]) && isfinite(X[S[B]])) || (X[S[A]] < X[S[B]]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}


