# include "addstar.h"
# include "setobjflags.h"

void GetConfig (char *config, char *field, char *format, int N, void *ptr);

int ConfigInit_setobjflags (int *argc, char **argv) {

  double ZERO_POINT;
  char *config, *file;
  char RadiusWord[80];
  char CatdirPhotcodeFile[256];
  char MasterPhotcodeFile[256];

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  // force CATDIR to be absolute (so parallel mode will work)
  char tmpcatdir[DVO_MAX_PATH];
  GetConfig (config, "CATDIR" ,                 "%s",  0, tmpcatdir);
  CATDIR = abspath (tmpcatdir, DVO_MAX_PATH);

  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);
  if (!ScanConfig (config, "CATCOMPRESS",       "%s",  0, CATCOMPRESS)) {
    strcpy (CATCOMPRESS, "NONE");
  }
  GetConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  SetZeroPoint (ZERO_POINT);

  sprintf (ImageCat, "%s/Images.dat", CATDIR);

  /* set the default search radius */
  if (!ScanConfig (config, "ADDSTAR_RADIUS", "%s", 0, RadiusWord)) {
    GetConfig (config, "RADIUS", "%s", 0, RadiusWord);
  }
  SRC_RADIUS = atof (RadiusWord);
  if (SRC_RADIUS < 1e-6) {
    fprintf (stderr, "non-sensical correlation radius %f\n", SRC_RADIUS);
    exit (1);
  }

  /* default mode, format, if not specified */
  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, MasterPhotcodeFile, TRUE)) {
    fprintf (stderr, "error loading photcode table %s or master file %s\n", CatdirPhotcodeFile, MasterPhotcodeFile);
    exit (1);
  }

  FreeConfigFile();
  free (config);
  free (file);
  return TRUE;
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

void FreeConfig (void) {
  FREE (CATDIR);
}
