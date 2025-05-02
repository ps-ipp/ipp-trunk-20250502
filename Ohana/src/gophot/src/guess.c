# include "gophot.h"

/* fill in values for star[NPMAX] based on rough stats and sky model */
float guess1 (float *star, float *dummy, int ix, int iy) {

  /* parinterp fills in star[4,5,6] from averages */
  parinterp (ix, iy, star);

  star[0] = sum2/ufactor;
  star[1] = maxval/ufactor;
  star[2] = xmax;
  star[3] = ymax;

  return (0.0);

}

/* fill in values for star[NPMAX] based on rough stats and sky model */
/* use sigma_x, sigma_y from fillerup */
float newguess (float *star, float *dummy, int ix, int iy) {

  star[0] = sum2 / ufactor;
  star[1] = maxval / ufactor;
  star[2] = xmax;
  star[3] = ymax;
  star[4] = xmax2;
  star[5] = 0;
  star[6] = ymax2;
  
  return (0.0);
}

/* fill in values for star from instar, rescaling */
float guess2 (float *star, float *instar, int *ix, int *iy) {

  /* i'm concerned that the ix, iy values need to be passed back. */
  float value;

  *ix = (int) (instar[2] + 0.5);
  *iy = (int) (instar[3] + 0.5);

  star[0] = instar[0]/ufactor;
  star[1] = instar[1]/ufactor;
  star[2] = instar[2] - *ix;
  star[3] = instar[3] - *iy;
  star[4] = instar[4];
  star[5] = instar[5];
  star[6] = instar[6];

  value = instar[0];				
  return (value);
}


/* fill in values for star from instar, rescaling */
float guess3 (float *star, float *instar, int *ix, int *iy) {

  float value;

  *ix = (int) (instar[2] + 0.5);
  *iy = (int) (instar[3] + 0.5);

  parinterp (instar[2], instar[3], star);

  star[0] = instar[0]/ufactor;
  star[1] = instar[1]/ufactor;
  star[2] = instar[2] - *ix;
  star[3] = instar[3] - *iy;

  value = instar[0];				
  return (value);
}
