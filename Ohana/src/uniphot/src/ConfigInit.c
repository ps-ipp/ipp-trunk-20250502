# include "uniphot.h"

void GetConfig (char *config, char *field, char *format, int N, void *ptr);

void ConfigInit (int *argc, char **argv) {

  double ZERO_POINT;
  char  *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  GetConfig (config, "CATDIR",                 "%s",  0, CATDIR);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);

  snprintf_nowarn (ImageCat, 256, "%s/Images.dat", CATDIR);

  GetConfig (config, "GSCFILE",                 "%s",  0, GSCFILE);
  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) {
    SKY_DEPTH = 2;
  }
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }

  ScanConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  SetZeroPoint (ZERO_POINT);

  free (config);
  free (file);

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");
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
