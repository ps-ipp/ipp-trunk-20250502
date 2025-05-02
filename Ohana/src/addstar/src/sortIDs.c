# include "addstar.h"
# include "loadgalphot.h"

/* sort a coordinate pair (X,Y) and the associated index (S) */
void sort_IDs_seqonly (int *X, int *S, int N) {
  
# define SWAPFUNC(A,B){ off_t itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)(X[S[A]] < X[S[B]])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

