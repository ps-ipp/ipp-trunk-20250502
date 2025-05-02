# include "addstar.h"

AddstarClientOptions ConfigInit (int *argc, char **argv) {

  double ZERO_POINT;
  char *config, *file;
  char RadiusWord[80], tmpword[80];
  char CatdirPhotcodeFile[256];
  char MasterPhotcodeFile[256];
  AddstarClientOptions options;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);


  /* exclude overscan region from the dB image boundaries */
  XOVERSCAN = YOVERSCAN = 0;
  ScanConfig (config, "XOVERSCAN",              "%d",  0, &XOVERSCAN);
  ScanConfig (config, "YOVERSCAN",              "%d",  0, &YOVERSCAN);

  /* only upload stars within region; a value of 0 means ignore the limit */
  XMIN = XMAX = YMIN = YMAX = 0;
  ScanConfig (config, "ADDSTAR_XMIN",           "%d",  0, &XMIN);
  ScanConfig (config, "ADDSTAR_XMAX",           "%d",  0, &XMAX);
  ScanConfig (config, "ADDSTAR_YMIN",           "%d",  0, &YMIN);
  ScanConfig (config, "ADDSTAR_YMAX",           "%d",  0, &YMAX);

  /* exclude stars with SN > SNLIMIT (ADDSTAR_SNLIMIT overrides old name MIN_SN_FSTAT) */
  SNLIMIT = 0;
  ScanConfig (config, "MIN_SN_FSTAT",           "%lf", 0, &SNLIMIT);
  ScanConfig (config, "ADDSTAR_SNLIMIT",        "%lf", 0, &SNLIMIT);

  /* exclude stars with bits that match the given photFlags bits */
  PHOTFLAG_EXCLUDE = 0;
  ScanConfig (config, "ADDSTAR_PHOTFLAG_EXCLUDE", "%d", 0, &PHOTFLAG_EXCLUDE);

  MAX_CERROR = 0.5; // arcseconds
  ScanConfig (config, "ADDSTAR_MAX_CERROR",     "%lf", 0, &MAX_CERROR);

  MIN_FWHM_X = 0.0; // arcseconds
  ScanConfig (config, "ADDSTAR_MIN_FWHM_X",     "%lf", 0, &MIN_FWHM_X);
  MIN_FWHM_Y = 0.0; // arcseconds
  ScanConfig (config, "ADDSTAR_MIN_FWHM_Y",     "%lf", 0, &MIN_FWHM_Y);

  /* used by parse_time to find time-related keywords */
  strcpy (DateKeyword, "NONE");
  strcpy (DateMode, "NONE");
  strcpy (UTKeyword, "NONE");
  strcpy (JDKeyword, "NONE");
  strcpy (MJDKeyword, "NONE");
  ScanConfig (config, "DATE-KEYWORD",           "%s",  0, DateKeyword);
  ScanConfig (config, "DATE-MODE",              "%s",  0, DateMode);
  ScanConfig (config, "UT-KEYWORD",             "%s",  0, UTKeyword);
  ScanConfig (config, "MJD-KEYWORD",            "%s",  0, MJDKeyword);
  ScanConfig (config, "JD-KEYWORD",             "%s",  0, JDKeyword);

  ScanConfig (config, "EXPTIME-KEYWORD",        "%s",  0, ExptimeKeyword);
  ScanConfig (config, "AIRMASS-KEYWORD",        "%s",  0, AirmassKeyword);
  ScanConfig (config, "CCDNUM-KEYWORD",         "%s",  0, CCDNumKeyword);
  ScanConfig (config, "ST-KEYWORD",             "%s",  0, STKeyword);

  ScanConfig (config, "OBSERVATORY-LATITUDE",   "%s",  0, tmpword);
  if (!strcasecmp(tmpword, "NONE")) {
      fprintf (stderr, "observatory latitude is not set\n");
      Latitude = NAN;
  } else {
      ScanConfig (config, "OBSERVATORY-LATITUDE",   "%lf", 0, &Latitude);
  }
  ScanConfig (config, "OBSERVATORY-LONGITUDE",   "%s",  0, tmpword);
  if (!strcasecmp(tmpword, "NONE")) {
      fprintf (stderr, "observatory longitude is not set\n");
      Longitude = NAN;
  } else {
      ScanConfig (config, "OBSERVATORY-LONGITUDE",  "%lf", 0, &Longitude);
  }
  fprintf (stderr, "observatory @ (%f,%f)\n", Longitude, Latitude);

  if (!strcasecmp(STKeyword, "NONE")) {
      if (isnan(Longitude)) { 
	  fprintf (stderr, "WARNING: ST cannot be determined for this image (no ST Keyword, no longitude)\n");
      } else {
	  fprintf (stderr, "ST Keyword is not defined, ST will be derived from time & longitude\n");
      }
  }

  ScanConfig (config, "SUBPIX_DATAFILE",        "%s",  0, SubpixDatafile);

  ScanConfig (config, "IMAGE-ID-KEYWORD",       "%s",  0, ImageIDKeyword);
  ScanConfig (config, "SOURCE-ID-KEYWORD",      "%s",  0, SourceIDKeyword);

  if (!ScanConfig (config, "EXTNAME-KEYWORD",        "%s",  0, ExtnameKeyword)) {
      strcpy (ExtnameKeyword, "EXTNAME"); 
  }

  // if this config variable is not set, do not set any zero point offset
  if (!ScanConfig (config, "ZERO_POINT_OPTION", "%s",  0, ZERO_POINT_OPTION)) {
      strcpy (ZERO_POINT_OPTION, "NOMINAL"); 
  }
  // if this config variable is not set, do not set any zero point offset
  if (!ScanConfig (config, "ZERO_POINT_KEYWORD", "%s",  0, ZERO_POINT_KEYWORD)) {
      strcpy (ZERO_POINT_KEYWORD, "ZPT_OBS"); 
  }

  /* instrumental magnitude range for calibration mode */
  CAL_INSTMAG_MAX =  -9.0;
  CAL_INSTMAG_MIN = -13.0;
  ScanConfig (config, "CAL_INSTMAG_MAX",        "%lf", 0, &CAL_INSTMAG_MAX);
  ScanConfig (config, "CAL_INSTMAG_MIN",        "%lf", 0, &CAL_INSTMAG_MIN);

  /* location of needed data sources */
  ScanConfig (config, "2MASS_DIR_AS",           "%s",  0, TWO_MASS_DIR_AS);
  ScanConfig (config, "2MASS_DIR_DR2",          "%s",  0, TWO_MASS_DIR_DR2);
  ScanConfig (config, "GSCDIR",                 "%s",  0, GSCDIR);

  if (!ScanConfig (config, "USNO_A_DIR",        "%s",  0, USNO_A_DIR)) {
    ScanConfig (config, "USNO_CDROM",           "%s",  0, USNO_A_DIR);
  }
  ScanConfig (config, "USNO_B_DIR",             "%s",  0, USNO_B_DIR);

  ScanConfig (config, "TYCHO_DIR",             	"%s",  0, TYCHO_DIR);

  // force CATDIR to be absolute (so parallel mode will work)
  char tmpcatdir[DVO_MAX_PATH];
  GetConfig (config, "CATDIR" ,                 "%s",  0, tmpcatdir);
  CATDIR = abspath (tmpcatdir, DVO_MAX_PATH);

  GetConfig (config, "GSCFILE",                	"%s",  0, GSCFILE);
  GetConfig (config, "PHOTCODE_FILE",          	"%s",  0, MasterPhotcodeFile);
  ScanConfig (config, "CATMODE",                "%s",  0, CATMODE);
  ScanConfig (config, "CATFORMAT",              "%s",  0, CATFORMAT);
  if (!ScanConfig (config, "CATCOMPRESS",       "%s",  0, CATCOMPRESS)) {
    strcpy (CATCOMPRESS, "NONE");
  }
  if (!ScanConfig (config, "SKY_DEPTH",         "%d",  0, &SKY_DEPTH)) {
    SKY_DEPTH = SKY_DEPTH_HST;
  }
  if (!ScanConfig (config, "SKY_TABLE",         "%s",  0, SKY_TABLE)) {
    SKY_TABLE[0] = 0;
  }
  GetConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  SetZeroPoint (ZERO_POINT);

  if (!ScanConfig (config, "IMAGE_TABLE",       "%s",  0, ImageCat)) {
    if (!ScanConfig (config, "IMAGE_CATALOG",   "%s",  0, ImageCat)) {
      sprintf (ImageCat, "%s/Images.dat", CATDIR);
    }
  }

  ScanConfig (config, "CAMERA_LAYOUT",          "%s",  0, CameraLayout);

  /* used by client/server setup */
  ScanConfig (config, "PASSWORD",               "%s",  0, PASSWORD);
  ScanConfig (config, "HOSTNAME",               "%s",  0, HOSTNAME);
  
  /* load valid ip list */
  {
    int i, Nvalid_IP, ip1, ip2, ip3, ip4, test, status;
    char string[80];

    Nvalid_IP = 0;
    NVALID_IP = 10;
    ALLOCATE (VALID_IP, int, NVALID_IP);
    for (i = 0; ScanConfig (config, "VALID_IP", "%s", i, string) != NULL; i++) {
      status = sscanf (string, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
      test = TRUE;
      test &= (status == 4);
      test &= ((ip1 > 0) && (ip1 < 256)); 
      test &= ((ip2 > 0) && (ip2 < 256)); 
      test &= ((ip3 > 0) && (ip3 < 256)); 
      test &= ((ip4 >=0) && (ip4 < 256)); 
      if (!test) {
	fprintf (stderr, "invalid IP address %s\n", string);
	exit (2);
      }
      VALID_IP[Nvalid_IP] = ip1 | (ip2 << 8) | (ip3 << 16) | (ip4 << 24);
      Nvalid_IP ++;
      CHECK_REALLOCATE (VALID_IP, int, NVALID_IP, Nvalid_IP, 10);
    }
    NVALID_IP = Nvalid_IP;
    REALLOCATE (VALID_IP, int, NVALID_IP);
    if (NVALID_IP == 0) {
      free (VALID_IP);
      VALID_IP = NULL;
    }
  }

  /* set the default search radius */
  if (!ScanConfig (config, "ADDSTAR_RADIUS", "%s", 0, RadiusWord)) {
    GetConfig (config, "RADIUS", "%s", 0, RadiusWord);
  }
  /* XXX this does not work for refcat and reflist modes... */
  if (!strcasecmp (RadiusWord, "header")) {
    options.radius = 0;
    if (!ScanConfig (config, "ADDSTAR_NSIGMA", "%lf", 0, &options.Nsigma)) {
      GetConfig (config, "NSIGMA", "%lf", 0, &options.Nsigma);
    }
  } else {
    options.radius = atof (RadiusWord);
    if (options.radius < 1e-6) {
      fprintf (stderr, "non-sensical correlation radius %f\n", options.radius);
      exit (1);
    }
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

  /* get detection filtering mask */
  if (!ScanConfig (config, "DETECTIONFILTER", "%u", 0, &options.detectionFilter)) {
      options.detectionFilter = 0;
  }

  FreeConfigFile();
  free (config);
  free (file);
  return (options);
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
