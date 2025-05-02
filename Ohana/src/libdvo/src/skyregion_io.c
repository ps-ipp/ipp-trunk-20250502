# include "dvo.h"

SkyTable *SkyTableLoad (char *filename, int VERBOSE) {

  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  SkyTable *skytable;
  FILE *f;
  int i;
  
  f = fopen (filename, "r");
  if (f == NULL) {
    if (VERBOSE) fprintf (stderr, "can't find Sky Region file %s\n", filename);
    return (NULL);
  }

  /* load in table data */
  ftable.header = &theader;
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Sky Region header\n");
    fclose (f);
    return (NULL);
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Sky Region matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return (NULL);
  }
  if (!gfits_fread_ftable (f, &ftable, "SKY_REGION")) {
    if (VERBOSE) fprintf (stderr, "can't read Sky Region table\n");
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    fclose (f);
    return (NULL);
  }

  ALLOCATE (skytable, SkyTable, 1);
  memset (skytable->hosts, 0, 80);
  skytable[0].regions = gfits_table_get_SkyRegion (&ftable, &skytable[0].Nregions, NULL, NULL);
  if (!skytable[0].regions) {
    fprintf (stderr, "ERROR: failed to read sky regions\n");
    exit (2);
  }

  ALLOCATE (skytable[0].filename, char *, skytable[0].Nregions);
  for (i = 0; i < skytable[0].Nregions; i++) {
    skytable[0].filename[i] = NULL;
  }
  
  // 2012.02.08 : I updated the schema for SkyTable.dat to add 3 fields by stealing the
  // last 3 bytes of the 'name' field (going from 21 bytes to 18 bytes).  This was safe
  // because no files had yet been defined with more than 16 bytes in the names, leave
  // room for 1 more byte includig the EOF char.

  // When we load old tables, we need to zero out the values of those 3 new fields (they
  // may have garbage left over).  To distinguish old and new tables, I add the field
  // 'HOSTS' to the PHU header (which also tells the location of the host table, if used).

  int haveHosts = gfits_scan (&header, "HOSTS", "%s", 1, skytable->hosts);
  if (!haveHosts) {
    strcpy (skytable->hosts, "HostTable.dat");
    for (i = 0; i < skytable[0].Nregions; i++) {
      if (strlen(skytable[0].regions[i].name) > 17) {
	fprintf (stderr, "WARNING : this skytable has some names longer than the adjusted size of 17 bytes.\n");
	fprintf (stderr, "  This is not compatible with the current SkyTable definition\n");
	exit (1);
      }
      skytable[0].regions[i].hostFlags = 0;
      skytable[0].regions[i].hostID    = 0;
      skytable[0].regions[i].backupID  = 0;
    }
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  fclose (f);

  return (skytable);
}

int SkyTableSave (SkyTable *skytable, char *filename) {

  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  FILE *f;

  /* make phu header (no matrix needed) */
  ftable.header = &theader;
  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "HOSTS", "%s", 1, skytable->hosts);

  gfits_table_set_SkyRegion (&ftable, skytable[0].regions, skytable[0].Nregions, TRUE);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) { 
    fprintf (stderr, "cannot open %s for output\n", filename);
    return (FALSE);
  }
  
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);
  fclose (f);

  gfits_free_header  (&header);
  gfits_free_matrix  (&matrix);
  gfits_free_header  (&theader);
  gfits_free_table   (&ftable);

  return (TRUE);
}

// load the skytable from the best location:
// 1) if we already have a defined catdir with skytable.fits, use that
// 2) in some cases, user may supply 'SKYFILE' with explicit path (this is not really used)
// 3) if a file does not exist, create a new skytable from the GSC reference
//    the GSC reference file contains a hard-code list of region names
//    and boundaries.

SkyTable *SkyTableLoadOptimal (char *catdir, char *skyfile, char *gscfile, int readwrite, int depth, int verbose) {

  char *filename;
  struct stat filestat;
  SkyTable *sky;

  /* first option: in CATDIR */
  filename = SkyTableFilename (catdir);
  if (stat (filename, &filestat)) goto SKYFILE;
  if (!check_file_access (filename, FALSE, readwrite, verbose)) goto SKYFILE;
  sky = SkyTableLoad (filename, verbose);
  if (sky == NULL) {
    fprintf (stderr, "error loading sky table\n");
    exit (1);
  }
  free (filename);
  return (sky);

SKYFILE:
  /* second option: SKYFILE */
  if (skyfile == NULL) goto GSCFILE;
  if (skyfile[0] != 0) goto GSCFILE;
  if (stat (skyfile, &filestat)) goto GSCFILE;
  if (!check_file_access (skyfile, FALSE, readwrite, verbose)) goto GSCFILE;
  sky = SkyTableLoad (skyfile, verbose);
  if (sky == NULL) {
    fprintf (stderr, "error loading sky table\n");
    free (filename);
    return (NULL);
  }
  /* set the depths to the default depth */
  SkyTableSetDepth (sky, depth);

  /* create CATDIR copy */
  check_file_access (filename, FALSE, readwrite, verbose);
  if (!SkyTableSave (sky, filename)) {
    free (filename);
    return NULL;
  }

  gfits_convert_SkyRegion (sky[0].regions, sizeof (SkyTable), sky[0].Nregions);
  free (filename);
  return (sky);

GSCFILE:

  /* third option: GSCFILE */

  if (gscfile == NULL) {
    fprintf (stderr, "error loading sky table from existing db (%s)\n", catdir);
    free (filename);
    return (NULL);
  }

  sky = SkyTableFromGSC (gscfile, depth, verbose);
  if (sky == NULL) {
    fprintf (stderr, "error loading sky table\n");
    free (filename);
    return (NULL);
  }

  /* create CATDIR copy */
  check_file_access (filename, FALSE, readwrite, verbose);
  if (!SkyTableSave (sky, filename)) {
    free (filename);
    return NULL;
  }

  gfits_convert_SkyRegion (sky[0].regions, sizeof (SkyRegion), sky[0].Nregions);
  free (filename);
  return (sky);
}

int SkyListSetFilenames (SkyList *list, char *path, char *ext) {

  int i;
  char line[256];

  // this generates the names, be sure to free when not needed
  for (i = 0; i < list[0].Nregions; i++) {
    snprintf (line, 256, "%s/%s.%s", path, list[0].regions[i][0].name, ext);
    list[0].filename[i] = strcreate (line);
  }

  return (TRUE);
}

int SkyTableSetFilenames (SkyTable *sky, char *path, char *ext) {

  int i;
  char line[256];

  // this generates the names, be sure to free when not needed
  for (i = 0; i < sky[0].Nregions; i++) {
    snprintf (line, 256, "%s/%s.%s", path, sky[0].regions[i].name, ext);
    sky[0].filename[i] = strcreate (line);
  }

  return (TRUE);
}

// given a CATDIR, return an allocated string with the full skytable name
char *SkyTableFilename (char *catdir) {

  if (!catdir) return NULL;
  
  char *skyfile;

  int Nchar = strlen(catdir) + strlen("/SkyTable.fits") + 16;
  ALLOCATE (skyfile, char, Nchar);
  snprintf (skyfile, Nchar, "%s/SkyTable.fits", catdir);

  return skyfile;
}
