# include "mosastro.h"

void ConfigInit (int *argc, char **argv) {
  
  char *config, *file;

  VERBOSE = TRUE;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  ScanConfig (config, "GSCFILE",          "%s",  0, GSCFILE);

  GetConfig (config, "ASTRO_REFCAT",      "%s",  0, REFCAT);

  /* possible sources of astrometric reference data */
  if (!ScanConfig (config, "USNO_A_DIR",             "%s",  0, USNO_A_DIR)) {
    ScanConfig (config, "USNO_CDROM",             "%s",  0, USNO_A_DIR);
  }
  ScanConfig (config, "USNO_B_DIR",       "%s",  0, USNO_B_DIR);
  ScanConfig (config, "GSCDIR",           "%s",  0, GSCDIR);
  ScanConfig (config, "STONE_DIR",        "%s",  0, StoneRegions);
  ScanConfig (config, "2MASS_DIR",        "%s",  0, TWO_MASS_DIR);
  ScanConfig (config, "ASTROM_CATDIR",    "%s",  0, ASTROM_CATDIR);

  /* abstracted header keywords - used by parse_time */
  ScanConfig (config, "DATE-KEYWORD",     "%s",  0, DateKeyword);
  ScanConfig (config, "DATE-MODE",        "%s",  0, DateMode);
  ScanConfig (config, "UT-KEYWORD",       "%s",  0, UTKeyword);
  ScanConfig (config, "MJD-KEYWORD",      "%s",  0, MJDKeyword);
  ScanConfig (config, "JD-KEYWORD",       "%s",  0, JDKeyword);
  ScanConfig (config, "EXPTIME-KEYWORD",  "%s",  0, ExptimeKeyword);

  GetConfig  (config, "RADIUS",           "%lf", 0, &RADIUS);
  GetConfig  (config, "SIGMA_LIM",        "%lf", 0, &SIGMA_LIM);
  ScanConfig (config, "ZERO_PT",          "%lf", 0, &ZERO_POINT);

  ScanConfig (config, "INST_MAG_MIN",     "%lf", 0, &IMAG_MIN);
  ScanConfig (config, "INST_MAG_MAX",     "%lf", 0, &IMAG_MAX);
  ScanConfig (config, "INST_BRIGHT",      "%lf", 0, &INST_BRIGHT);
  
  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  free (config);
  free (file);

  return;
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
