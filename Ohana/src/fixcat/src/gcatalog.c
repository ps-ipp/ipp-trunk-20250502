# include "markstar.h"

gcatalog (catname, catalog)
char *catname;
Catalog catalog[];
{
  
  char filename[128], line[64];
  int Nitems, nitems;
  int i, Nmeas, Nmiss, done;
  FILE *f;
  struct tm *local;
  struct timeval now;
  unsigned int NotTrail;
  unsigned short NotBad;

  sprintf (filename, "%s/%s\0", CATDIR, catname);

  /* read catalog header */
  if (!gfits_read_header (filename, &catalog[0].header)) {
    fprintf (stderr, "ERROR: file doesn't exist %s\n", filename);
    exit (0);
  }

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't open catalog file: %s\n", filename);
    exit (0);
  }
  fseeko (f, catalog[0].header.datasize, SEEK_SET); 

  /** find number of stars, measurements **/
  catalog[0].Nmissing = catalog[0].Naverage = catalog[0].Nmeasure = 0;
  gfits_scan (&catalog[0].header, "NSTARS", "%d", 1, &catalog[0].Naverage);
  gfits_scan (&catalog[0].header, "NMEAS", "%d", 1, &catalog[0].Nmeasure);
  gfits_scan (&catalog[0].header, "NMISS", "%d", 1, &catalog[0].Nmissing);

  ALLOCATE (catalog[0].average, Average, MAX (catalog[0].Naverage, 1));
  ALLOCATE (catalog[0].measure, Measure, MAX (catalog[0].Nmeasure, 1));
  ALLOCATE (catalog[0].missing, Missing, MAX (catalog[0].Nmissing, 1));
  if (catalog[0].Naverage == 0) {
    /* this is a valid ending state: no stars */
    if (VERBOSE) fprintf (stderr, "SUCCESS: no stars yet in catalog %s\n", filename);
    fclose (f);
    clear_lockfile ();
    exit (0);
  }

  /* read average values */
  Nitems = catalog[0].Naverage;
  nitems = Fread (catalog[0].average, sizeof(Average), Nitems, f, "average");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to read data from catalog file %s (1)\n", filename);
    fclose (f);
    exit (0);
  }
  
  /* read measurements */
  Nitems = catalog[0].Nmeasure;
  nitems = Fread (catalog[0].measure, sizeof(Measure), Nitems, f, "measure");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to read data from catalog file %s (2)\n", filename);
    fclose (f);
    exit (0);
  }
  
  /* read missing */
  Nitems = catalog[0].Nmissing;
  nitems = Fread (catalog[0].missing, sizeof(Missing), Nitems, f, "missing");
  if (nitems != Nitems) {
    fprintf (stderr, "ERROR: failed to read data from catalog file %s (3)\n", filename);
    fclose (f);
    exit (0);
  }
  
  if (VERBOSE) fprintf (stderr, "read %d stars from catalog file %s (%d measurements, %d missing)\n", 
	   catalog[0].Naverage, filename, catalog[0].Nmeasure, catalog[0].Nmissing);

  for (i = Nmeas = Nmiss = 0; i < catalog[0].Naverage; i++) {
    Nmeas += catalog[0].average[i].Nm; 
    Nmiss += catalog[0].average[i].Nn; 
  }
  if ((Nmeas != catalog[0].Nmeasure) || (Nmiss != catalog[0].Nmissing)) {
    fprintf (stderr, "ERROR: data in catalog %s is corrupt, sums don't check\n");
    fprintf (stderr, "ERROR: Nmeas: %d, %d\n", Nmeas, catalog[0].Nmeasure);
    fprintf (stderr, "ERROR: Nmiss: %d, %d\n", Nmiss, catalog[0].Nmissing);
    exit (0);
  }

  if (RESET) {
    NotBad = (0xffff ^ 0xc003);
    for (i = 0; i < catalog[0].Naverage; i++) {
      if ((catalog[0].average[i].code & ID_MOVING) &&
	  (catalog[0].average[i].code & ID_BAD_OBJECT) &&
	  (catalog[0].average[i].code & 0x0003)) {
	/* this will set the correct bit for each to 0 */ 
	catalog[0].average[i].code &= NotBad;
      }
    }
    for (i = 0; i < catalog[0].Nmeasure; i++) {
      catalog[0].measure[i].average &= ~PART_OF_TRAIL;
    }
  }


  fclose (f);

}
