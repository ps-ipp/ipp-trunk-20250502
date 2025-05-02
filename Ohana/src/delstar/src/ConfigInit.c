# include "delstar.h"

void ConfigInit (int *argc, char **argv) {

  char *config, *file;
  char CatdirPhotcodeFile[256];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  ScanConfig (config, "NSIGMA",                 "%lf", 0, &NSIGMA);
  ScanConfig (config, "ALPHA",                  "%lf", 0, &ALPHA);
  ScanConfig (config, "GSCFILE",                "%s", 0, GSCFILE);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);

  // force CATDIR to be absolute (so parallel mode will work)
  char *tmpcatdir = NULL;
  ALLOCATE (tmpcatdir, char, DVO_MAX_PATH);
  ScanConfig (config, "CATDIR",                 "%s",  0, tmpcatdir);
  CATDIR = abspath (tmpcatdir, DVO_MAX_PATH);
  free (tmpcatdir);

  snprintf (ImageCat, DVO_MAX_PATH, "%s/Images.dat", CATDIR);

  ScanConfig (config, "DATE-KEYWORD",           "%s", 0, DateKeyword);
  ScanConfig (config, "DATE-MODE",              "%s", 0, DateMode);
  ScanConfig (config, "UT-KEYWORD",             "%s", 0, UTKeyword);
  ScanConfig (config, "MJD-KEYWORD",            "%s", 0, MJDKeyword);
  ScanConfig (config, "JD-KEYWORD",             "%s", 0, JDKeyword);

  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) {
    SKY_DEPTH = 2;
  }
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }

  FreeConfigFile();
  free (config);
  free (file);
}
