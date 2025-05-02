# include "gophot.h"

bool cosmic (float *star) {

  float dummy[NPMAX];
  float obs, maxob, sky, chistar, chicos, pred, temp;
  float sn2, tnoise, *Bval, *Nval;
  int ix, iy, I, J, ii, jj, npix, imax, jmax;
  bool pointy;

  pointy = FALSE;

  sky = guess2 (dummy, star, &ix, &iy);
  temp = big[ix + iy*nfast] / ufactor - dummy[0];
  if (finite (temp)) dummy[1] = temp;
  maxob = -HUGE_VAL;
  chistar = 0;
  chicos  = 0;
  npix = 0;
  
  for (J = -1; J <= 1; J++) {
    jj = iy + J;
    if (jj < 0) continue;
    if (jj > nslow - 1) continue;
    
    Bval = &big[ix - 1 + jj*nfast];
    Nval = &noise[ix - 1 + jj*nfast];
    
    for (I = -1; I <= 1; I++, Bval++, Nval++) {
      ii = ix + I;
      if (ii < 0) continue;
      if (ii > nfast - 1) continue;
      if (!finite (*Nval)) continue;
      npix ++;
      pred = ufactor*onestar (I, J, dummy, (float *) NULL);
      obs = *Bval;
      temp = 1.0 / (*Nval + *Bval);
      chistar += SQ (obs - pred) * temp;
      sn2 = SQ (obs - sky) * temp;
      chicos += sn2;
      if ((obs > maxob) && (sn2 >= sn2cos)) {
	imax = ii;
	jmax = jj;
	maxob = obs;
	tnoise = temp;
      }
    }
  }

  /* this is meant to test if the object is mostly a single pixel event.
     doesn't do a good job of distinguishing a single pixel event from a
     generally poor fit to the top of the star. */
  if ((npix >= 7) && (maxob > -HUGE_VAL)) {
    chicos -= SQ (maxob - sky) * tnoise;
    pointy = (chicos/chistar < discrim);
    mprint (3, "location %d, %d,  chi-star & chi-cosmic = %f, %f\n", ix, iy, chistar, chicos);
  }

  if (pointy) {
    mprint (2, "cosmic ray intensity, x, y: %f %d %d\n", maxob, imax, jmax);
    star[0] = sky;
    star[1] = maxob;
    star[2] = imax;
    star[3] = jmax;
    star[4] = widobl;
    star[5] = -1;
    star[6] = widobl;
  }
  return (pointy);
}

/* We are basically comparing if the object is more like a stellar
   object, or if it is compatible with a single high point over a
   general sky-like region.  

   we have a danger here that a wide object will boost Io
   significantly, which in turn makes the fit to the object
   particularly poor in the center areas.  This in turn makes the
   object seem comparable to the sky, and therefore a cosmic.  Since
   we really only care if the inner 9 pixels are comparable to the
   star shape, vaguely, we can fudge this by temporarily setting Io to
   the central pixel value.  We don't bother if that pixel is bad (NaN).

*/
