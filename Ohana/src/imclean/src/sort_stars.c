# include "imclean.h"

void sort_stars (SMPData *stars, int N) {

# define SWAPFUNC(A,B){ SMPData tmp; tmp = stars[A]; stars[A] = stars[B]; stars[B] = tmp; }
# define COMPARE(A,B)(stars[A].X < stars[B].X)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
