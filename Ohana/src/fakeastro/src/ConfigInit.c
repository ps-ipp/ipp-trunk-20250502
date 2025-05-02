# include "fakeastro.h"

void ConfigInit (int *argc, char **argv) {

  char  *config, *file;
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

  // set defaults for all of these if they are not used by parallel / remote clients
  // if (!ScanConfig (config, "ADDSTAR_RADIUS",         "%lf", 0, &ADDSTAR_RADIUS))   ADDSTAR_RADIUS = 1.0;

  if ((FAKEASTRO_OP == OP_GALAXY) || (FAKEASTRO_OP == OP_2MASS) || (FAKEASTRO_OP == OP_GAIA)) {
    // force CATDIR to be absolute (so parallel mode will work)
    GetConfig (config, "CATDIR",                 "%s",  0, CATDIR);
    char *tmpcatdir = abspath (CATDIR, DVO_MAX_PATH);
    strcpy (CATDIR, tmpcatdir);
    free (tmpcatdir);
  }

  GetConfig (config, "GSCFILE",                "%s",  0, GSCFILE);
  ScanConfig(config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig(config, "CATFORMAT",              "%s",  0, CATFORMAT);
  GetConfig (config, "PHOTCODE_FILE",          "%s",  0, MasterPhotcodeFile);

  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) SKY_DEPTH = 2;
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) SKY_TABLE[0] = 0;

  
  if (!ScanConfig (config, "FAKEASTRO_NLOOP", "%d", 0, &FAKEASTRO_NLOOP)) {
    FAKEASTRO_NLOOP = 1;
  }
  if (!ScanConfig (config, "FAKEASTRO_NSTARS", "%d", 0, &FAKEASTRO_NSTARS)) {
    FAKEASTRO_NSTARS = 5000000;
  }
  if (!ScanConfig (config, "FAKEASTRO_NQSO_ICRF", "%d", 0, &FAKEASTRO_NQSO_ICRF)) {
    FAKEASTRO_NQSO_ICRF = 5000;
  }
  if (!ScanConfig (config, "FAKEASTRO_NQSO_ZERO", "%d", 0, &FAKEASTRO_NQSO_ZERO)) {
    FAKEASTRO_NQSO_ZERO = 200000;
  }
  if (!ScanConfig (config, "FAKEASTRO_ZGAL", "%f", 0, &FAKEASTRO_ZGAL)) {
    FAKEASTRO_ZGAL = 500.0; // parsec
  }
  if (!ScanConfig (config, "FAKEASTRO_RGAL", "%f", 0, &FAKEASTRO_RGAL)) {
    FAKEASTRO_RGAL = 2000.0; // parsec
  }
  if (!ScanConfig (config, "FAKEASTRO_REF_EPOCH", "%s", 0, FAKEASTRO_REF_EPOCH)) {
    strcpy (FAKEASTRO_REF_EPOCH, "2000/01/01,00:00:00"); // epoch of truth positions
  }
  if (!ScanConfig (config, "FAKEASTRO_2MASS_EPOCH", "%s", 0, FAKEASTRO_2MASS_EPOCH)) {
    strcpy (FAKEASTRO_2MASS_EPOCH, "2000/01/01,00:00:00"); // epoch of 2MASS astrometry
  }
  if (!ScanConfig (config, "FAKEASTRO_GAIA_EPOCH", "%s", 0, FAKEASTRO_GAIA_EPOCH)) {
    strcpy (FAKEASTRO_GAIA_EPOCH, "2015/01/01,00:00:00"); // epoch of GAIA astrometry
  }

  /* set the default search radius */
  if (!ScanConfig (config, "ADDSTAR_RADIUS", "%f", 0, &RADIUS)) {
    GetConfig (config, "RADIUS", "%f", 0, &RADIUS);
  }
  if (RADIUS < 0.0001) {
    fprintf (stderr, "absurd match radius %f\n", RADIUS);
    exit (2);
  }

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  char *CATDIR_CHECK = (FAKEASTRO_OP == OP_IMAGES) ? CATDIR_OUTPUT : CATDIR;

  // OP_2MASS is adding detections to an existing db, the others require and empty db
  if ((FAKEASTRO_OP != OP_2MASS) && (FAKEASTRO_OP != OP_GAIA)) {
    // check for existence of CATDIR
    struct stat filestat;
    int status = stat (CATDIR_CHECK, &filestat);
    if (!FORCE && (status == 0)) {
      fprintf (stderr, "directory %s exists, refusing to contaminate\n", CATDIR);
      exit (1);
    }
  }

  /* update master photcode table if not defined */
  snprintf_nowarn (CatdirPhotcodeFile, DVO_MAX_PATH, "%s/Photcodes.dat", CATDIR_CHECK);
  if (!LoadPhotcodes (CatdirPhotcodeFile, MasterPhotcodeFile, TRUE)) {
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
