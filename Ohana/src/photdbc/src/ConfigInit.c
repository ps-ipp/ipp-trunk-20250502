# include "photdbc.h"

int success = TRUE;

void WarnConfig (char *config, char *key, char * mode, int N, void *var) {
  if (!ScanConfig (config, key, mode, N, var)) {
    fprintf (stderr, "missing config variable %s\n", key);
    success = FALSE;
  }
}

void ConfigInit (int *argc, char **argv) {

  char *config, *file;
  char CatdirPhotcodeFile[DVO_MAX_PATH];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  // XXX join_stars (in photdbc.c) is currently disabled
  // WarnConfig (config, "PHOTDBC_JOIN_RADIUS",       "%lf", 0, &JOIN_RADIUS);

  // XXX unique_measures (in photdbc.c) is currently disabled
  // WarnConfig (config, "UNIQ_RADIUS",            "%lf", 0, &UNIQ_RADIUS);

  // XXX flag_measures (in photdbc.c) is currently disabled
  // WarnConfig (config, "XMIN",                   "%lf", 0, &XMIN);
  // WarnConfig (config, "XMAX",                   "%lf", 0, &XMAX);
  // WarnConfig (config, "YMIN",                   "%lf", 0, &YMIN);
  // WarnConfig (config, "YMAX",                   "%lf", 0, &YMAX);
  
  // WarnConfig (config, "DMCAL_MIN",              "%lf", 0, &tmp);  DMCAL_MIN = 1000*tmp;
  // WarnConfig (config, "MMIN",                   "%lf", 0, &tmp);  MMIN      = 1000*tmp;
  // WarnConfig (config, "MMAX",                   "%lf", 0, &tmp);  MMAX      = 1000*tmp;
  // WarnConfig (config, "CHISQ_MAX",              "%lf", 0, &CHISQ_MAX);

  // XXX get_mags (in photdbc.c) is currently disabled
  // WarnConfig (config, "DMSYS",                  "%lf", 0, &tmp);  DMSYS     = SQ(1000*tmp);
  // WarnConfig (config, "DMGAIN",                 "%lf", 0, &DMGAIN); 

  ScanConfig (config, "SIGMA_MAX",              "%lf", 0, &SIGMA_MAX);
  ScanConfig (config, "AVE_SIGMA_LIM",          "%lf", 0, &AVE_SIGMA_LIM);
  ScanConfig (config, "NMEAS_MIN",              "%d",  0, &NMEAS_MIN);
  ScanConfig (config, "NMEAS_MIN_FILTERED",     "%d",  0, &NMEAS_MIN_FILTERED);

  // do not apply this limit for now
  NCODE_MIN = 0;

  WarnConfig (config, "CATDIR",                 "%s",  0, CATDIR);
  char *tmpcatdir = abspath (CATDIR, DVO_MAX_PATH);
  strcpy (CATDIR, tmpcatdir);
  free (tmpcatdir);

  snprintf_nowarn (ImageCat, DVO_MAX_PATH, "%s/Images.dat", CATDIR);

  WarnConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);

  // NOTE: in this program, CATFORMAT, CATMODE, CATCOMPRESS may only be set 
  // as command-line options, eg: -set-format PS1_V5

  if (!success) { 
    fprintf (stderr, "missing config parameter\n");
    exit (1);
  }

  /* XXX this does not yet write out the master photcode table */
  snprintf_nowarn (CatdirPhotcodeFile, DVO_MAX_PATH, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
  SetZeroPoint (ZERO_POINT);

  free (config);
  free (file);
}
