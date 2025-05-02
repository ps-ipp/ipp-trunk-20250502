# include "dvoImageExtract.h"

int ConfigInit_extract (int *argc, char **argv) {

  char *config, *file;

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
  snprintf_nowarn (ImageCat, 256, "%s/Images.dat", CATDIR);

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  free (config);
  free (file);

  return (TRUE);
}
