# include "gophot.h"

impaper2 (int k) {

  int i, nsky, nsort;
  float sky, sum, r, dr, zsort[MAXFIL];

  /* find median sky outside inner radius */

  dr = 0.5*(SQ (0.5*arect[1]) + SQ (0.5*arect[2]));
  nsort = 0;
  for (i = 0; i < npt; i++) {
    r = SQ (xs[i]) + SQ (ys[i]);
    if (r >= dr) {
      zsort[nsort] = zs[i];
      nsort ++;
    }
  }
  
  if (nsort < 1) return (0);

  fsort (zsort, nsort);
        
  sky = 0;
  nsky = 0;
  for (i = 0.25*nsort; i < 0.75*nsort; i++) {
    sky += zsort[i];
    nsky ++;
  }
  sky /= nsky;

  sum = 0;
  for (i = 0; i < npt; i++) {
    sum += (zs[i] - sky);
  }
        
  apple[k][1] = sum*ufactor;

  return (1);

}

