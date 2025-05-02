# include "gophot.h"

float findsky () {

  int i, skip, nsky, bin;
  int zvalue[0x10000], nvalue;
  float median, *valB;
  
  skip = 100;
  valB = big;

  for (i = 0; i < nfast*nslow; i+=skip, valB+=skip) {
    bin = *valB;
    if ((bin >= 0) && (bin < 0x10000)) {
      nvalue ++;
      zvalue[bin] ++;
    }
  }
  
  nsky = 0;
  for (i = 0; (i < 0x10000) && (nsky < nvalue / 2); i++) {
    nsky += zvalue[i];
  }
  if (nsky < nvalue / 2) {
    mprint (1, "weird situation: no median found\n");
    exit (1);
  }
  median = i;
  mprint (1, "median = %f\n", median);

  return (median);
}

/* arguments OK */

/* this median method assumes 64k data range */
