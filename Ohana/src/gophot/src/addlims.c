# include "gophot.h"

addlims (float *star, int *jrect) {

  float temp, fudgex, fudgey;
	
  if (star[1] > 0) {
    if (beta4 < 0.1) {
      temp = star[1]/nphsub - 1.0;
    } else {
      temp = pow (6*star[1]/nphsub, 0.33333);
    }
    if (star[4] > 0) {
      fudgex = sqrt(temp*star[4]*2);
    } else {
      fudgex = 1.5*irect[1]/2;
    }
    if (star[6] > 0) {
      fudgey = sqrt(temp*star[6]*2);
    } else {
      fudgey = 1.5*irect[2]/2;
    }
  } else {
    fudgex = 1.5*irect[1]/2;
    fudgey = 1.5*irect[2]/2;
  }
  /* don't extrapolate beyond 2*fit box */
  fudgex = MIN (irect[1], fudgex);
  fudgey = MIN (irect[2], fudgex);
  jrect[1] = star[2] - fudgex;
  jrect[2] = star[2] + fudgex;
  jrect[3] = star[3] - fudgey;
  jrect[4] = star[3] + fudgey;
}
