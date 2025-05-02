# include "imregister.h"
# include "imphot.h"

int success;

void ConfigInitImphot (int *argc, char **argv) {

  int i, NDB;
  char *config, *file, ElixirBase[80], catdir[256];
  char CatdirPhotcodeFile[256];
  char MasterPhotcodeFile[256];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }

  success = TRUE;

  WarnConfig (config, "CATDIR",                      "%s", 0, catdir);
  WarnConfig (config, "PHOTCODE_FILE",         	     "%s", 0, MasterPhotcodeFile);
  sprintf (ImPhotDB, "%s/Images.dat", catdir);

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
  sprintf (ImstatFifo, "%s.photcode", ElixirBase);   
  WarnConfig (config, "ptolemy",                     "%s", 0, ElixirBase);
  sprintf (PtolemyFifo, "%s.photcode", ElixirBase);

  if (!ScanConfig (config, "CONNECT", "%s",  0, CONNECT)) {
    sprintf (CONNECT, "/usr/bin/rsh");
  }

  /* load Detrend Alt Databases paths */
  NDB = 10;
  NDetrendAltDB = 0;
  ALLOCATE (DetrendAltDB, char *, NDB);
  ALLOCATE (DetrendAltDB[NDetrendAltDB], char, 256);
  for (i = 1; ScanConfig (config, "DETREND_ALT_DB", "%s", i, DetrendAltDB[NDetrendAltDB]); i++) {
    NDetrendAltDB ++;
    if (NDetrendAltDB == NDB) {
      NDB += 10;
      REALLOCATE (DetrendAltDB, char *, NDB);
    }
    ALLOCATE (DetrendAltDB[NDetrendAltDB], char, 256);
  }
  free (DetrendAltDB[NDetrendAltDB]);

  if (! success) {
    fprintf (stderr, "ERROR: problem with elixir configuration\n");
    exit (1);
  }

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
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
