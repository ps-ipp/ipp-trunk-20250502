# include "skycells.h"

void GetConfig (char *config, char *field, char *format, int N, void *ptr);

int ConfigInit_skycells (int *argc, char **argv) {

  char *config, *file;
  char CatdirPhotcodeFile[512];
  char MasterPhotcodeFile[512];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  /* used by parse_time to find time-related keywords */
  strcpy (DateKeyword, "NONE");
  strcpy (DateMode, "NONE");
  strcpy (UTKeyword, "NONE");
  strcpy (JDKeyword, "NONE");
  strcpy (MJDKeyword, "NONE");
  ScanConfig (config, "DATE-KEYWORD",           "%s",  0, DateKeyword);
  ScanConfig (config, "DATE-MODE",              "%s",  0, DateMode);
  ScanConfig (config, "UT-KEYWORD",             "%s",  0, UTKeyword);
  ScanConfig (config, "MJD-KEYWORD",            "%s",  0, MJDKeyword);
  ScanConfig (config, "JD-KEYWORD",             "%s",  0, JDKeyword);

  ScanConfig (config, "EXPTIME-KEYWORD",        "%s",  0, ExptimeKeyword);
  ScanConfig (config, "AIRMASS-KEYWORD",        "%s",  0, AirmassKeyword);
  ScanConfig (config, "CCDNUM-KEYWORD",         "%s",  0, CCDNumKeyword);
  ScanConfig (config, "ST-KEYWORD",             "%s",  0, STKeyword);

  GetConfig (config, "GSCFILE",                	"%s",  0, GSCFILE);
  GetConfig (config, "CATDIR",                 	"%s",  0, CATDIR);
  GetConfig (config, "PHOTCODE_FILE",          	"%s",  0, MasterPhotcodeFile);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);
  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) {
    SKY_DEPTH = SKY_DEPTH_HST;
  }
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }
  sprintf (ImageCat, "%s/Images.dat", CATDIR);

  /* default mode, format, if not specified */
  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, MasterPhotcodeFile, !READONLY)) {
    fprintf (stderr, "error loading photcode table %s or master file %s\n", CatdirPhotcodeFile, MasterPhotcodeFile);
    exit (1);
  }

  free (config);
  free (file);
  return (TRUE);
}

void GetConfig (char *config, char *field, char *format, int N, void *ptr) {

  char *status;

  status = ScanConfig (config, field, format, N, ptr);
  if (status == NULL) {
    fprintf (stderr, "error in config, cannot find %s\n", field);
    exit (1);
  }
  return;
}
