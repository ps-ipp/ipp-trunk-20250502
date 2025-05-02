# include "lightcurve.h"

// construct an index for the stars sorted by RA
void sort_stars (int **index, Star *stars, int Nstars) {
  
  int i;
  double *RAs;

  ALLOCATE (myIndex, int, Nstars);
  for (i = 0; i < Nstars; i++) {
    myIndex[i] = i;
  }

# define SWAPFUNC(A,B){ int tmp = myIndex[A]; myIndex[A] = myIndex[B]; myIndex[B] = tmp; }
# define COMPARE(A,B)(stars[myIndex[A]].RA < stars[myIndex[B]].RA)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

  *index = myIndex;
}
