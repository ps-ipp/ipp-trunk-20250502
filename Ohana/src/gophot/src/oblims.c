# include "gophot.h"

oblims (float *star, int *jrect) {

  float temp, fudgex, fudgey;

  if (star[1] > 0) {
    /* think about this line: */
    temp = pow ((6*star[1]/nphob), 0.33333333);
    if (star[4] > 0) {
      fudgex = sqrt(temp*star[4]*2);
    } else {
      fudgex = 10;
    }
    if (star[6] > 0) {
      fudgey = sqrt(temp*star[6]*2);
    } else {
      fudgey = 10;
    }
  } else {
    fudgex = 10;
    fudgey = 10;
  }

  mprint (3, "fudge size: %f %f\n", fudgex, fudgey);
  jrect[1] = star[2] - fudgex;
  jrect[2] = star[2] + fudgex;
  jrect[3] = star[3] - fudgey;
  jrect[4] = star[3] + fudgey;

}
