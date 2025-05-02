# include "relphot.h"
# define D_NUNIQUE 1000;

void match_stars (unique, N, stars, radec, Nstars, sources, Nsources)
Unique **unique;
int    *N;
Star   *stars;
int    *radec;
int     Nstars;
Source *sources;
int     Nsources;
{

  int i, j, link, k, good, Nchain, NUNIQUE, Nunique;
  Unique  temp_unique;
  Star   *next_unique, *this_unique;
  double radius, dRA, dDec;

  NUNIQUE = D_NUNIQUE;
  ALLOCATE (unique[0], Unique, NUNIQUE);

  for (i = Nunique = 0; (i < Nstars - 1); i++) {
    
    dRA = COS * (stars[radec[i+1]].RA - stars[radec[i]].RA);

    if (dRA < 0.0) {
      fprintf (stderr, "stars out of order!!!  %d  %d  %f  %f\n", 
	       radec[i+1], radec[i], stars[radec[i+1]].RA, stars[radec[i]].RA);
    }

    if ((fabs(dRA) < RADIUS) && (stars[radec[i]].next_this_unique == NULL)) {

      /* set up the chain */
      for (link = i, j = link + 1, Nchain = 1; (fabs(dRA) < RADIUS) && (j < Nstars); j++) {
	dDec   = stars[radec[j]].Dec - stars[radec[link]].Dec;
	radius = hypot(dRA, dDec);
	if (radius < RADIUS) {
	  stars[radec[link]].next_this_unique = &stars[radec[j]];
	  link = j;
	  Nchain ++;
	}
	if (j < Nstars - 1)
	  dRA  = COS * (stars[radec[j + 1]].RA - stars[radec[link]].RA);
      }

      /* are any 2 links on the same image? */
      this_unique = &stars[radec[i]];
      for (j = 0, good = TRUE; j < Nchain - 1; j++) {
	next_unique = this_unique[0].next_this_unique;
	for (k = j + 1; k < Nchain; k++) {
	  if (this_unique[0].image_number == next_unique[0].image_number) {
	    good = FALSE;
	    fprintf (stderr, "clump on image %d, stars %d and %d\n", 
		     this_unique[0].image_number, 
		     this_unique[0].star_number, 
		     next_unique[0].star_number);
	  }
	  next_unique = next_unique[0].next_this_unique;
	}
	this_unique = this_unique[0].next_this_unique;
      }

      /* if not, add chain to unique[] */
      if ((good) && (Nchain > 1)) {
	this_unique = unique[0][Nunique].first_this_unique = &stars[radec[i]];
	while (this_unique != NULL) {
	  this_unique[0].unique_number = Nunique;
	  this_unique = this_unique[0].next_this_unique;
	}
	unique[0][Nunique].Nmeasurements = Nchain;
	Nunique ++;
	if (Nunique == NUNIQUE - 1) { 
	  NUNIQUE += D_NUNIQUE;
	  REALLOCATE(unique[0], Unique, NUNIQUE);
	}
      }
    }
  } 

  *N = Nunique;
  fprintf (stderr, "Nunique: %d\n", Nunique);

}

