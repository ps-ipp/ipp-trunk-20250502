# include "gastro2.h"

void ConfigInit (int *argc, char **argv) {
  
  char *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  /* default values for config variables: used if key is missing from the config file */
  strcpy (ROUGH_ASTROMETRY, "header");

  ScanConfig (config, "CCD_PC1_1",         "%lf", 0, &CCD_PC1_1);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC2_2",         "%lf", 0, &CCD_PC2_2);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC1_2",         "%lf", 0, &CCD_PC1_2);       // guess if WCS is missing
  ScanConfig (config, "CCD_PC2_1",         "%lf", 0, &CCD_PC2_1);       // guess if WCS is missing
  ScanConfig (config, "ASEC_PIX",          "%lf", 0, &ASEC_PIX);        // guess if WCS is missing
  ScanConfig (config, "NFIELD",            "%lf", 0, &NFIELD);          // search region *padding* in field units
  ScanConfig (config, "NGRID_PIX",         "%d",  0, &NGRID_PIX);       // resolution of grid search (pixels)
  ScanConfig (config, "NPOLYTERMS",        "%d",  0, &NPOLYTERMS);      // high-order fit terms (2 or 3)
  ScanConfig (config, "ROT_ZERO",          "%lf", 0, &ROT_ZERO);        // rotation search region
  ScanConfig (config, "dROT",              "%lf", 0, &dROT);            // rotation search region
  ScanConfig (config, "NROT",              "%d",  0, &NROT);            // rotation search region
  ScanConfig (config, "POLAR_ALIGNMENT",   "%d",  0, &POLAR_ALIGNMENT); // apply polar alignment correction
  ScanConfig (config, "POLAR_AXIS_RA",     "%lf", 0, &POLE_RA);         // true coords of pole (should be HA, not RA)
  ScanConfig (config, "POLAR_AXIS_DEC",    "%lf", 0, &POLE_DEC);        // true coords of pole
  ScanConfig (config, "RA_OFFSET",         "%lf", 0, &RA_OFFSET);       // ?? not well defined (should be euler angle)
  ScanConfig (config, "DEC_OFFSET",        "%lf", 0, &DEC_OFFSET);      // ?? not well defined (should be euler angle)

  /* possible sources of astrometric reference data */
  if (!ScanConfig (config, "USNO_A_DIR",             "%s",  0, USNO_A_DIR)) {  // location of USNO A data (USNO_CDROM in gastro)
    ScanConfig (config, "USNO_CDROM",             "%s",  0, USNO_A_DIR);  // alternate location of USNO A data
  }
  ScanConfig (config, "USNO_B_DIR",        "%s",  0, USNO_B_DIR);       // location of USNO B ref data
  ScanConfig (config, "GSCDIR",            "%s",  0, GSCDIR);           // location of HST GSC ref data 
  ScanConfig (config, "2MASS_DIR",         "%s",  0, TWO_MASS_DIR);  	// location of 2MASS ref data 
  ScanConfig (config, "ASTROM_CATDIR",     "%s",  0, ASTROM_CATDIR); 	// location of ptolemy-format ref data

  ScanConfig (config, "GSCFILE",           "%s",  0, GSCFILE);          // location of sky table
  ScanConfig (config, "ASTRO_REFCAT",      "%s",  0, REFCAT);           // which astrometry catalog to use
  ScanConfig (config, "ROUGH_ASTROMETRY",  "%s",  0, ROUGH_ASTROMETRY); // where to get initial guess (header, config)
  ScanConfig (config, "GASTRO_MAX_NSTARS", "%d",  0, &GASTRO_MAX_NSTARS); // max number of stars from image to fit
  ScanConfig (config, "GASTRO_MAX_MAG_ERROR", "%lf", 0, &MAX_ERROR);    // S/N limit on image stars used in fit

  ScanConfig (config, "PHOTCODE_FILE",     "%s",  0, PhotCodeFile);  // not used

  if (NFIELD <= 0.0) {
      fprintf (stderr, "NFIELD is not sensible: choose a non-zero number\n");
      exit (1);
  }

  if (!GASTRO_MAX_NSTARS) GASTRO_MAX_NSTARS = 300;
  if (!MAX_ERROR) MAX_ERROR = 0.2;
  if (!NGRID_PIX) NGRID_PIX = 50.0;
  if (!NFIELD) NFIELD = 0.1;
  
  if (strcasecmp (ROUGH_ASTROMETRY, "header") && 
      strcasecmp (ROUGH_ASTROMETRY, "config")) {
    fprintf (stderr, "ROUGH_ASTROMETRY must be one of: header, config\n");
    exit (1);
  }
  free (config);
  free (file);
}
