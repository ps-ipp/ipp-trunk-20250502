# include "dvoshell.h"

int cmatch (int argc, char **argv) {
  
  Catalog catalog1, catalog2;
  char filename[128];
  double radius;
  Vector *rvec, *dvec, *mvec, *drvec, *ddvec, *dmvec;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: cmatch file radius (RA) (DEC) (Mag) (dRA) (dDEC) (dMag)\n");
    gprint (GP_ERR, "       match a set of object coordinates with a DVO db table\n");
    return (FALSE);
  }

  /*** this function is not well-defined.  re-assess it and re-code it ***/
  gprint (GP_ERR, "disabled for now\n");
  return (FALSE);

  radius = atof (argv[2]);

  if ((rvec  = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dvec  = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((mvec  = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((drvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ddvec = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dmvec = SelectVector (argv[8], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  /* load data from the photometry database file */
  dvo_catalog_init (&catalog1, TRUE);
  catalog1.filename = filename;
  catalog1.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog1, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog1.filename);
      exit (2);
  }
  dvo_catalog_unlock (&catalog1);
  gprint (GP_ERR, "read "OFF_T_FMT" stars from phot catalog file %s\n",  catalog1.Naverage, filename);

  /* this is for loading from a text file, presumably hstgsc or usno
     replace this with references to the ra and dec vectors?
  nstar = 0;
  NSTARS = DNSTARS;
  ALLOCATE (tbuffer, char, (BLOCK*BYTES_STAR));
  ALLOCATE (catalog2.average, Average, NSTARS);
  Nbytes = BLOCK*BYTES_STAR;
  while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
    for (i = 0; i < nbytes / BYTES_STAR; i++) {
      dparse (&R, 1, &tbuffer[i*BYTES_STAR]);
      dparse (&D, 2, &tbuffer[i*BYTES_STAR]);
      dparse (&M, 3, &tbuffer[i*BYTES_STAR]);
      catalog2.average[nstar].R = R;
      catalog2.average[nstar].D = D;
      catalog2.average[nstar].M = M * 1000.0;
      nstar++;
      if (nstar == NSTARS - 1) {
	NSTARS += DNSTARS;
	REALLOCATE (catalog2.average, Average, NSTARS);
      }
    }
  }
  free (tbuffer);
  REALLOCATE (catalog2.average, Average, MAX (nstar, 1));
  catalog2.Naverage = nstar;
  fclose (f);
  */

  /* sort data in order of RA */
  sortave (catalog1.average, catalog1.Naverage);
  sortave (catalog2.average, catalog2.Naverage);

  /* data has been loaded, use gcompare algorithm to match */
  compare (&catalog1, &catalog2, rvec, dvec, mvec, drvec, ddvec, dmvec, radius);

  dvo_catalog_free (&catalog1);
  free (catalog2.average);

  return (TRUE);

}
  
