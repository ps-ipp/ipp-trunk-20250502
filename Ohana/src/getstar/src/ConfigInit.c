# include "getstar.h"

# define MY_MAX_PATH 256

int ConfigInit (int *argc, char **argv) {

  char *config, *file;
  char CatdirPhotcodeFile[MY_MAX_PATH];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  ScanConfig (config, "GSCFILE",                "%s", 0, GSCFILE);
  ScanConfig (config, "CATDIR",                 "%s", 0, CATDIR);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);
  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) {
    SKY_DEPTH = 2;
  }
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  snprintf_nowarn (CatdirPhotcodeFile, MY_MAX_PATH, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
  SetZeroPoint (25.0);

  free (config);
  free (file);

  return (TRUE);
}
