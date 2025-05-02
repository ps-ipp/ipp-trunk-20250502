# include "gophot.h"

makemask () {

  int i, j, ii, jj;
  float value;
  float dumx, dumy, dummy, star[NPMAX];

  dumx = 0;
  dumy = 0;
  dummy = parinterp (dumx, dumy, star);
  
  star[0] = 0;			
  star[1] = 1;			
  star[2] = 0;			
  star[3] = 0;	
  
  for (j = 0, jj = -iyby2; j < 2*iyby2 + 1; j++, jj++) {
    for (i = 0, ii = -ixby2; i < 2*ixby2 + 1; i++, ii++) {
      starmask[i][j] = value = onestar (ii, jj, star, (float *) NULL);
      mprint (2, "starmask[%d][%d]: %f\n", ii, jj, value);
    }
  }
}

/* this function uses C 0,N-1 for a[], fa[] */
