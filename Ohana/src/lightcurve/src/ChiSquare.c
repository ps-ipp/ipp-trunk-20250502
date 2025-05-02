# include "lightcurve.h"

void ChiSquare (unique, images, Nunique, Nimages)
Unique    *unique;
Image     *images;
int        Nunique;
int        Nimages;
{

  int    i, j;
  Star   *this_star;
  double m, dm, Mcal, Mrel, ChiSquare, Ndof;

  Ndof = ChiSquare = 0.0;
  for (i = 0; i < Nunique; i ++) {
    this_star = unique[i].first_this_unique;
    for (j = 0; j < unique[i].Nmeasurements; j ++) {
      dm   = this_star[0].dm;
      m    = this_star[0].m;
      Mcal = images[this_star[0].image_number].Mcal;
      Mrel = unique[i].Mrel;
      Ndof += 1.0;
      ChiSquare += SQ (m - Mcal - Mrel) / SQ (dm);
      this_star = this_star[0].next_this_unique;
    }
  }
  Ndof = Ndof - Nunique - Nimages;
  fprintf (stderr, "Chi Square %9.2f  Ndof %6.0f  Reduced %6.3f  A_LAMBDA  %6.4f\n", 
	   ChiSquare, Ndof, ChiSquare/Ndof, A_LAMBDA);
}
