# include "gophot.h"

float set_thresholds (float sky, float dSky, float *factor, int *Nit) {

  float dsky, tmp;

  /* tmin starts with Nsigma above sky */

  dsky = sqrt(sky + rnoise);
  fprintf (stderr, "dsky: %f %f %f %f\n", rnoise, sky, dsky, dSky);
  dsky = MAX (dsky, dSky);
  tmin = tmin * dsky;

  mprint (0, "gain is %f, median sky is %f, min threshold is %f\n",
	  eperdn, sky, tmin);

  /* set starting threshold */
  tmp = pow (2.0, tfac);
  tmax = 0.5*itop;
  *Nit  = log (tmax / tmin) / log (tmp);
  *factor = pow ((tmax / tmin), (1.0 / *Nit));

  return (tmax);

}

/* we are going to evenly divide the range from 
   sky + tmin*dsky to 0.5*itop in slices with ratios 
   close to tfac */

/* images with very messy backgrounds should not be pushed to the 
   absolute minimum.  We let the sky sigma be the max of the 
   formal and the measured sigmas */
