# include "gophot.h"

bool oblit (float *star) {

  bool value;
	
  float wx, wy, dum, duma[NPMAX];
  int ix, iy, ixhi, ixlo, iyhi, iylo, jx, jy;
	
  dum = guess2 (duma, star, &ix, &iy);
  wx = star[4];				
  wy = star[6];

  ixhi = MIN ((int)(ix + 0.5*wx + 0.5), nfast-1);
  ixlo = MAX ((int)(ix - 0.5*wx + 0.5), 0);
  iyhi = MIN ((int)(iy + 0.5*wy + 0.5), nslow-1);
  iylo = MAX ((int)(iy - 0.5*wy + 0.5), 0);

  mprint (3, "obliterating following region : %d - %d, %d - %d\n", ixlo, ixhi, iylo, iyhi);
  for (jy = iylo; jy <= iyhi; jy++) {
    for (jx = ixlo; jx <= ixhi; jx++) {
      big[jx+jy*nfast] = MAGIC;
      noise[jx+jy*nfast] = MAGIC;
    }
  }

  mprint (2, "obliteration: %d, %d (%f x %f)\n", ix, iy, wx, wy);

  return (TRUE);

}

	
	
