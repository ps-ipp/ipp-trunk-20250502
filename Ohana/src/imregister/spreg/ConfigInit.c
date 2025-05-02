# include "imregister.h"
# include "spreg.h"

int success;

void ConfigInitSpec (int *argc, char **argv) {

  char *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }

  success = TRUE;

  WarnConfig (config, "SPECTRUM_DATABASE",       "%s", 0, SpectrumDB);

  /* keyword abstractions for parse_time */	   
  WarnConfig (config, "DATE-KEYWORD",                "%s", 0, DateKeyword);
  WarnConfig (config, "DATE-MODE",                   "%s", 0, DateMode);
  WarnConfig (config, "UT-KEYWORD",                  "%s", 0, UTKeyword);
  WarnConfig (config, "MJD-KEYWORD",                 "%s", 0, MJDKeyword);
  WarnConfig (config, "JD-KEYWORD",                  "%s", 0, JDKeyword);
						   
  /* keyword abstractions for spinfo */		   
  WarnConfig (config, "EXPTIME-KEYWORD",             "%s", 0, ExptimeKeyword);
  WarnConfig (config, "AIRMASS-KEYWORD",             "%s", 0, AirmassKeyword);
  WarnConfig (config, "CAMERA-KEYWORD",              "%s", 0, CameraKeyword);
  WarnConfig (config, "OBJECT-KEYWORD",              "%s", 0, ObjectKeyword);
  WarnConfig (config, "TELESCOPE-KEYWORD",           "%s", 0, TelescopeKeyword);

  /* semi-optional values */
  ScanConfig (config, "RA-DDD-KEYWORD",              "%s", 0, RADecDegKeyword);
  ScanConfig (config, "DEC-DDD-KEYWORD",             "%s", 0, DECDecDegKeyword);
  ScanConfig (config, "RA-HMS-KEYWORD",              "%s", 0, RASexigKeyword);
  ScanConfig (config, "DEC-DMS-KEYWORD",             "%s", 0, DECSexigKeyword);
  if (!RADecDegKeyword[0] & !DECDecDegKeyword[0] && !RASexigKeyword[0] && !DECSexigKeyword[0]) {
    fprintf (stderr, "missing astrometry configuration information\n");
    success = FALSE;
  }

  if (! success) {
    fprintf (stderr, "ERROR: problem with elixir configuration\n");
    exit (1);
  }

  free (config);
  free (file);

}
