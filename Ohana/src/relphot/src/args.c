# include "relphot.h"

RelphotMode args (int argc, char **argv) {

  int N;
  double trange;

  /* define time */
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTART)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      return (MODE_ERROR);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      if (!ohana_str_to_time (argv[N], &TSTOP)) { 
	fprintf (stderr, "ERROR: syntax error\n");
	return (MODE_ERROR);
      }
    } else {
      if (trange < 0) {
	trange = fabs (trange);
	TSTOP = TSTART;
	TSTART -= trange;
      } else {
	TSTOP = TSTART + trange;
      }
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
  }

  /* specify portion of the sky */
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

  /* specify region file by name (eg n0000/0000.00) */
  UserCatalog = NULL;
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserCatalog = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // If we are looking at the whole sky (or whole relevant sky), use the full image table
  // -- this save substantial memory.  this could be automatic if the skyregion covers
  // more than 2pi.
  USE_ALL_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-use-all-images"))) {
    remove_argument (N, &argc, argv);
    USE_ALL_IMAGES = TRUE;
  }

  USE_BASIC_CHECK = FALSE;
  if ((N = get_argument (argc, argv, "-basic-image-search"))) {
    remove_argument (N, &argc, argv);
    USE_BASIC_CHECK = TRUE;
  }

  IS_DIFF_DB = FALSE;
  if ((N = get_argument (argc, argv, "-is-diff-db"))) {
    remove_argument (N, &argc, argv);
    IS_DIFF_DB = TRUE;
  }

  USE_FULL_OVERLAP = TRUE;
  if ((N = get_argument (argc, argv, "-sloppy-image-overlap"))) {
    remove_argument (N, &argc, argv);
    USE_FULL_OVERLAP = FALSE;
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE2 = VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  VERBOSE_IMAGE = FALSE;
  if ((N = get_argument (argc, argv, "-vim"))) {
    VERBOSE_IMAGE = TRUE;
    remove_argument (N, &argc, argv);
  }

  NTHREADS = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    NTHREADS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  HOST_ID = 0;
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // relphot will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the relphot_client jobs remotely, they are 
  // run in serial via 'system'
  PARALLEL_SERIAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-serial"))) {
    if (PARALLEL_MANUAL) {
      fprintf (stderr, "ERROR: cannot mix -parallel-manual and -parallel-serial\n");
      exit (1);
    }
    PARALLEL = TRUE; // -parallel-serial implies -parallel
    PARALLEL_SERIAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_PARALLEL_GROUPS = 0;
  if ((N = get_argument (argc, argv, "-skip-parallel-groups"))) {
    remove_argument (N, &argc, argv);
    SKIP_PARALLEL_GROUPS = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // elements needed for parallel regions / parallel images
  MANUAL_UNIQUER = NULL;
  if ((N = get_argument (argc, argv, "-manual-uniquer"))) {
    remove_argument (N, &argc, argv);
    MANUAL_UNIQUER = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PLOTSTUFF = FALSE;
  if ((N = get_argument (argc, argv, "-plot"))) {
    PLOTSTUFF = TRUE;
    remove_argument (N, &argc, argv);
  }

  PLOTDELAY = 500000;
  if ((N = get_argument (argc, argv, "-plotdelay"))) {
    remove_argument (N, &argc, argv);
    PLOTDELAY = 1e6*atof(argv[N]);
    PLOTSTUFF = TRUE; // always turn on plotting if i request a plot delay
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-outroot"))) {
    remove_argument (N, &argc, argv);
    OUTROOT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else {
    OUTROOT = strcreate ("relphot");
  }      

  strcpy (STATMODE, "WT_MEAN");
  if ((N = get_argument (argc, argv, "-statmode"))) {
    remove_argument (N, &argc, argv);
    strcpy (STATMODE, argv[N]);
    remove_argument (N, &argc, argv);
  }

  NLOOP = 8;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    NLOOP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-nloop"))) {
    remove_argument (N, &argc, argv);
    NLOOP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MANUAL_ITERATION = FALSE;
  if ((N = get_argument (argc, argv, "-manual-iteration"))) {
    remove_argument (N, &argc, argv);
    MANUAL_ITERATION = TRUE;
  }

  TEST_IMAGE1 = -1;
  if ((N = get_argument (argc, argv, "-test-image1"))) {
    char *endptr;
    remove_argument (N, &argc, argv);
    TEST_IMAGE1 = strtol(argv[N], &endptr, 0);
    if (*endptr) relphot_usage (argc, argv); 
    remove_argument (N, &argc, argv);
  }
  TEST_IMAGE2 = -1;
  if ((N = get_argument (argc, argv, "-test-image2"))) {
    char *endptr;
    remove_argument (N, &argc, argv);
    TEST_IMAGE2 = strtol(argv[N], &endptr, 0);
    if (*endptr) relphot_usage (argc, argv); 
    remove_argument (N, &argc, argv);
  }

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    remove_argument (N, &argc, argv);
    RESET = TRUE;
  }
  RESET_ZEROPTS = FALSE;
  if ((N = get_argument (argc, argv, "-reset-zpts"))) {
    remove_argument (N, &argc, argv);
    RESET_ZEROPTS = TRUE;
  }
  RESET_FLATCORR = FALSE;
  if ((N = get_argument (argc, argv, "-reset-flat"))) {
    remove_argument (N, &argc, argv);
    RESET_FLATCORR = TRUE;
  }

  REPAIR_WARPS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-warps"))) {
    remove_argument (N, &argc, argv);
    REPAIR_WARPS = TRUE;
  }

  PRESERVE_PS1 = FALSE;
  if ((N = get_argument (argc, argv, "-preserve-ps1"))) {
    remove_argument (N, &argc, argv);
    PRESERVE_PS1 = TRUE;
  }
  REQUIRE_PSFFIT = FALSE;
  if ((N = get_argument (argc, argv, "-require-psffit"))) {
    remove_argument (N, &argc, argv);
    REQUIRE_PSFFIT = TRUE;
  }
  USE_APER_FOR_STARGAL = FALSE;
  if ((N = get_argument (argc, argv, "-use-aper-for-stargal"))) {
    remove_argument (N, &argc, argv);
    USE_APER_FOR_STARGAL = TRUE;
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-update-catformat"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  UPDATE_XRAD = FALSE;
  if ((N = get_argument (argc, argv, "-update-xrad-average"))) {
    remove_argument (N, &argc, argv);
    UPDATE_XRAD = TRUE;
  }

  SAVE_IMAGE_UPDATES = TRUE;
  if ((N = get_argument (argc, argv, "-skip-image-updates"))) {
    remove_argument (N, &argc, argv);
    SAVE_IMAGE_UPDATES = FALSE;
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  CLOUD_TOLERANCE = 0.02;
  if ((N = get_argument (argc, argv, "-cloud-limit"))) {
    remove_argument (N, &argc, argv);
    CLOUD_TOLERANCE = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX should we load a tree from CATDIR by default?
  // NOTE: a given catdir needs an appropriate boundary tree
  BOUNDARY_TREE = NULL;
  if ((N = get_argument (argc, argv, "-boundary-tree"))) {
    remove_argument (N, &argc, argv);
    BOUNDARY_TREE = strcreate(argv[N]);
    load_tess (BOUNDARY_TREE);
    remove_argument (N, &argc, argv);
  }

  SHOW_PARAMS = TRUE;
  if ((N = get_argument (argc, argv, "-params"))) {
    remove_argument (N, &argc, argv);
    SHOW_PARAMS = FALSE;
  }

  PlotMmin = 10.0; PlotMmax = 20.0; PlotdMmin = -1.0; PlotdMmax = 1.0;
  if ((N = get_argument (argc, argv, "-plrange"))) {
    remove_argument (N, &argc, argv);
    PlotMmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    PlotMmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    PlotdMmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    PlotdMmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // global restriction on fitting image zero points for mosaic cameras
  FREEZE_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-imfreeze"))) {
    remove_argument (N, &argc, argv);
    FREEZE_IMAGES = TRUE;
  }
  // global restriction on fitting mosaic zero points
  FREEZE_MOSAICS = FALSE;
  if ((N = get_argument (argc, argv, "-mosfreeze"))) {
    remove_argument (N, &argc, argv);
    FREEZE_MOSAICS = TRUE;
  }
  CALIBRATE_STACKS_AND_WARPS = FALSE;
  if ((N = get_argument (argc, argv, "-only-stacks-and-warps"))) {
    remove_argument (N, &argc, argv);
    CALIBRATE_STACKS_AND_WARPS = TRUE;
  }
  USE_MCAL_PSF_FOR_STACK_APER = FALSE;
  if ((N = get_argument (argc, argv, "-use-mcal-psf-for-stack-aper"))) {
    remove_argument (N, &argc, argv);
    USE_MCAL_PSF_FOR_STACK_APER = TRUE;
  }

  // default behavior is to fit all chips independently
  TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_NONE;
  MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
  IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_ALL;

  /* XXX this argument used to do two things: specify the camera name and tell the analysis to
     calculate a common zero point for a single mosaic.  I've moved the MOSAICNAME concept into the
     config system, but need to use this argument to specify that the mosaic zeropoints should be
     calculated. */
  MOSAIC_ZEROPT = FALSE;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    MOSAIC_ZEROPT = TRUE;
    if (!strcasecmp (MOSAICNAME, "none")) {
      fprintf (stderr, "mosaic astrometry selected by MOSAICNAME not defined\n");
      exit (2);
    }
    IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
    MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_ALL;
  }

  // calibrate the photometry by night / time period
  TGROUP_ZEROPT = FALSE;
  char *TGROUP_FILENAME = NULL;
  if ((N = get_argument (argc, argv, "-tgroup")) || (N = get_argument (argc, argv, "-tgroups"))) {
    TGROUP_ZEROPT = TRUE;
    remove_argument (N, &argc, argv);
    // if we fit for TGROUPS, start with the images and mosaics frozen
    TGROUP_ZPT_MODE = TGROUP_ZPT_MODE_ALL;
    IMAGE_ZPT_MODE  = IMAGE_ZPT_MODE_NONE;
    MOSAIC_ZPT_MODE = MOSAIC_ZPT_MODE_NONE;
  }
  if ((N = get_argument (argc, argv, "-tgroup-list"))) {
    if (!TGROUP_ZEROPT) {
      fprintf (stderr, "-tgroup-list (filename) requires -tgroup option\n");
      exit (2);
    }
    remove_argument (N, &argc, argv);
    TGROUP_FILENAME = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-tgroup")) || (N = get_argument (argc, argv, "-tgroups"))) {
    fprintf (stderr, "use only one of -tgroups and -tgroup (same meaning)\n");
    exit (2);
  }
  TGROUP_FIT_AIRMASS = FALSE;
  if ((N = get_argument (argc, argv, "-tgroup-fit-airmass"))) {
    if (!TGROUP_ZEROPT) {
      fprintf (stderr, "-tgroup-fit-airmass requires -tgroup (filename) option\n");
      exit (2);
    }
    TGROUP_FIT_AIRMASS = TRUE;
    remove_argument (N, &argc, argv);
  }

  // GRID_ZEROPT is not valid for all cases, probably should be its own mode...
  GRID_ZEROPT = FALSE;
  GRID_MEANFILE = NULL; // this is set by GridCorrectionSave()
  GRID_ZPT_MODE = GRID_ZPT_MODE_NONE; // start with grid off
  if ((N = get_argument (argc, argv, "-grid"))) {
    remove_argument (N, &argc, argv);
    GRID_ZEROPT = TRUE;
  }
  if ((N = get_argument (argc, argv, "-grid-meanfile"))) {
    GRID_ZEROPT = TRUE;
    remove_argument (N, &argc, argv);
    GRID_MEANFILE = strcreate (argv[N]); // used by relphot -averages and relphot -apply-offsets
    remove_argument (N, &argc, argv);
    GRID_ZPT_MODE  = GRID_ZPT_MODE_ALL; // since we are applying the grid, need to active this
  }

  GRID_BIN_GPC1 = 16;
  if ((N = get_argument (argc, argv, "-grid-bin-gpc1"))) {
    remove_argument (N, &argc, argv);
    GRID_BIN_GPC1 = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  GRID_BIN_GPC2 = 16;
  if ((N = get_argument (argc, argv, "-grid-bin-gpc2"))) {
    remove_argument (N, &argc, argv);
    GRID_BIN_GPC2 = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  GRID_BIN_HSC = 8;
  if ((N = get_argument (argc, argv, "-grid-bin-hsc"))) {
    remove_argument (N, &argc, argv);
    GRID_BIN_HSC = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  GRID_BIN_CFH = 8;
  if ((N = get_argument (argc, argv, "-grid-bin-cfh"))) {
    remove_argument (N, &argc, argv);
    GRID_BIN_CFH = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  KEEP_UBERCAL = TRUE;
  if ((N = get_argument (argc, argv, "-reset-ubercal"))) {
    remove_argument (N, &argc, argv);
    KEEP_UBERCAL = FALSE;
  }

  VARIABILITY_STATS = FALSE;
  if ((N = get_argument (argc, argv, "-varstats"))) {
    remove_argument (N, &argc, argv);
    VARIABILITY_STATS = TRUE;
  }
  USE_OLS_FOR_AVERAGES = FALSE;
  if ((N = get_argument (argc, argv, "-use-ols-for-averages"))) {
    remove_argument (N, &argc, argv);
    USE_OLS_FOR_AVERAGES = TRUE;
  }

  MIN_ERROR = 0.001;
  if ((N = get_argument (argc, argv, "-minerror"))) {
    remove_argument (N, &argc, argv);
    MIN_ERROR = atof (argv[N]);
    remove_argument (N, &argc, argv);
    /* require MIN_ERROR > 0 */
  }  

  AreaSelect = FALSE;
  if ((N = get_argument (argc, argv, "-area"))) {
    remove_argument (N, &argc, argv);
    AreaXmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaXmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaYmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaYmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaSelect = TRUE;
  }

  ImagSelect = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    remove_argument (N, &argc, argv);
    ImagMin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagMax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagSelect = TRUE;
  }

  DophotSelect = FALSE;
  if ((N = get_argument (argc, argv, "-dophot"))) {
    remove_argument (N, &argc, argv);
    DophotValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
    DophotSelect = TRUE;
  }

  // if -synthphot is chosen, an artificial w-band measurement is generated for each object 
  // with r and i photometry.  this was used to apply the ubercal photometry to w-band images.
  SyntheticPhotometry = FALSE;
  if ((N = get_argument (argc, argv, "-synthphot"))) {
    remove_argument (N, &argc, argv);
    SyntheticPhotometry = TRUE;
    init_synthetic_mags();
  }

  refPhotcode = NULL;
  if ((N = get_argument (argc, argv, "-refcode"))) {
    remove_argument (N, &argc, argv);
    refPhotcode = GetPhotcodebyName (argv[N]);
    if (!refPhotcode) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }
  USE_REFERENCE_WEIGHT = FALSE;
  if ((N = get_argument (argc, argv, "-ref-weight"))) {
    remove_argument (N, &argc, argv);
    USE_REFERENCE_WEIGHT = TRUE;
  }

  STAGES = STAGE_CHIP | STAGE_WARP | STAGE_STACK;
  if ((N = get_argument (argc, argv, "-skip-chip"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_CHIP;
  }
  if ((N = get_argument (argc, argv, "-skip-warp"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_WARP;
  }
  if ((N = get_argument (argc, argv, "-skip-stack"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_STACK;
  }
  if (!STAGES && NLOOP > 1) {
    fprintf (stderr, "WARNING: no averages requested (all stages skipped), but nloop > 1.  is this wasted effort?\n");
  }

  REGION_FILE = NULL;
  if ((N = get_argument (argc, argv, "-region-hosts"))) {
    remove_argument (N, &argc, argv);
    REGION_FILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  REGION_HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-region-hostID"))) {
    remove_argument (N, &argc, argv);
    REGION_HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  RelphotMode mode = MODE_ERROR;
  if ((N = get_argument (argc, argv, "-images"))) {
    remove_argument (N, &argc, argv);
    mode = UPDATE_IMAGES;
  }
  if ((N = get_argument (argc, argv, "-averages"))) {
    remove_argument (N, &argc, argv);
    mode = UPDATE_AVERAGES;
  }

  // if -synthphot-zpts is supplied, the map of corrections needed for the synthetic photometry is supplied
  // and used to tie down the synthetic magnitudes
  // NOTE: SYNTH_ZERO_POINTS is used for the same file in both -synthphot-zpts and -synthphot_means
  SYNTH_ZERO_POINTS = NULL;
  if ((N = get_argument (argc, argv, "-synthphot-zpts"))) {
    remove_argument (N, &argc, argv);
    myAssert (N < argc, "missing argument to -synthphot-zpts");
    SYNTH_ZERO_POINTS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-synthphot_means"))) {
    mode = SYNTH_PHOT;
    remove_argument (N, &argc, argv);
    myAssert (N < argc, "missing argument to -synthphot_means");
    SYNTH_ZERO_POINTS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-apply-offsets"))) {
    remove_argument (N, &argc, argv);
    mode = APPLY_OFFSETS;
  }
  IMAGE_TABLE = NULL;
  if ((N = get_argument (argc, argv, "-parallel-images"))) {
    remove_argument (N, &argc, argv);
    mode = PARALLEL_IMAGES;
    if (N >= argc) relphot_usage(argc, argv);
    IMAGE_TABLE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    if (!REGION_FILE) relphot_usage(argc, argv);
  }

  PARALLEL_REGIONS_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-regions"))) {
    remove_argument (N, &argc, argv);
    mode = PARALLEL_REGIONS;
    if (!REGION_FILE) relphot_usage(argc, argv);
    if ((N = get_argument (argc, argv, "-parallel-regions-manual"))) {
      remove_argument (N, &argc, argv);
      PARALLEL_REGIONS_MANUAL = TRUE;
    }
  }

  // (photcodes, Nphotcodes) is the list of active average photcodes
  PhotcodeList = NULL;
  photcodes = NULL;
  switch (mode) {
    case SYNTH_PHOT:
    case UPDATE_AVERAGES:
      // note: initialize sets PhotcodeList to the full set of average photcodes
      // if the mode is UPDATE_AVERAGES
      // if (argc != 1) relphot_usage(argc, argv);
      // break;
      
      if (argc == 1) break; // if no photcodes are given, use them all?

    case UPDATE_IMAGES:
    case PARALLEL_IMAGES:
    case PARALLEL_REGIONS:
    case APPLY_OFFSETS:
      PhotcodeList = strcreate (argv[1]);
      photcodes = ParsePhotcodeList (PhotcodeList, &Nphotcodes, TRUE); // require SEC photcodes
      remove_argument (1, &argc, argv);
      break;

    default:
      fprintf (stderr, "no valid mode selected\n");
      relphot_usage(argc, argv);
      break;
  }
  if (argc != 1) relphot_usage (argc, argv);

  // if we supplied a file, we need to load it now
  if (TGROUP_FILENAME) {
    loadTGroups (TGROUP_FILENAME);
    free (TGROUP_FILENAME);
  }

  return mode;
}

// for the given measurement photcode, find the sequence number of
// the equivalent active photcode (-1 if none match)
// XXX this could be optimized by making a lookup table of active equiv codes
int GetActivePhotcodeIndex (int photcode) {

  // first find the matching photcode (our list of active secondary photcodes
  // does not necessarily match the list of all secondary photcodes)
  int Ns = -1;
  int ecode = GetPhotcodeEquivCodebyCode (photcode);
  for (int i = 0; (Ns < 0) && (i < Nphotcodes); i++) {
    if (photcodes[i][0].code != ecode) continue;
    Ns = i;
  }
  return Ns;
}

// XXX need to free sky
void relphot_free (SkyTable *sky, SkyList *skylist) {
  FREE (UserCatalog);
  FREE (OUTROOT);
  FREE (UPDATE_CATFORMAT);

  FREE (BOUNDARY_TREE);
  FREE (REGION_FILE);
  FREE (SYNTH_ZERO_POINTS);
  FREE (GRID_MEANFILE);
  FREE (IMAGE_TABLE);
  FREE (CATDIR);

  FREE (PhotcodeList);
  FREE (photcodes);
  
  free_tess();
  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  
  freeTGroups();
  free_error();

  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
}

int args_client (int argc, char **argv) {

  int N;
  double trange;

 // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;
  SKIP_PARALLEL_GROUPS = 0;

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) relphot_client_usage();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) relphot_client_usage();

  IMAGES = NULL; // used in -update mode
  BCATALOG = NULL; // used in -load mode
  MODE = MODE_NONE;
  if ((N = get_argument (argc, argv, "-load"))) {
    MODE = MODE_LOAD;
    remove_argument (N, &argc, argv);
    BCATALOG = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-update-catalogs"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot mix modes (-load, -update-catalogs, -update-objects)\n");
      relphot_client_usage();
    }
    MODE = MODE_UPDATE;
    remove_argument (N, &argc, argv);
    IMAGES = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-update-objects"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot mix modes (-load, -update-catalogs, -update-objects)\n");
      relphot_client_usage();
    }
    MODE = MODE_UPDATE_OBJECTS;
    remove_argument (N, &argc, argv);
  }
  SYNTH_ZERO_POINTS = NULL;
  if ((N = get_argument (argc, argv, "-synthphot-zpts"))) {
    remove_argument (N, &argc, argv);
    myAssert (N < argc, "missing argument to -synthphot-zpts");
    SYNTH_ZERO_POINTS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-synthphot_means"))) {
    MODE = MODE_SYNTH_PHOT;
    remove_argument (N, &argc, argv);
    SYNTH_ZERO_POINTS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!MODE) relphot_client_usage();

  strcpy (STATMODE, "WT_MEAN");
  if ((N = get_argument (argc, argv, "-statmode"))) {
    remove_argument (N, &argc, argv);
    strcpy (STATMODE, argv[N]);
    remove_argument (N, &argc, argv);
  }

  BOUNDARY_TREE = NULL;
  if ((N = get_argument (argc, argv, "-boundary-tree"))) {
    remove_argument (N, &argc, argv);
    BOUNDARY_TREE = strcreate(argv[N]);
    load_tess (BOUNDARY_TREE);
    remove_argument (N, &argc, argv);
  }

  USE_ALL_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-use-all-images"))) {
    remove_argument (N, &argc, argv);
    USE_ALL_IMAGES = TRUE;
  }

  USE_BASIC_CHECK = FALSE;
  if ((N = get_argument (argc, argv, "-basic-image-search"))) {
    remove_argument (N, &argc, argv);
    USE_BASIC_CHECK = TRUE;
  }

  /* specify portion of the sky */
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

  IS_DIFF_DB = FALSE;
  if ((N = get_argument (argc, argv, "-is-diff-db"))) {
    remove_argument (N, &argc, argv);
    IS_DIFF_DB = TRUE;
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE2 = VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  TEST_IMAGE1 = -1;
  TEST_IMAGE2 = -1;

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    remove_argument (N, &argc, argv);
    RESET = TRUE;
  }
  RESET_ZEROPTS = FALSE;
  if ((N = get_argument (argc, argv, "-reset-zpts"))) {
    remove_argument (N, &argc, argv);
    RESET_ZEROPTS = TRUE;
  }
  RESET_FLATCORR = FALSE;
  if ((N = get_argument (argc, argv, "-reset-flat"))) {
    remove_argument (N, &argc, argv);
    RESET_FLATCORR = TRUE;
  }

  PRESERVE_PS1 = FALSE;
  if ((N = get_argument (argc, argv, "-preserve-ps1"))) {
    remove_argument (N, &argc, argv);
    PRESERVE_PS1 = TRUE;
  }

  REPAIR_WARPS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-warps"))) {
    remove_argument (N, &argc, argv);
    REPAIR_WARPS = TRUE;
  }

  KEEP_UBERCAL = TRUE;
  if ((N = get_argument (argc, argv, "-reset-ubercal"))) {
    remove_argument (N, &argc, argv);
    KEEP_UBERCAL = FALSE;
  }

  USE_MCAL_PSF_FOR_STACK_APER = FALSE;
  if ((N = get_argument (argc, argv, "-use-mcal-psf-for-stack-aper"))) {
    remove_argument (N, &argc, argv);
    USE_MCAL_PSF_FOR_STACK_APER = TRUE;
  }

  /* define time */
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTART)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      if (!ohana_str_to_time (argv[N], &TSTOP)) { 
	fprintf (stderr, "ERROR: syntax error\n");
	return (FALSE);
      }
    } else {
      if (trange < 0) {
	trange = fabs (trange);
	TSTOP = TSTART;
	TSTART -= trange;
      } else {
	TSTOP = TSTART + trange;
      }
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
  }


  VARIABILITY_STATS = FALSE;
  if ((N = get_argument (argc, argv, "-varstats"))) {
    remove_argument (N, &argc, argv);
    VARIABILITY_STATS = TRUE;
  }
  USE_OLS_FOR_AVERAGES = FALSE;
  if ((N = get_argument (argc, argv, "-use-ols-for-averages"))) {
    remove_argument (N, &argc, argv);
    USE_OLS_FOR_AVERAGES = TRUE;
  }

  MIN_ERROR = 0.001;
  if ((N = get_argument (argc, argv, "-minerror"))) {
    remove_argument (N, &argc, argv);
    MIN_ERROR = atof (argv[N]);
    remove_argument (N, &argc, argv);
    /* require MIN_ERROR > 0 */
  }  

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-update-catformat"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  AreaSelect = FALSE;
  if ((N = get_argument (argc, argv, "-area"))) {
    remove_argument (N, &argc, argv);
    AreaXmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaXmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaYmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaYmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    AreaSelect = TRUE;
  }

  SyntheticPhotometry = FALSE;
  if ((N = get_argument (argc, argv, "-synthphot"))) {
    remove_argument (N, &argc, argv);
    SyntheticPhotometry = FALSE;
    init_synthetic_mags();
  }

  // load the GridCorrection file
  GRID_ZEROPT = FALSE;
  GRID_MEANFILE = NULL;
  if ((N = get_argument (argc, argv, "-grid"))) {
    GRID_ZEROPT = TRUE;
    remove_argument (N, &argc, argv);
    GRID_MEANFILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    GRID_ZPT_MODE  = GRID_ZPT_MODE_ALL; // since we are applying the grid, need to active this
  }

  ImagSelect = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    remove_argument (N, &argc, argv);
    ImagMin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagMax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagSelect = TRUE;
  }

  DophotSelect = FALSE;
  if ((N = get_argument (argc, argv, "-dophot"))) {
    remove_argument (N, &argc, argv);
    DophotValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
    DophotSelect = TRUE;
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  STAGES = STAGE_CHIP | STAGE_WARP | STAGE_STACK;
  if ((N = get_argument (argc, argv, "-skip-chip"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_CHIP;
  }
  if ((N = get_argument (argc, argv, "-skip-warp"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_WARP;
  }
  if ((N = get_argument (argc, argv, "-skip-stack"))) {
    remove_argument (N, &argc, argv);
    STAGES &= ~STAGE_STACK;
  }
  if (!STAGES) {
    fprintf (stderr, "ERROR: no valid stages selected\n");
    exit (3);
  }

  if ((MODE == MODE_SYNTH_PHOT)  && (argc == 1)) return TRUE;
  if ((MODE == MODE_UPDATE_OBJECTS)  && (argc == 1)) return TRUE;
  if (argc != 2) relphot_client_usage ();

  return TRUE;
}

// XXX need to free sky
void relphot_client_free (SkyTable *sky, SkyList *skylist) {
  FREE (HOSTDIR);
  FREE (BCATALOG);
  FREE (IMAGES);

  FREE (SYNTH_ZERO_POINTS);
  FREE (GRID_MEANFILE);
  FREE (BOUNDARY_TREE);
  FREE (UPDATE_CATFORMAT);

  FREE (CATDIR);
  FREE (PhotcodeList);
  FREE (photcodes);
  
  free_tess();
  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  
  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
}


