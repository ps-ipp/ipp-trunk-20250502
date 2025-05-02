# include "gophot.h"

makenoise () {

  int i, j;
  float *valN, *valB;

  valN = noise;
  valB = big;

  for (i = 0; i < nslow; i++) {
    if ((i < nbadbot) || (i > nslow - nbadtop - 1)) {
      for (j = 0; j < nfast; j++, valN++, valB++) *valN = MAGIC;
      continue;
    }      
    for (j = 0; j < nfast; j++, valN++, valB++) {
      if (j < nbadleft) {
	*valN = MAGIC;
	continue;
      }
      if (j > nfast - nbadright - 1) {
	*valN = MAGIC;
	continue;
      }
      *valB *= eperdn;
      if (*valB > itop) {
	*valN = MAGIC;
	continue;
      }
      if (*valB <= ibot) {
	*valN = MAGIC;
	continue;
      }
      *valN = rnoise;
    }
  }
}

/* arguments OK */


/* NEW CHOICES FOR NOISE AND BIG:

   'big' is converted to electrons - this is the Poisson component of the error
   'noise' will contain just the SQ(read-noise) - this is added to when a star is subtracted

*/
