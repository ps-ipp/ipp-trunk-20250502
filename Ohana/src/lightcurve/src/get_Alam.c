# include "lightcurve.h"

void get_Alam (unique, images, Nunique, Nimages)
Unique    *unique;
Image     *images;
int        Nunique;
int        Nimages;
{

  int    i, j;
  Star   *this_star;
  double m, dm, Mcal, Mrel, Alm, R, zeta, clouds;
  double Clouds, dClouds, R2, N;

  R = R2 = N = 0.0;
  for (i = 0; i < Nimages; i++) {
    images[i].clouds = images[i].Mcal + C_LAMBDA - A_LAMBDA * images[i].airmass + images[i].AmF;
    if (!images[i].empty && images[i].fixed) {
      R  +=     images[i].clouds;
      R2 += SQ (images[i].clouds);
      N  += 1.0;
    }
  }

  Clouds  = R / N;
  dClouds = sqrt (R2 / N - Clouds*Clouds);
  dClouds = MAX (0.0001, dClouds);

  R = Alm = 0.0;
  for (i = 0; i < Nunique; i ++) {
    this_star = unique[i].first_this_unique;
    for (j = 0; j < unique[i].Nmeasurements; j ++) {
      dm   = this_star[0].dm;
      m    = this_star[0].m;
      Mrel = unique[i].Mrel;
      zeta = images[this_star[0].image_number].airmass;
      clouds = images[this_star[0].image_number].clouds;
      if (fabs(clouds - Clouds) > SIG*dClouds) {
	R   +=   zeta / SQ(dm);
	Alm += zeta * (m + C_LAMBDA - clouds - Mrel) / SQ (dm);
      }
      this_star = this_star[0].next_this_unique;
    }
  }
  if (R > 0) 
    A_LAMBDA = (9.0*A_LAMBDA + Alm / R) / 10.0;
}
