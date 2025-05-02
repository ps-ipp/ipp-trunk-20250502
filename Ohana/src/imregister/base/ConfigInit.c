# include "imregister.h"

int success;

void ConfigInit (int *argc, char **argv) {

  int i, NDB;
  char *config, *file, ElixirBase[80], catdir[MY_MAX_PATH];
  char CatdirPhotcodeFile[MY_MAX_PATH];
  char MasterPhotcodeFile[MY_MAX_PATH];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }

  success = TRUE;

  WarnConfig (config, "REGISTRATION_DATABASE",       "%s", 0, ImageDB);
  WarnConfig (config, "DETREND_DATABASE",            "%s", 0, DetrendDB);
  WarnConfig (config, "PHOT_DATABASE",               "%s", 0, PhotDB);
  WarnConfig (config, "TRANS_DATABASE",              "%s", 0, TransDB);

  WarnConfig (config, "CATDIR",                      "%s", 0, catdir);
  WarnConfig (config, "PHOTCODE_FILE",               "%s", 0, MasterPhotcodeFile);
  snprintf_nowarn (ImPhotDB, MY_MAX_PATH, "%s/Images.dat", catdir);

  /* small text databases: filters, camera defs */ 
  WarnConfig (config, "TEMPERATURE_LOG",             "%s", 0, TempLogFile);
  WarnConfig (config, "FILTER_LIST",                 "%s", 0, FilterList);
  WarnConfig (config, "CAMERA_CONFIG",               "%s", 0, CameraConfig);
  WarnConfig (config, "DETREND_RECIPES",             "%s", 0, RecipeFile);
						   
  /* pixel scale for FWHM */ 
  WarnConfig (config, "ASEC_PIX",                    "%lf", 0, &ARCSEC_PIXEL);

  /* keyword abstractions for parse_time */	   
  WarnConfig (config, "DATE-KEYWORD",                "%s", 0, DateKeyword);
  WarnConfig (config, "DATE-MODE",                   "%s", 0, DateMode);
  WarnConfig (config, "UT-KEYWORD",                  "%s", 0, UTKeyword);
  WarnConfig (config, "MJD-KEYWORD",                 "%s", 0, MJDKeyword);
  WarnConfig (config, "JD-KEYWORD",                  "%s", 0, JDKeyword);
						   
  /* keyword abstractions for iminfo */		   
  WarnConfig (config, "EXPTIME-KEYWORD",             "%s", 0, ExptimeKeyword);
  WarnConfig (config, "IMAGETYPE-KEYWORD",           "%s", 0, ImagetypeKeyword);
  WarnConfig (config, "CCDNUM-KEYWORD",              "%s", 0, CCDnumKeyword);
  WarnConfig (config, "FILTER-KEYWORD",              "%s", 0, FilterKeyword);
  WarnConfig (config, "AIRMASS-KEYWORD",             "%s", 0, AirmassKeyword);
  WarnConfig (config, "FOCUS-KEYWORD",               "%s", 0, FocusKeyword);
  WarnConfig (config, "ROTATION-KEYWORD",            "%s", 0, RotationKeyword);
  WarnConfig (config, "DETTEMP-KEYWORD",             "%s", 0, DettempKeyword);
  WarnConfig (config, "TELDATA1-KEYWORD",            "%s", 0, Teldata1Keyword);
  WarnConfig (config, "TELDATA2-KEYWORD",            "%s", 0, Teldata2Keyword);
  WarnConfig (config, "TELDATA3-KEYWORD",            "%s", 0, Teldata3Keyword);
  WarnConfig (config, "CAMERA-KEYWORD",              "%s", 0, CameraKeyword);

  ScanConfig (config, "CAMERA",                      "%s", 0, Camera);
  ScanConfig (config, "SEEING_REF_CCD",              "%s", 0, SeeingREFCCD);

  /* optional values */
  ScanConfig (config, "RA-DDD-KEYWORD",              "%s", 0, RADecDegKeyword);
  ScanConfig (config, "DEC-DDD-KEYWORD",             "%s", 0, DECDecDegKeyword);
  ScanConfig (config, "RA-HMS-KEYWORD",              "%s", 0, RASexigKeyword);
  ScanConfig (config, "DEC-DMS-KEYWORD",             "%s", 0, DECSexigKeyword);

  if (!RADecDegKeyword[0] & !DECDecDegKeyword[0] && !RASexigKeyword[0] && !DECSexigKeyword[0]) {
    fprintf (stderr, "missing astrometry configuration information\n");
    success = FALSE;
  }
						   
  WarnConfig (config, "imstats",                     "%s", 0, ElixirBase);
  snprintf_nowarn (ImstatFifo, MY_MAX_PATH, "%s.photcode", ElixirBase);   
  WarnConfig (config, "ptolemy",                     "%s", 0, ElixirBase);
  snprintf_nowarn (PtolemyFifo, MY_MAX_PATH, "%s.photcode", ElixirBase);

  if (!ScanConfig (config, "CONNECT", "%s",  0, CONNECT)) {
    snprintf_nowarn (CONNECT, 64, "/usr/bin/rsh");
  }

  /* load Detrend Alt Databases paths */
  NDB = 10;
  NDetrendAltDB = 0;
  ALLOCATE (DetrendAltDB, char *, NDB);
  ALLOCATE (DetrendAltDB[NDetrendAltDB], char, MY_MAX_PATH);
  for (i = 1; ScanConfig (config, "DETREND_ALT_DB", "%s", i, DetrendAltDB[NDetrendAltDB]); i++) {
    NDetrendAltDB ++;
    if (NDetrendAltDB == NDB) {
      NDB += 10;
      REALLOCATE (DetrendAltDB, char *, NDB);
    }
    ALLOCATE (DetrendAltDB[NDetrendAltDB], char, MY_MAX_PATH);
  }
  free (DetrendAltDB[NDetrendAltDB]);

  if (! success) {
    fprintf (stderr, "ERROR: problem with elixir configuration\n");
    exit (1);
  }

  /* XXX this does not yet write out the master photcode table */
  snprintf_nowarn (CatdirPhotcodeFile, MY_MAX_PATH, "%s/Photcodes.dat", catdir);
  if (!LoadPhotcodes (CatdirPhotcodeFile, MasterPhotcodeFile, TRUE)) {
    fprintf (stderr, "error loading photcode table %s or master file %s\n", CatdirPhotcodeFile, MasterPhotcodeFile);
    exit (1);
  }

  free (config);
  free (file);

}

void WarnConfig (char *config, char *key, char * mode, int N, void *var) {
  if (!ScanConfig (config, key, mode, N, var)) {
    fprintf (stderr, "missing config variable %s\n", key);
    success = FALSE;
  }
}
