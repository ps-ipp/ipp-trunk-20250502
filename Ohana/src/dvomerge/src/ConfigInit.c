# include "dvomerge.h"

int ConfigInit (int *argc, char **argv) {

  double ZERO_POINT;
  char RadiusWord[80];
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


  GetConfig (config, "GSCFILE",                	"%s",  0, GSCFILE);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);
  GetConfig (config, "SKY_DEPTH",              "%d",  0, &SKY_DEPTH);
  if (SKY_DEPTH > 4) {
      fprintf (stderr, "invalid SKY_DEPTH %d\n", SKY_DEPTH);
      exit (2);
  }

  /* default mode, format, if not specified */
  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  GetConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  SetZeroPoint (ZERO_POINT);

  /* set the default search radius */
  if (!ScanConfig (config, "ADDSTAR_RADIUS", "%s", 0, RadiusWord)) {
    GetConfig (config, "RADIUS", "%s", 0, RadiusWord);
  }
  RADIUS = atof (RadiusWord);
  if (RADIUS < 1e-6) {
      fprintf (stderr, "non-sensical correlation radius %f\n", RADIUS);
      exit (1);
  }

  VERBOSE = TRUE;

  FreeConfigFile();
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
