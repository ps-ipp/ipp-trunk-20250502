# include "markstar.h"

int wcatalog (char *catname, catalog *catalog) {
  
  int status, mode;
  off_t i, Nitems, nitems;
  char filename[128], line[256];
  FILE *f;
  struct stat filestat;

  sprintf (filename, "%s/%s\0", CATDIR, catname);

  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, make backup copy */
    sprintf (line, "mv %s %s~\0", filename, filename);
    status = system (line);
    if (status) {
      fprintf (stderr, "ERROR: unable to create %s~, exiting\n", filename);
      exit (0);
    }
  }

  if (catalog[0].Naverage == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (FALSE);
  }
  
  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't create new catalog file: %s\n", filename);
    exit (0);
  }
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (filename, mode);
  
  gfits_modify (&catalog[0].header, "NSTARS", OFF_T_FMT, 1, catalog[0].Naverage);
  gfits_modify (&catalog[0].header, "NMEAS",  OFF_T_FMT, 1, catalog[0].Nmeasure);
  gfits_modify (&catalog[0].header, "NMISS",  OFF_T_FMT, 1, catalog[0].Nmissing);

  gfits_modify_alt (&catalog[0].header, "MARKSTAR", "%t", 1, TRUE);

  nitems = Fwrite (catalog[0].header.buffer, 1, catalog[0].header.datasize, f, "char");
  if (nitems != catalog[0].header.size) {
    fprintf (stderr, "ERROR: failed to write header\n");
    exit (0);
  }

  Nitems = catalog[0].Naverage;
  nitems = Fwrite (catalog[0].average, sizeof(Average), Nitems, f, "average");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to write catalog file aves %s\n", filename);
    exit (0);
  }
  
  Nitems = catalog[0].Nmeasure;
  nitems = Fwrite (catalog[0].measure, sizeof(Measure), Nitems, f, "measure");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to write catalog file meas %s\n", filename);
    exit (0);
  }

  Nitems = catalog[0].Nmissing;
  nitems = Fwrite (catalog[0].missing, sizeof(Missing), Nitems, f, "missing");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to write catalog file miss %s\n", filename);
    exit (0);
  }

  fclose (f);

  return TRUE;
}
