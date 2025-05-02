# include "gophot.h"

bool toobright (float *star) {

  bool value;
  bool flag;
  int jrect[5];
  int nsat, nbad, ix, iy, ixhi, ixlo, iyhi, iylo, jx, jy;
  float dum, duma[NPMAX];

  value = FALSE;
	
  if (star[1] < itop/4) return (FALSE);

  nsat = 0;
  nbad = 0;
  dum = guess2 (duma, star, &ix, &iy);
  ixhi = MIN ((int)(ix + krect[1]/2 + 0.5), nfast - 1);
  ixlo = MAX ((int)(ix - krect[1]/2 + 0.5), 0);
  iyhi = MIN ((int)(iy + krect[2]/2 + 0.5), nslow - 1);
  iylo = MAX ((int)(iy - krect[2]/2 + 0.5), 0);

  for (jy = iylo; jy <= iyhi; jy++) {
    for (jx = ixlo; jx <= ixhi; jx++) {
      if (!finite (noise[jx+jy*nfast])) nbad ++;
      if (big[jx+jy*nfast] >= itop) nsat ++;
    }
  }

  value = (star[1] > cmax) || (nsat >= icrit) || (nbad >= icrit);

  mprint (3, "object at: %d, %d - %d:  %d %d %f %f %f\n", ix,iy, value, nsat, nbad, star[1], big[ix+iy*nfast], noise[ix+iy*nfast]);

  if (value) {
    if (nsat >= icrit)  star[1] = ctpersat*nsat;
    
    oblims(star,jrect);
    
    star[4] = jrect[2] - jrect[1];
    star[5] = nsat;
    star[6] = jrect[4] - jrect[3];
    
    mprint (0, "%d, %d funny pixels in object at %d, %d (%f peak)\n", nsat, nbad, ix, iy, star[1]);
  }

  return (value);
}

bool verybright (float *star) {

  float flux;

  /* rough scaling to compare fits with different scale */

  flux = star[1]*sqrt((star[4]*star[6])/(ava[4]*ava[6]));;

  if (flux > cmax) {
    return (TRUE);
  } else {
    return (FALSE);
  }

}

