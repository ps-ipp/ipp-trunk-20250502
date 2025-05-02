# include "markrock.h"

ConfigInit (int *argc, char **argv) {

  char *config, *file;
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

  /* used in other pipeline functions */
  ScanConfig (config, "CATDIR",                 "%s",  0, CATDIR);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);

  /* unique to markrock */
  ScanConfig (config, "ROCK_NEIGHBOR_RADIUS",   "%lf", 0, &ROCK_NEIGHBOR_RADIUS);
  ScanConfig (config, "ROCK_NEIGHBOR_NMAX",     "%d",  0, &ROCK_NEIGHBOR_NMAX);

  ScanConfig (config, "ROCK_RADIUS",            "%lf", 0, &RADIUS);
  ScanConfig (config, "ROCK_MAX_RADIUS",        "%lf", 0, &MAX_RADIUS);
  ScanConfig (config, "ROCK_MAX_SPEED",         "%lf", 0, &MAX_SPEED);
  ScanConfig (config, "ROCK_MAX_DELAY",         "%lf", 0, &MAX_DELAY);
  ScanConfig (config, "ROCK_CATALOG",           "%s",  0, RockCat);

  ScanConfig (config, "BRIGHT_HALO_MAG",        "%lf", 0, &BRIGHT_HALO_MAG);
  ScanConfig (config, "BRIGHT_HALO_SLOPE",      "%lf", 0, &BRIGHT_HALO_SLOPE);
  ScanConfig (config, "GSCFILE",                "%s",  0, GSCFILE);
  ScanConfig (config, "GSCDIR",                 "%s",  0, GSCDIR);
  ScanConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);

  if (*CATMODE == 0) strcpy (CATMODE, "RAW");
  if (*CATFORMAT == 0) strcpy (CATFORMAT, "ELIXIR");

  /* XXX this does not yet write out the master photcode table */
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }

  free (config);
  free (file);

}
