# include "gastro.h"
# define MMIN 2
# define dM 0.5
# define NMBIN 64

void gproject (SStars *catalog, SStars **stars, int Ncat, int *Nstars, Coords *coords, int NX, int NY, double dNdM, int N1) {
  
  double mbin[NMBIN], ratio;
  int i, j, NSTARS, nstar, nstar2;
  double X, Y, m0, Mmin, Mmax;
  int XMIN, XMAX, YMIN, YMAX;
  SStars *tstars, *t2stars;

  NSTARS = Ncat;
  ALLOCATE (tstars, SStars, NSTARS);

  XMIN = 0.5*(1.0 - NFIELD)*NX;
  XMAX = 0.5*(1.0 + NFIELD)*NX;
  YMIN = 0.5*(1.0 - NFIELD)*NY;
  YMAX = 0.5*(1.0 + NFIELD)*NY;
  
  /* project to local coords, select stars within region */
  for (nstar = i = 0; i < Ncat; i++) {
    RD_to_XY (&X, &Y, catalog[i].X, catalog[i].Y, coords);
    if ((X > XMIN) && (X < XMAX) && (Y > YMIN) && (Y < YMAX)) {
      tstars[nstar].X = X;
      tstars[nstar].Y = Y;
      tstars[nstar].mag = catalog[i].mag;
      nstar++;
      if (CATDUMP) {
	fprintf (stdout, "%f %f %f %f %f\n", catalog[i].X, catalog[i].Y, X, Y, catalog[i].mag);
      }
    }
  }
  if (CATDUMP) {
    exit (0);
  }

  REALLOCATE (tstars, SStars, nstar);
  if (VERBOSE) fprintf (stderr, "%d total reference stars\n", nstar);
  
  /* find appropriate magnitude range */ 
  ratio = NFIELD * NFIELD;
  m0 = 0;
  bzero (mbin, NMBIN * sizeof (double));
  for (i = 0; i < nstar; i++) {
    if (tstars[i].mag < MMIN) {
      fprintf (stderr, "%d %f %f %f\n", i, tstars[i].X, tstars[i].Y, tstars[i].mag);
    }
    j = (tstars[i].mag - MMIN) / dM;
    j = MIN (MAX (j, 0), (NMBIN - 1));
    mbin[j] ++;
  }
  for (i = 0; i < NMBIN; i++) {
    if (ratio * dNdM < mbin[i] / dM) {
      m0 = (i - 1) * dM + MMIN;
      break;
    }
  }
  
  Mmin = m0 - 1.0;
  Mmax = m0 + MAX (N1/dNdM, 1) + 1.0;
  if (m0 == 0) {
    Mmin = 0;
    Mmax = 32;
  }
  if (VERBOSE) fprintf (stderr, "choosing magnitude range for reference stars: ");
  if (VERBOSE) fprintf (stderr, " %5.2f to %5.2f mags\n", Mmin, Mmax);


  ALLOCATE (t2stars, SStars, nstar);
  if (MAGLIMS) {
    /* make cut on mags */
    for (nstar2 = i = 0; i < nstar; i++) {
      if ((tstars[i].mag > Mmin) && (tstars[i].mag < Mmax)) { 
	t2stars[nstar2] = tstars[i];
	nstar2 ++;
      }
    }
  } else {
    bcopy (tstars, t2stars, nstar*sizeof(SStars));
    nstar2 = nstar;
  }
    

  free (tstars);
  *stars = t2stars;
  *Nstars = nstar2;
  if (VERBOSE) fprintf (stderr, "%d reference stars in mag range\n\n", nstar2);

}


/* in this routine, 
   catalog[i].X,Y are RA,DEC in dec degrees
   stars[i].X.Y   are projected coords in approx pixels on image
   */
