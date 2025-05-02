# include "gophot.h"

bool toofaint (float *star, float *err) {

  bool value;
  float cmin, sig2;

  cmin = 1.0;
  value = (star[1] < cmin);

  if (err[1] > 0) {
    sig2 = SQ(star[1]/ufactor) / err[1];
    value = value || (sig2 < crit7);
    mprint (3, "sig2 = %f (%f %f %f)\n", sig2, star[1], ufactor, err[1]);
    mprint (3, "value, crit7: %d, %f\n", value, crit7);
  }

  return (value);

}

/* this function uses C 0,N-1 for a[], fa[] */
