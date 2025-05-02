# include "setphot.h"

void initialize_setphot_client (int argc, char **argv) {

  struct stat statbuffer;

  char CatdirPhotcodeFile[256];

  SetZeroPoint (25.0); // XXX is this needed?
  args_setphot_client (argc, argv);

  if (stat (CATDIR, &statbuffer)) {
    fprintf (stderr, "error accessing dvo database directory '%s'\n", CATDIR);
    exit (1);
  }

  // load the photcode table : XXX needed?
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
}

int args_setphot_client (int argc, char **argv) {

  int N;

  KH_FILE = NULL;
  if ((N = get_argument (argc, argv, "-KH"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    KH_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  KH_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-KH-reset"))) {
    remove_argument (N, &argc, argv);
    KH_RESET = TRUE;
  }

  DCR_FILE = NULL;
  if ((N = get_argument (argc, argv, "-DCR"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    DCR_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  DCR_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-DCR-reset"))) {
    remove_argument (N, &argc, argv);
    DCR_RESET = TRUE;
  }

  CAM_ASTROM_FILE = NULL;
  if ((N = get_argument (argc, argv, "-CAM"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    CAM_ASTROM_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  CAM_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-CAM-reset"))) {
    remove_argument (N, &argc, argv);
    CAM_RESET = TRUE;
  }

  // NOTE: I do not need to pass the cam-flat file to the 
  // setphot clients because the database gets an update to 
  // CATDIR/flatfield.fits which is loaded by setphot_client
  CAM_PHOTOM_FILE = NULL;

  SET_GAL_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-setgalmodel"))) {
    remove_argument (N, &argc, argv);
    SET_GAL_MODEL = strcreate (argv[N]);
    remove_argument (N, &argc, argv);

    if (!InitGalaxyModel (SET_GAL_MODEL)) {
      fprintf (stderr, "failed to init galaxy model %s\n", SET_GAL_MODEL);
      fprintf (stderr, "valid models: ROESER, FEAST-HIPPARCOS\n");
      exit (2);
    }
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    RESET = TRUE;
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_MIN = 0;
  PHOTCODE_MAX = 0;
  if ((N = get_argument (argc, argv, "-photcode-range"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_MIN = atoi (argv[N]);    
    remove_argument (N, &argc, argv);
    PHOTCODE_MAX = atoi (argv[N]);    
    remove_argument (N, &argc, argv);
  }

  // region of interest
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  REPAIR_BY_OBJID = FALSE;
  if ((N = get_argument (argc, argv, "-repair-by-objid"))) {
    REPAIR_BY_OBJID = TRUE;
    remove_argument (N, &argc, argv);
  }

  UBERCAL = FALSE;
  if ((N = get_argument (argc, argv, "-ubercal"))) {
    remove_argument (N, &argc, argv);
    UBERCAL = TRUE;
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) goto usage;

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) goto usage;

  if ((N = get_argument (argc, argv, "-catdir"))) {
    remove_argument (N, &argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!CATDIR) goto usage;

  IMAGES = NULL;
  if ((N = get_argument (argc, argv, "-images"))) {
    remove_argument (N, &argc, argv);
    IMAGES = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!IMAGES) goto usage;

  if (argc != 1) goto usage;
  return (TRUE);

usage:
  fprintf (stderr, "USAGE: setphot_client -hostID (hostID) -catdir (catdir) -hostdir (hostdir) -images (images) [options]\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -ubercal : interpret zpt file as a GPC1 ubercal FITS table\n");
  fprintf (stderr, "    -update : actually write results to detections tables\n");
  exit (2);
}

