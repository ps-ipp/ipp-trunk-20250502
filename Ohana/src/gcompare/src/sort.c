# include "gcompare.h"

void data_sort (data_type data) {

# define SWAPFUNC(A,B){ value_type tmp; tmp = data.values[A]; data.values[A] = data.values[B]; data.values[B] = tmp; }
# define COMPARE(A,B)(data.values[A].X < data.values[B].X)

  OHANA_SORT (data.Nvalues, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
