# include "lightcurve.h"

void get_Mcal (images, unique, Nimages)
Image  *images;
Unique *unique;
int     Nimages;
{
  
  int    i, j;
  Star  *this_star;
  double m, dm, Mrel, r, R, M, M2, N;
  
  for (i = 0; i < Nimages; i++) {
    N = M = M2 = R = 0;
    this_star = images[i].first_this_image;
    for (j = 0; j < images[i].Nstars; j++) {
      if (this_star[0].unique_number != EMPTY) {
	dm   = this_star[0].dm;
	m    = this_star[0].m;
	Mrel = unique[this_star[0].unique_number].Mrel;
	r    = 1.0 / (dm*dm);
	R   += r;
	M   +=    (m - Mrel) * r;
	M2  +=  SQ(m - Mrel) * r;
	N   += 1.0;
      }
      this_star = this_star[0].next_this_image;
    }
    if (R != 0) {
      images[i].Mcal  = M / R;
      images[i].dMcal = sqrt(fabs(M2 / R - SQ(M/R))) / sqrt (N);
    }
    else {
      /* no multiple detections on this image! */
      images[i].empty = TRUE;
      images[i].Mcal  = 9.999;
      images[i].dMcal = 9.999;
    }
  }
}

/* m (instrumental) = Mrel (star) + Mcal (image) */

