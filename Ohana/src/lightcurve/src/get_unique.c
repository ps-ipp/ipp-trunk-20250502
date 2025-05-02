# include "lightcurve.h"
# define D_NUNIQUE 1000;

void get_unique (unique, N, stars, radec, Nstars, sources, Nsources)
Unique **unique;
int    *N;
Star   *stars;
int    *radec;
int     Nstars;
Star   *sources;
int     Nsources;
{

  int i, j, link, k, n, confused, Nchain, NUNIQUE, Nunique, first, NTEMP;
  int Ntemp, Nclump, Nblobs;
  Unique  temp_unique;
  int    *templist;
  Star   *next_unique, *this_unique;
  double radius, dRA, dDec, RAo, DECo, dRA2, dDec2;
  double dRo, dDo, dRo2, dDo2;

  NUNIQUE = D_NUNIQUE;
  ALLOCATE (unique[0], Unique, NUNIQUE);
  NTEMP = 100;
  ALLOCATE (templist, int, NTEMP);  
  Nblobs = dRo = dDo = dRo2 = dDo2 = 0;

  for (i = Nunique = 0; (i < Nstars - 1); i++) {
    
    /* check if this star is not already assigned to a clump */
    if (stars[radec[i]].next_this_unique == NULL) {
      
      /* first, find all stars within 2*RADIUS of this star */
      first = i;
      Ntemp = 1;
      dRA = 0;
      templist[0] = radec[first];  
      for (j = first + 1; (dRA <= 2.0*RADIUS) && (j < Nstars); j++) {
	if (stars[radec[j]].next_this_unique != NULL)
	  continue;
	dRA  = COS * (stars[radec[j]].RA - stars[radec[first]].RA);  
	dDec = stars[radec[j]].Dec - stars[radec[first]].Dec;
	radius = hypot(dRA, dDec);
	if (radius < 2.0*RADIUS) {
	  /* add to templist: */
	  templist[Ntemp] = radec[j];
	  Ntemp ++;
	  if (Ntemp == NTEMP - 1) {
	    NTEMP += 100;
	    REALLOCATE (templist, int, NTEMP); 
	  }
	}
      }
	     
      if (Ntemp < 3) {
	continue; /* check this pops out of the right level... */
      }

      /* next, find the centroid of this subset of stars */
      dRA = dDec = dRA2 = dDec2 = 0;
      for (j = 0; j < Ntemp; j++) {
	dRA   += stars[templist[j]].RA;
	dDec  += stars[templist[j]].Dec;
	dRA2  += SQ(stars[templist[j]].RA);
	dDec2 += SQ(stars[templist[j]].Dec);
      }
      RAo   = dRA / (double)Ntemp;
      DECo  = dDec / (double)Ntemp;
      dRA2  = sqrt(dRA2 / (double)Ntemp - SQ(RAo));
      dDec2 = sqrt(dDec2 / (double)Ntemp - SQ(DECo));

      /* then, find all stars within 1 RADIUS of this centroid */
      Nclump = 0;
      unique[0][Nunique].first_this_unique = (Star *) NULL;
      for (j = 0; j < Ntemp; j++) {
	dRA  = COS * (stars[templist[j]].RA - RAo);
	dDec = stars[templist[j]].Dec - DECo;
	radius = hypot(dRA, dDec);
	if (radius < RADIUS) {
	  if (Nclump != 0) {
	    this_unique[0].next_this_unique = &stars[templist[j]];
	    this_unique = &stars[templist[j]];
	    this_unique[0].unique_number = Nunique;
	  }
	  else {
	    unique[0][Nunique].first_this_unique = &stars[templist[j]];
	    this_unique = &stars[templist[j]];
	    this_unique[0].unique_number = Nunique;
	  }
	  Nclump ++;
	}
      }
      
      /* if too few in clump, eliminate, move on */
      if (Nclump < 5) {
	this_unique = unique[0][Nunique].first_this_unique;
	unique[0][Nunique].first_this_unique = NULL;
	while (this_unique != NULL) {
	  next_unique = this_unique[0].next_this_unique;
	  this_unique[0].unique_number = EMPTY;
	  this_unique[0].next_this_unique = NULL;
	  this_unique = next_unique;
	}
	continue;
      }

      /* count number of confused stars...
      this_unique = unique[0][Nunique].first_this_unique;
      for (j = 0, confused = FALSE; j < Nclump - 1; j++) {
	next_unique = this_unique[0].next_this_unique;
	for (k = j + 1; k < Nclump; k++) {
	  if (this_unique[0].image_number == next_unique[0].image_number) {
	    confused = TRUE;
	  }
	  next_unique = next_unique[0].next_this_unique;
	}
	this_unique = this_unique[0].next_this_unique;
      } 
       */

      /* if this object is confused, ignore it and move on.  
	 keep the "next_this_unique" values on, but remove
	 the "unique_number" */

      /*
      
      if (confused) {
	this_unique = unique[0][Nunique].first_this_unique;
	unique[0][Nunique].first_this_unique = NULL;
	while (this_unique != NULL) {
	  this_unique[0].unique_number = EMPTY;
	  this_unique = this_unique[0].next_this_unique;
	}
	Nblobs ++;
	continue;
      }
      */
      
      this_unique = unique[0][Nunique].first_this_unique;
      for (j = 0; j < Nclump; j++) {
	dRA   += this_unique[0].RA;
	dDec  += this_unique[0].Dec;
	dRA2  += SQ(this_unique[0].RA);
	dDec2 += SQ(this_unique[0].Dec);
	next_unique = this_unique[0].next_this_unique;
      }
      RAo   = dRA / (double)Nclump;
      DECo  = dDec / (double)Nclump;
      for (n = 0; n < Nsources; n++) {
	dRA = COS * (RAo - sources[n].RA);
	dDec = (DECo - sources[n].Dec);
	radius = hypot(dRA, dDec);
	if (radius < RADIUS) {
	  sources[n].unique_number = Nunique;
	  fprintf (stderr, "%d %d  %f %f  %f %f\n", 
		   n, Nunique, RAo, DECo, sources[n].RA, sources[n].Dec);
	  break;
	}
      } 
      
      unique[0][Nunique].Nmeasurements = Nclump;
      unique[0][Nunique].Mrel = 0.0;
      /* use only stars which show up on all images */
      
      dRo += dRA2;
      dDo += dDec2;
      dRo2 += dRA2*dRA2;
      dDo2 += dDec2*dDec2;
      Nunique ++;
      if (Nunique == NUNIQUE - 1) { 
	NUNIQUE += D_NUNIQUE;
	REALLOCATE(unique[0], Unique, NUNIQUE);
      }
    }
  } 

  *N = Nunique;
  dRo = dRo / (double)Nunique;
  dDo = dDo / (double)Nunique;
  dRo2 = sqrt(dRo2 / (double)Nunique - SQ(dRo));
  dDo2 = sqrt(dDo2 / (double)Nunique - SQ(dDo));

  fprintf (stderr, "%f   %f %f %f %f   %4d %3d\n", 3600*RADIUS, dRo, dDo, dRo2, dDo2, Nunique, Nblobs);

}

