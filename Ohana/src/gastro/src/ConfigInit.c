# include "gastro.h"

void ConfigInit (int *argc, char **argv) {
  
  char *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  ScanConfig (config, "OFFSET_RADIUS",     "%lf", 0, &SEARCH_RADIUS);   // max allowed offset in gcenter
  ScanConfig (config, "MIN_MATCHES",       "%d",  0, &MIN_MATCHES);     // min allowed fitted stars 
  ScanConfig (config, "DEFAULT_RADIUS",    "%lf", 0, &DEFAULT_RADIUS);  // starting radius for matched fit
  ScanConfig (config, "MINIMUM_RADIUS",    "%lf", 0, &MINIMUM_RADIUS);  // min allowed radius for matched fit
  ScanConfig (config, "MAX_ERROR",         "%lf", 0, &MAX_ERROR);       // max allowed error for valid solution
  ScanConfig (config, "MAX_NONLINEAR",     "%lf", 0, &MAX_NONLINEAR);   // max allowed shear |(11*22 - 12*21) / (L1*L2)|
  ScanConfig (config, "CCD_PC1_1",         "%lf", 0, &CCD_PC1_1);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC2_2",         "%lf", 0, &CCD_PC2_2);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC1_2",         "%lf", 0, &CCD_PC1_2);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC2_1",         "%lf", 0, &CCD_PC2_1);       // guess if WCS is missing
  ScanConfig (config, "ASEC_PIX",          "%lf", 0, &ASEC_PIX);        // guess if WCS is missing
  ScanConfig (config, "NFIELD",            "%lf", 0, &NFIELD);          // search region in field units
  ScanConfig (config, "NPOLYTERMS",        "%d",  0, &NPOLYTERMS);      // fit order
  ScanConfig (config, "ROT_ZERO",          "%lf", 0, &ROT_ZERO);        // rotation search region
  ScanConfig (config, "dROT",              "%lf", 0, &dROT);            // rotation search region
  ScanConfig (config, "NROT",              "%d",  0, &NROT);            // rotation search region
  ScanConfig (config, "POLAR_ALIGNMENT",   "%d",  0, &POLAR_ALIGNMENT); // apply polar alignment correction
  ScanConfig (config, "POLAR_AXIS_RA",     "%lf", 0, &POLE_RA);         // true coords of pole (should be HA, not RA)
  ScanConfig (config, "POLAR_AXIS_DEC",    "%lf", 0, &POLE_DEC);        // true coords of pole
  ScanConfig (config, "RA_OFFSET",         "%lf", 0, &RA_OFFSET);       // ?? not well defined (should be euler angle)
  ScanConfig (config, "DEC_OFFSET",        "%lf", 0, &DEC_OFFSET);      // ?? not well defined (should be euler angle)
  ScanConfig (config, "LONEOS_REGIONS",    "%s",  0, LONEOS_REGION_FILE); // table of LONEOS regions to fix guess astrometry

  ScanConfig (config, "GSCFILE",           "%s",  0, GSCFILE);          // location of sky table
  ScanConfig (config, "GSCDIR",            "%s",  0, GSCDIR);           // location of HST GSC data 
  ScanConfig (config, "USNO_CDROM",        "%s",  0, CDROM);            // location of USNO A data (USNO_A_DIR in gastro2)
  ScanConfig (config, "ASTRO_REFCAT",      "%s",  0, REFCAT);           // which astrometry catalog to use
  ScanConfig (config, "CATDIR",            "%s",  0, CATDIR);           // location of ptolemy-format ref data
  ScanConfig (config, "ROUGH_ASTROMETRY",  "%s",  0, ROUGH_ASTROMETRY); // where to get initial guess (header, config)

  if (strcasecmp (ROUGH_ASTROMETRY, "header") && 
      strcasecmp (ROUGH_ASTROMETRY, "config")) {
    fprintf (stderr, "ROUGH_ASTROMETRY must be one of: header, config\n");
    exit (0);
  }
  free (config);
  free (file);
}
