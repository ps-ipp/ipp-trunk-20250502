# include "gastro2.h"

void sort_stars_mag (StarData *stars, int N) {

# define SWAPFUNC(A,B){ StarData tmp; tmp = stars[A]; stars[A] = stars[B]; stars[B] = tmp; }
# define COMPARE(A,B)(stars[A].M < stars[B].M)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void sort_stars_X (StarData *stars, int N) {

# define SWAPFUNC(A,B){ StarData tmp; tmp = stars[A]; stars[A] = stars[B]; stars[B] = tmp; }
# define COMPARE(A,B)(stars[A].X < stars[B].X)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
