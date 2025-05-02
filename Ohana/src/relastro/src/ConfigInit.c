# include "relastro.h"

void ConfigInit (int *argc, char **argv) {

  char  *config, *file;
  char CatdirPhotcodeFile[DVO_MAX_PATH];
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

  if (!ScanConfig (config, "RELASTRO_IMFIT_CLIP_NITER",    "%d",  0, &IMFIT_CLIP_NITER))    IMFIT_CLIP_NITER  = 3;
  if (!ScanConfig (config, "RELASTRO_IMFIT_CLIP_NSIGMA",   "%lf", 0, &IMFIT_CLIP_NSIGMA))   IMFIT_CLIP_NSIGMA = 3.0;
  if (!ScanConfig (config, "RELASTRO_IMFIT_SYS_SIGMA_LIM", "%lf", 0, &IMFIT_SYS_SIGMA_LIM)) IMFIT_SYS_SIGMA_LIM = 0.01;
  if (!ScanConfig (config, "RELASTRO_IMFIT_TOO_FEW",       "%d",  0, &IMFIT_TOO_FEW))       IMFIT_TOO_FEW = 5;

  if (!ScanConfig (config, "PM_DT_MIN",              "%lf", 0, &PM_DT_MIN))        PM_DT_MIN = 0.25;   
  // if (!ScanConfig (config, "PM_TOOFEW",              "%d",  0, &PM_TOOFEW))	   PM_TOOFEW = 4;   
  // if (!ScanConfig (config, "POS_TOOFEW",             "%d",  0, &POS_TOOFEW))	   POS_TOOFEW = 1;  
  if (!ScanConfig (config, "PAR_FACTOR_MIN",         "%lf", 0, &PAR_FACTOR_MIN))   PAR_FACTOR_MIN = 0.2;
  if (!ScanConfig (config, "RELASTRO_MAP_NX",        "%d",  0, &NX_MAP))	   NX_MAP = 5;
  if (!ScanConfig (config, "RELASTRO_MAP_NY",        "%d",  0, &NY_MAP))	   NY_MAP = 5;
  if (!ScanConfig (config, "RELASTRO_DPOS_MAX",      "%lf", 0, &DPOS_MAX))	   DPOS_MAX = 6.0;    
  if (!ScanConfig (config, "ADDSTAR_RADIUS",         "%lf", 0, &ADDSTAR_RADIUS))   ADDSTAR_RADIUS = 1.0;

  if (!ScanConfig (config, "RELASTRO_MIN_DISTANCE_MOD",     "%lf", 0, &MIN_DISTANCE_MOD))     MIN_DISTANCE_MOD     =  7.5;
  if (!ScanConfig (config, "RELASTRO_MAX_DISTANCE_MOD",     "%lf", 0, &MAX_DISTANCE_MOD))     MAX_DISTANCE_MOD     = 15.0;
  if (!ScanConfig (config, "RELASTRO_MAX_DISTANCE_MOD_ERR", "%lf", 0, &MAX_DISTANCE_MOD_ERR)) MAX_DISTANCE_MOD_ERR =  1.0;

  if (!ScanConfig (config, "USE_FIXED_PIXCOORDS", "%d", 0, &USE_FIXED_PIXCOORDS))  USE_FIXED_PIXCOORDS = FALSE;
  if (!ScanConfig (config, "USE_GALAXY_MODEL",    "%d", 0, &USE_GALAXY_MODEL))     USE_GALAXY_MODEL = FALSE;
  if (!ScanConfig (config, "USE_ICRF_CORRECT",    "%d", 0, &USE_ICRF_CORRECT))     USE_ICRF_CORRECT = FALSE;
  if (!ScanConfig (config, "USE_ICRF_LOCAL",      "%d", 0, &USE_ICRF_LOCAL))       USE_ICRF_LOCAL = FALSE;
  if (!ScanConfig (config, "USE_ICRF_SHFIT",      "%d", 0, &USE_ICRF_SHFIT))       USE_ICRF_SHFIT = FALSE;
  if (!ScanConfig (config, "USE_ICRF_POLE",       "%d", 0, &USE_ICRF_POLE))        USE_ICRF_POLE = FALSE;

  // force CATDIR to be absolute (so parallel mode will work)
  GetConfig (config, "CATDIR",                 "%s",  0, CATDIR);
  char *tmpcatdir = abspath (CATDIR, DVO_MAX_PATH);
  strcpy (CATDIR, tmpcatdir);
  free (tmpcatdir);

  GetConfig (config, "GSCFILE",                "%s",  0, GSCFILE);
  ScanConfig(config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig(config, "CATFORMAT",              "%s",  0, CATFORMAT);

  // Defaults for WHERE_A are established in db_utils.c: intializeConstraints
  ScanConfig(config, "WHERE_A",                "%s",  0, WHERE_A);
  ScanConfig(config, "WHERE_B",                "%s",  0, WHERE_B);

  snprintf_nowarn (ImageCat, DVO_MAX_PATH, "%s/Images.dat", CATDIR);

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
  snprintf_nowarn (CatdirPhotcodeFile, DVO_MAX_PATH, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
  SetZeroPoint (25.0);

  FreeConfigFile();
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
