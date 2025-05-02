# include "relphot.h"

void get_Mrel (unique, images, Nunique)
Unique   *unique;
Image    *images;
int       Nunique;
{
  
  int    i, j;
  Star  *this_star;
  double dm, m, r, R, M, M2, Mcal, N;

  for (i = 0; i < Nunique; i++) {
    N = M = M2 = R = 0;
    this_star = unique[i].first_this_unique;
    for (j = 0; j < unique[i].Nmeasurements; j ++) {
      dm   = this_star[0].dm;
      m    = this_star[0].m;
      Mcal = images[this_star[0].image_number].Mcal;
      r    = 1.0 / (dm*dm);
      R   += r;
      M   +=    (m - Mcal) * r;
      M2  +=  SQ(m - Mcal) * r;
      N   += 1.0;
      this_star = this_star[0].next_this_unique;
    }
    unique[i].Mrel  = M / R;
    unique[i].dMrel = sqrt(fabs(M2 / R - SQ(M/R))) / sqrt (N);
  }

  for (i = 0; i < Nunique; i++) {
    N = X2 = 0;
    Mrel = unique[i].Mrel;
    this_star = unique[i].first_this_unique;
    for (j = 0; j < unique[i].Nmeasurements; j ++) {
      dm   = this_star[0].dm;
      m    = this_star[0].m;
      Mcal = images[this_star[0].image_number].Mcal;
      X2  += SQ((m - Mcal - Mrel) / dm);
      this_star = this_star[0].next_this_unique;
    }
    unique[i].ChiSq = X2 / (N - 1);
  }
}

/* m (instrumental) = Mrel (star) + Mcal (image) */
