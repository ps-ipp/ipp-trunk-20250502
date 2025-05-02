# include "imclean.h"

void ConfigInit (int *argc, char **argv) {
  
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

  ScanConfig (config, "ZERO_PT",           "%lf", 0, &ZERO_POINT);
  ScanConfig (config, "MIN_SN_FSTAT",      "%lf", 0, &MIN_SN_FSTAT);
  ScanConfig (config, "DEFAULT_ERROR_FSTAT", "%lf", 0, &DEFAULT_ERROR);
  ScanConfig (config, "DOPHOT_CHAR_LINE",  "%d", 0, &CHAR_LINE);
  ScanConfig (config, "DOPHOT_TYPE_FIELD", "%d", 0, &TYPE_FIELD);
  ScanConfig (config, "DOPHOTF_FIELD",  "%d", 0, &PSF_FIELD);
  ScanConfig (config, "DOPHOT_AP_FIELD",   "%d", 0, &AP_FIELD);
  ScanConfig (config, "PHOTCODE_FILE",     "%s", 0, PhotCodeFile);

  /* unique to markstar */
  ScanConfig (config, "SEARCH_RADIUS",   "%lf", 0, &RADIUS);
  ScanConfig (config, "TRAIL_WIDTH",     "%lf", 0, &TRAIL_WIDTH);
  ScanConfig (config, "NANGLE_BINS",     "%d",  0, &NBINS);
  ScanConfig (config, "NPTSINLINE",      "%d",  0, &NPTSINLINE);
  ScanConfig (config, "MIN_DENSITY",     "%lf", 0, &MIN_DENSITY);
  ScanConfig (config, "SPACE_SIGMA",     "%lf", 0, &NSIGMA); 

  free (config);
  free (file);
}
