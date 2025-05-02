# include <dvo.h>
# define DEBUG 0

Catalog *dvo_catalog_create_subcat (Catalog *catalog, char *ext, char *tablename);

// create a new dvo catalog file (if split, lock extra files as well?)
// catalog internals which must be set:
// catalog[0].filename
// catalog[0].catmode
// catalog[0].catformat (used by dvo_catalog_save)
// catalog[0].lockmode
void dvo_catalog_create (SkyRegion *region, Catalog *catalog) {

  if (DEBUG) fprintf (stderr, "new catalog file: %s\n", catalog[0].filename);

  // init the data values, not the internals listed above
  dvo_catalog_init (catalog, FALSE);

  /* for RAW mode, make header a fake image */
  if (catalog[0].catmode == DVO_MODE_RAW) {
    catalog[0].header.bitpix   = 16;
    catalog[0].header.Naxes    = 2;
    catalog[0].header.Naxis[0] = 1;
    catalog[0].header.Naxis[1] = 1;
  }
  gfits_create_header (&catalog[0].header);

  if (catalog[0].catmode == DVO_MODE_SPLIT) {
    catalog[0].measure_catalog 	= dvo_catalog_create_subcat (catalog, "cpm", "MEASURE");
    catalog[0].missing_catalog 	= dvo_catalog_create_subcat (catalog, "cpn", "MISSING");
    catalog[0].secfilt_catalog 	= dvo_catalog_create_subcat (catalog, "cps", "SECFILT");
    catalog[0].lensing_catalog 	= dvo_catalog_create_subcat (catalog, "cpx", "LENSING");
    catalog[0].lensobj_catalog 	= dvo_catalog_create_subcat (catalog, "cpy", "LENSOBJ");
    catalog[0].starpar_catalog 	= dvo_catalog_create_subcat (catalog, "cpz", "STARPAR");
    catalog[0].galphot_catalog  = dvo_catalog_create_subcat (catalog, "cpq", "GALPHOT");

    // lock the additional split files
    // XXX clear residual locks if we fail
    if (dvo_catalog_lock (catalog[0].measure_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].missing_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].secfilt_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].lensing_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].lensobj_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].starpar_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
    if (dvo_catalog_lock (catalog[0].galphot_catalog, catalog[0].lockmode) != DVO_CAT_OPEN_EMPTY) {
      fprintf (stderr, "error with file lock\n");
      exit (2);
    }
  }    

  /* write RA,DEC range in header */
  if (region) {
    gfits_modify (&catalog[0].header, "RA0",  "%lf", 1, region[0].Rmin);
    gfits_modify (&catalog[0].header, "DEC0", "%lf", 1, region[0].Dmin);
    gfits_modify (&catalog[0].header, "RA1",  "%lf", 1, region[0].Rmax);
    gfits_modify (&catalog[0].header, "DEC1", "%lf", 1, region[0].Dmax);
    
    catalog[0].catID = region[0].index;
    gfits_modify (&catalog[0].header, "CATID", "%d", 1, catalog[0].catID);
  }

  /* write creation date in header */
  time_t now;
  ohana_str_to_time ("now", &now);
  char *line = ohana_sec_to_date (now);
  gfits_modify (&catalog[0].header, "DATE", "%s", 1, line);
  free (line);

  /* dummy allocation so realloc will succeed */
  ALLOCATE (catalog[0].average, Average, 1);
  ALLOCATE (catalog[0].measure, Measure, 1);
  ALLOCATE (catalog[0].missing, Missing, 1);
  ALLOCATE (catalog[0].secfilt, SecFilt, 1);
  ALLOCATE (catalog[0].lensing, Lensing, 1);
  ALLOCATE (catalog[0].lensobj, Lensobj, 1);
  ALLOCATE (catalog[0].starpar, StarPar, 1);
  ALLOCATE (catalog[0].galphot, GalPhot, 1);

  /* setup secondary filters to match photcodes:
   * Nsecfilt is number of filters.  Number of entries in array is
   * Nsecfilt * Naverage.  At this point, N entries == 0
   */
}
  
Catalog *dvo_catalog_create_subcat (Catalog *catalog, char *ext, char *tablename) {
  
  char *path = pathname (catalog[0].filename);
  char *root = filerootname (catalog[0].filename);

  int length = strlen(path) + strlen(root) + 16;
    
  Catalog *subcat;
  
  /* define subcat catalog file */
  ALLOCATE (subcat, Catalog, 1);
  dvo_catalog_init (subcat, TRUE);
  
  /* create basic data for measure catalog file */
  gfits_create_header (&subcat->header);

  subcat->catcompress = catalog->catcompress;

  ALLOCATE (subcat->filename, char, length);

  snprintf (subcat->filename, length, "%s/%s.%s", path, root, ext);

  char *file = filebasename (subcat->filename);
  gfits_modify (&catalog[0].header, tablename, "%s", 1, file);

  free (file);
  free (path);
  free (root);

  return subcat;
}

int dvo_catalog_set_range (Catalog *catalog) {

  int i;
  double Rmin, Rmax, Dmin, Dmax;

  /* determine RA,DEC range */
  Rmin = 360.0;
  Rmax =   0.0;
  Dmin = +90.0;
  Dmax = -90.0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Rmin = MIN (Rmin, catalog[0].average[i].R);
    Rmax = MAX (Rmax, catalog[0].average[i].R);
    Dmin = MIN (Dmin, catalog[0].average[i].D);
    Dmax = MAX (Dmax, catalog[0].average[i].D);
  }

  /* write RA,DEC range in header */
  gfits_modify (&catalog[0].header, "RA0",  "%lf", 1, Rmin);
  gfits_modify (&catalog[0].header, "DEC0", "%lf", 1, Dmin);
  gfits_modify (&catalog[0].header, "RA1",  "%lf", 1, Rmax);
  gfits_modify (&catalog[0].header, "DEC1", "%lf", 1, Dmax);
  return (TRUE);
}
