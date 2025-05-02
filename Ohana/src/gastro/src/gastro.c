# include "gastro.h"
# define ABORT \
{ Nmatch = 1; dR = 0; \
  gheader (argv[1], coords, dR, Nmatch); \
  exit (0); }

int main (int argc, char **argv) {

  int N1, N2, Ncat, NX, NY, Nmatch, Nminterms;
  SStars *catalog, *stars1, *stars2;
  struct timeval now, then;  
  Coords coords;
  double dNdM, dR, Radius;
  
  gettimeofday (&then, (void *) NULL);

  ConfigInit (&argc, argv);
  gargs (&argc, argv, &coords); 

  /* load stars from image (*.cmp file) */
  N1 = NMAX_STARS;  /* we only want a small number of stars */
  stars1 = gstars (argv[1], &N1, &coords, &NX, &NY, &dNdM);

  /* load stars from reference catalogs */
  greference (&catalog, &Ncat, &coords, NX, NY);

  /* get rough alignment with reference stars */
  gproject (catalog, &stars2, Ncat, &N2, &coords, NX, NY, dNdM, N1);
  gcenter (stars1, stars2, N1, N2, &coords, NX, NY, &Radius);
  free (stars2);

  /* reload reference, get good astrometry */
  if (!greference (&catalog, &Ncat, &coords, NX, NY)) ABORT;
  /* NFIELD = 1.2; */
  gproject (catalog, &stars2, Ncat, &N2, &coords, NX, NY, dNdM, N1);
  if (!gfit (stars1, stars2, N1, N2, &coords, NX, NY, &Radius, &dR, &Nmatch, 1)) ABORT;
  free (stars2);
 
  gproject (catalog, &stars2, Ncat, &N2, &coords, NX, NY, dNdM, N1);
  gfitpoly (stars1, stars2, N1, N2, &coords, &Radius, &dR, &Nmatch);

  if (dR > MAX_ERROR) {
    fprintf (stderr, "ERROR: bad solution! %f %f (%d stars)\n", dR, (dR / sqrt(1.0*Nmatch)), Nmatch);
    ABORT;
  }
  fprintf (stderr, "good solution: %f %f (%d stars)\n", dR, (dR / sqrt(1.0*Nmatch)), Nmatch);

  Nminterms = MIN_MATCHES;
  switch (NPOLYTERMS) {
  case 0:
  case 1:
    /* we don't really allow zero order fits */
    Nminterms = MIN_MATCHES;
    break;
  case 2:
    Nminterms = 20;
    break;
  case 3:
    Nminterms = 40;
    break;
  }
    
  if (Nmatch <= Nminterms) { 
    fprintf (stderr, "ERROR: too few stars for reliable solution, only %d\n", Nmatch);
    ABORT;
  }

  gheader (argv[1], coords, dR, Nmatch);

  if (VERBOSE) {
    gettimeofday (&now, (void *) NULL);
    fprintf (stderr, "%s: elapsed time = %.2f sec\n", argv[1], 
	     (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));
  }
  fprintf (stderr, "SUCCESS\n");
  exit (0);
}

/*
  free (stars2);

  DEFAULT_RADIUS = DEFAULT_RADIUS / 4;
  gproject (catalog, &stars2, Ncat, &N2, &coords, NX, NY, dNdM, N1);
  if (!gfit (stars1, stars2, N1, N2, &coords, NX, NY, &Radius, &dR, &Nmatch, 0)) ABORT;
*/
