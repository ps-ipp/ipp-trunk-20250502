# include "uniphot.h"

void sort_time (unsigned int *value, int N) {

# define SWAPFUNC(A,B){ unsigned int tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
