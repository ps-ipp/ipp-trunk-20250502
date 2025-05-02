# include "relphot.h"

// do not use with %s
# define DefConfig(NAME, FMT, DEF, VAR) {				\
  char *status = ScanConfig (config, NAME, FMT, 0, &VAR);		\
  if (status == NULL) { VAR = DEF; }					\
  }

void ConfigInit (int *argc, char **argv) {

  double ZERO_POINT;
  char  *config, *file;
  char CatdirPhotcodeFile[256];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  GetConfig (config, "MAG_LIM",                "%lf", 0, &MAG_LIM);
  GetConfig (config, "SIGMA_LIM",              "%lf", 0, &SIGMA_LIM);

  DefConfig ("STAR_SCATTER",                    "%lf",  0.1, STAR_SCATTER);
  DefConfig ("STAR_CHISQ",                      "%lf", 10.0, STAR_CHISQ);

  DefConfig ("NIGHT_SCATTER",                   "%lf",  0.1, NIGHT_SCATTER);
  DefConfig ("NIGHT_CHISQ",                     "%lf", 10.0, NIGHT_CHISQ);

  DefConfig ("MOSAIC_SCATTER",                  "%lf",  0.1, MOSAIC_SCATTER);
  DefConfig ("MOSAIC_CHISQ",                    "%lf", 10.0, MOSAIC_CHISQ);

  GetConfig (config, "IMAGE_SCATTER",          "%lf", 0, &IMAGE_SCATTER);
  GetConfig (config, "IMAGE_OFFSET",           "%lf", 0, &IMAGE_OFFSET);

  GetConfig (config, "GRID_TOOFEW",            "%d",  0, &GRID_TOOFEW);
  GetConfig (config, "STAR_TOOFEW",            "%d",  0, &STAR_TOOFEW);
  GetConfig (config, "IMAGE_TOOFEW",           "%d",  0, &IMAGE_TOOFEW);
  GetConfig (config, "IMAGE_GOOD_FRACTION",    "%lf", 0, &IMAGE_GOOD_FRACTION);

  // force CATDIR to be absolute (so parallel mode will work)
  char *tmpcatdir = NULL;
  ALLOCATE (tmpcatdir, char, DVO_MAX_PATH);
  GetConfig (config, "CATDIR",                 "%s",  0, tmpcatdir);
  CATDIR = abspath (tmpcatdir, DVO_MAX_PATH);
  free (tmpcatdir);

  GetConfig (config, "CAMERA",                  "%s",  0, CAMERA);

  GetConfig (config, "GSCFILE",                 "%s",  0, GSCFILE);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);

  snprintf (ImageCat, DVO_MAX_PATH, "%s/Images.dat", CATDIR);

  DefConfig ("RELPHOT_IMFIT_SYS_SIGMA_LIM", "%lf", 0.01, IMFIT_SYS_SIGMA_LIM);
  DefConfig ("SKY_DEPTH",                   "%d",     2, SKY_DEPTH);

  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }

  GetConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  GetConfig (config, "CAMERA_CONFIG",          "%s",  0, CameraConfig);
  GetConfig (config, "MOSAICNAME",             "%s",  0, MOSAICNAME);

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
  SetZeroPoint (ZERO_POINT);

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

