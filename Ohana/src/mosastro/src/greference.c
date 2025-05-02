# include "mosastro.h"

StarData *greference (int *Nrefcat) {

  int Nstars;
  StarData *stars;
  CatStats catstats;

  if (VERBOSE) fprintf (stderr, "loading astrometric reference data from %s\n", REFCAT); 

  stars = NULL;
  Nstars = 0;
  catstats.RA[0]  = field.Rmin;
  catstats.RA[1]  = field.Rmax;
  catstats.DEC[0] = field.Dmin;
  catstats.DEC[1] = field.Dmax;

  if (VERBOSE) fprintf (stderr, "full region: %f - %f, %f - %f\n", catstats.RA[0], catstats.RA[1], catstats.DEC[0], catstats.DEC[1]);

  /* get stars from the Stone et al catalog for the given region */
  if (!strcmp (REFCAT, "STONE")) {
    stars = getstone (&catstats, &Nstars);
  }

  /* get stars from the USNO A catalog for the given region */
  if (!strcmp (REFCAT, "USNO")) {
    stars = getusno (&catstats, &Nstars);
  }

  /* get stars from the USNO A catalog for the given region */
  if (!strcmp (REFCAT, "USNOB")) {
    stars = getusnob (&catstats, &Nstars);
  }

  /* get stars from the HST GSC catalog for the given region */
  if (!strcmp (REFCAT, "GSC")) {
    stars = getgsc (&catstats, &Nstars);
  }
  
  /* get stars from 2MASS for the given region -- add PHOTCODE check? */
  if (!strcmp (REFCAT, "2MASS")) {
    strcpy (CATDIR, TWO_MASS_DIR);
    stars = getptolemy (&catstats, &Nstars);
  }
  
  /* get stars from the DVO CATDIR for the given region */
  if (!strcmp (REFCAT, "PTOLEMY")) {
    strcpy (CATDIR, ASTROM_CATDIR);
    stars = getptolemy (&catstats, &Nstars);
  }
  
  if (Nstars == 0) {
    fprintf (stderr, "no ref objs: %s\n", REFCAT);
    exit (1);
  }
  *Nrefcat = Nstars;
  return (stars);

}
