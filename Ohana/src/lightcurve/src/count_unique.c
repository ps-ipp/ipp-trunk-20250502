# include "lightcurve.h"

void count_unique (images, Nimages, Nstars)
Image *images;
int    Nimages; 
int    Nstars;
{

  int i, j;
  Star *this_star;

  for (i = 0; i < Nimages; i++) {
    images[i].Nunique = 0;
    this_star = images[i].first_this_image;
    for (j = 0; j < images[i].Nstars; j++) {
      if (this_star[0].unique_number != EMPTY)
	images[i].Nunique ++;
      this_star = this_star[0].next_this_image;
    }
  }
}

    
