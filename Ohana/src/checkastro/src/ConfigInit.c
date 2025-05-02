# include "checkastro.h"

void ConfigInit (int *argc, char **argv) {

  char  *config, *file;
  char CatdirPhotcodeFile[256];
  struct stat filestat;
  int status;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  // set defaults for all of these if they are not used by parallel / remote clients
  if (!ScanConfig (config, "RELASTRO_SIGMA_LIM",         "%lf", 0, &SIGMA_LIM))       SIGMA_LIM = 0.01; 
  if (!ScanConfig (config, "RELASTRO_SRC_MEAS_TOOFEW",   "%d",  0, &SRC_MEAS_TOOFEW)) SRC_MEAS_TOOFEW = 3;

  // force CATDIR to be absolute (so parallel mode will work)
  GetConfig (config, "CATDIR",                 "%s",  0, CATDIR);
  char *tmpcatdir = abspath (CATDIR, DVO_MAX_PATH);
  strcpy (CATDIR, tmpcatdir);
  free (tmpcatdir);

  // GetConfig (config, "GSCFILE",                "%s",  0, GSCFILE);
  ScanConfig(config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig(config, "CATFORMAT",              "%s",  0, CATFORMAT);

  sprintf (ImageCat, "%s/Images.dat", CATDIR);

  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) SKY_DEPTH = 2;
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) SKY_TABLE[0] = 0;

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  // check for existence of CATDIR
  status = stat (CATDIR, &filestat);
  if (status == -1) {
    fprintf (stderr, "directory %s does not exist, giving up\n", CATDIR);
    exit (1);
  }

  /* update master photcode table if not defined */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
  SetZeroPoint (25.0);

  free (config);
  free (file);

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
