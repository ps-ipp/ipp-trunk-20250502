# include "markstar.h"

ConfigInit (int argc, char **argv) {

  char *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (&argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  /* used in other pipeline functions */
  ScanConfig (config, "CATDIR",          "%s",  0, CATDIR);
  ScanConfig (config, "GSCDIR",          "%s",  0, GSCDIR);
  ScanConfig (config, "GSCFILE",         "%s",  0, GSCFILE);
  sprintf (ImageCat, "%s/Images.dat", CATDIR);

  /* unique to markstar */
  ScanConfig (config, "SEARCH_RADIUS",   "%lf", 0, &RADIUS);
  ScanConfig (config, "TRAIL_WIDTH",     "%lf", 0, &TRAIL_WIDTH);
  ScanConfig (config, "NANGLE_BINS",     "%d",  0, &NBINS);
  ScanConfig (config, "NPTSINLINE",      "%d",  0, &NPTSINLINE);
  ScanConfig (config, "MIN_DENSITY",     "%lf", 0, &MIN_DENSITY);
  ScanConfig (config, "SPACE_SIGMA",     "%lf", 0, &NSIGMA); 

  ScanConfig (config, "BRIGHT_HALO_MAG",     "%lf", 0, &BRIGHT_HALO_MAG);
  ScanConfig (config, "BRIGHT_HALO_SLOPE",   "%lf", 0, &BRIGHT_HALO_SLOPE);
  ScanConfig (config, "BRIGHT_XTRAIL_WIDTH", "%lf", 0, &BRIGHT_XTRAIL_WIDTH);
  ScanConfig (config, "BRIGHT_XTRAIL_MAG",   "%lf", 0, &BRIGHT_XTRAIL_MAG);
  ScanConfig (config, "BRIGHT_XTRAIL_SLOPE", "%lf", 0, &BRIGHT_XTRAIL_SLOPE);
  ScanConfig (config, "BRIGHT_YTRAIL_WIDTH", "%lf", 0, &BRIGHT_YTRAIL_WIDTH);
  ScanConfig (config, "BRIGHT_YTRAIL_MAG",   "%lf", 0, &BRIGHT_YTRAIL_MAG);
  ScanConfig (config, "BRIGHT_YTRAIL_SLOPE", "%lf", 0, &BRIGHT_YTRAIL_SLOPE);

  ScanConfig (config, "GHOST_MAG",       "%lf", 0, &GHOST_MAG);
  ScanConfig (config, "GHOST_RADIUS",    "%lf", 0, &GHOST_RADIUS);
  ScanConfig (config, "OPTICAL_AXIS1",   "%lf", 0, &OPTICAL_AXIS1);
  ScanConfig (config, "OPTICAL_AXIS2",   "%lf", 0, &OPTICAL_AXIS2);
 
  free (config);
}
