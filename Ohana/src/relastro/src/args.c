# include "relastro.h"
void usage (int argc, char **argv);
void usage_client (int argc, char **argv);
void usage_merge_source (void);
void usage_merge_source_id (char *name);
float *ParseLoopWeights (char *rawlist);
int *ParseLoopOrder (char *rawlist, int minValue);

int args (int argc, char **argv) {

  int N;
  double trange;
  char *endptr;

  /* possible operations */
  FIT_TARGET = TARGET_NONE;
  RELASTRO_OP = OP_NONE;
  FIT_MODE = FIT_AVERAGE;

  OBJ_ID_SRC = OBJ_ID_DST = 0;
  CAT_ID_SRC = CAT_ID_DST = 0;

  if ((N = get_argument (argc, argv, "-merge-source"))) {
    if (N > argc - 6) usage_merge_source();
    if (strcmp(argv[N+3], "into")) usage_merge_source();
    RELASTRO_OP = OP_MERGE_SOURCE;
    remove_argument (N, &argc, argv);
    OBJ_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_merge_source_id (argv[N]); 
    remove_argument (N, &argc, argv);
    CAT_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_merge_source_id (argv[N]); 
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv); // remove the 'into'
    OBJ_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_merge_source_id (argv[N]); 
    remove_argument (N, &argc, argv);
    CAT_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_merge_source_id (argv[N]); 
    remove_argument (N, &argc, argv);

    if (argc != 1) usage (argc, argv);
    return TRUE;
  }

  if ((N = get_argument (argc, argv, "-update-objects"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_UPDATE_OBJECTS;
  }

  if ((N = get_argument (argc, argv, "-update-offsets"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_UPDATE_OFFSETS;
  }

  if ((N = get_argument (argc, argv, "-repair-stacks"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_STACKS;
  }
  if ((N = get_argument (argc, argv, "-repair-object-id"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_OBJECT_ID;
  }

  if ((N = get_argument (argc, argv, "-repair-warps"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_WARPS;
  }

  // the default is to only measure the scatter for the stacks, not to perform a new fit
  FIT_STACKS = FALSE;
  if ((N = get_argument (argc, argv, "-fit-stacks"))) {
    remove_argument (N, &argc, argv);
    FIT_STACKS = TRUE;
  }

  REPAIR_STACKS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-stacks-on-update"))) {
    remove_argument (N, &argc, argv);
    REPAIR_STACKS = TRUE;
  }
  CHECK_MEASURE_TO_IMAGE = FALSE;
  if ((N = get_argument (argc, argv, "-check-measures"))) {
    remove_argument (N, &argc, argv);
    CHECK_MEASURE_TO_IMAGE = TRUE;
  }

  // catch-up mode : for a re-run, allow the sync file to be ahead of the desired location:
  CATCH_UP = FALSE;
  if ((N = get_argument (argc, argv, "-catch-up"))) {
    remove_argument (N, &argc, argv);
    CATCH_UP = TRUE;
  }

  // only update the fit statistics (systematic floor)
  IMSTATS_ONLY = FALSE;
  if ((N = get_argument (argc, argv, "-imstats-only"))) {
    remove_argument (N, &argc, argv);
    IMSTATS_ONLY = TRUE;
  }

  // elements needed for parallel regions / parallel images
  MANUAL_UNIQUER = NULL;
  if ((N = get_argument (argc, argv, "-manual-uniquer"))) {
    remove_argument (N, &argc, argv);
    MANUAL_UNIQUER = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // elements needed for parallel regions / parallel images
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

  IMAGE_TABLE = NULL;
  if ((N = get_argument (argc, argv, "-parallel-images"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_PARALLEL_IMAGES;
    if (N >= argc) usage (argc, argv);
    IMAGE_TABLE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    if (!REGION_FILE) usage (argc, argv);
  }

  if ((N = get_argument (argc, argv, "-images"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_IMAGES;
  }

  // used to decide if changes to the image parameters get applied to the measures
  APPLY_OFFSETS = FALSE;
  if ((N = get_argument (argc, argv, "-apply-offsets"))) {
    remove_argument (N, &argc, argv);
    APPLY_OFFSETS = TRUE;
  }

  APPLY_PROPER_MOTION = FALSE;
  if ((N = get_argument (argc, argv, "-apply-proper-motion"))) {
    remove_argument (N, &argc, argv);
    APPLY_PROPER_MOTION = TRUE;
  }

  SKIP_PS1_CHIP = FALSE;
  if ((N = get_argument (argc, argv, "-skip-ps1-chip"))) {
    remove_argument (N, &argc, argv);
    SKIP_PS1_CHIP = TRUE;
  }
  SKIP_PS1_STACK = FALSE;
  if ((N = get_argument (argc, argv, "-skip-ps1-stack"))) {
    remove_argument (N, &argc, argv);
    SKIP_PS1_STACK = TRUE;
  }
  SKIP_HSC = FALSE;
  if ((N = get_argument (argc, argv, "-skip-hsc"))) {
    remove_argument (N, &argc, argv);
    SKIP_HSC = TRUE;
  }
  SKIP_CFH = FALSE;
  if ((N = get_argument (argc, argv, "-skip-cfh"))) {
    remove_argument (N, &argc, argv);
    SKIP_CFH = TRUE;
  }

  UPDATE_ALL_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-all-cameras"))) {
    remove_argument (N, &argc, argv);
    UPDATE_ALL_MEASURE = TRUE;
  }
  UPDATE_PS1_STACK_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-ps1-stack"))) {
    remove_argument (N, &argc, argv);
    UPDATE_PS1_STACK_MEASURE = TRUE;
  }
  UPDATE_PS1_CHIP_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-ps1-chip"))) {
    remove_argument (N, &argc, argv);
    UPDATE_PS1_CHIP_MEASURE = TRUE;
  }
  UPDATE_HSC_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-hsc"))) {
    remove_argument (N, &argc, argv);
    UPDATE_HSC_MEASURE = TRUE;
  }
  UPDATE_CFH_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-cfh"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CFH_MEASURE = TRUE;
  }
  if (RELASTRO_OP == OP_UPDATE_OFFSETS) {
    if (!UPDATE_ALL_MEASURE && !UPDATE_PS1_STACK_MEASURE && !UPDATE_PS1_CHIP_MEASURE && !UPDATE_HSC_MEASURE && !UPDATE_CFH_MEASURE) {
      fprintf (stderr, "for -update-offsets, need to select at least one of -update-all-cameras, -update-ps1-stack, -update-ps1-chip, -update-hsc, -update-cfh\n");
      exit (2);
    }
  }
  if (RELASTRO_OP == OP_UPDATE_OBJECTS) {
    if (!UPDATE_ALL_MEASURE && !UPDATE_PS1_STACK_MEASURE && !UPDATE_PS1_CHIP_MEASURE && !UPDATE_HSC_MEASURE && !UPDATE_CFH_MEASURE) {
      fprintf (stderr, "for -update-objects, need to select at least one of -update-all-cameras, -update-ps1-stack, -update-ps1-chip, -update-hsc, -update-cfh\n");
      exit (2);
    }
  }
  if ((RELASTRO_OP == OP_PARALLEL_REGIONS) || (RELASTRO_OP == OP_PARALLEL_IMAGES) || (RELASTRO_OP == OP_IMAGES)) {
    if (APPLY_OFFSETS && !UPDATE_ALL_MEASURE && !UPDATE_PS1_STACK_MEASURE && !UPDATE_PS1_CHIP_MEASURE && !UPDATE_HSC_MEASURE && !UPDATE_CFH_MEASURE) {
      fprintf (stderr, "for [-images or -parallel-images] with -apply-offsets, need to select at least one of -update-ps1-stack, -update-ps1-chip, -update-hsc, -update-cfh\n");
      exit (2);
    }
  }

  // for fitting objects, this is always the same as 'ALLOW_IRLS' below, but for fitting
  // images, this is set to FALSE while doing the fit for the image parameters
  USE_IRLS = TRUE;  
  ALLOW_IRLS = TRUE;
  if ((N = get_argument (argc, argv, "-no-irls"))) {
    remove_argument (N, &argc, argv);
    USE_IRLS = FALSE;
    ALLOW_IRLS = FALSE;
  }

  PARALLEL_REGIONS_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-regions"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_PARALLEL_REGIONS;
    if (!REGION_FILE) usage (argc, argv);
    if ((N = get_argument (argc, argv, "-parallel-regions-manual"))) {
      remove_argument (N, &argc, argv);
      PARALLEL_REGIONS_MANUAL = TRUE;
    }
  }

  if ((N = get_argument (argc, argv, "-testobj1"))) {
    if (N > argc - 3) usage (argc, argv);
    remove_argument (N, &argc, argv);
    OBJ_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage (argc, argv);
    remove_argument (N, &argc, argv);
    CAT_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage (argc, argv); 
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-testobj2"))) {
    if (N > argc - 3) usage (argc, argv);
    remove_argument (N, &argc, argv);
    OBJ_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage (argc, argv);
    remove_argument (N, &argc, argv);
    CAT_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage (argc, argv); 
    remove_argument (N, &argc, argv);
  }

  // check for object fitting modes
  if ((N = get_argument (argc, argv, "-pm"))) {
    remove_argument (N, &argc, argv);
    FIT_MODE = FIT_PM_ONLY;
  }
  if ((N = get_argument (argc, argv, "-pmpar"))) {
    remove_argument (N, &argc, argv);
    FIT_MODE = FIT_PM_AND_PAR;
  }

  // NOTE : this is no longer a valid mode
  // if ((N = get_argument (argc, argv, "-par"))) {
  //   remove_argument (N, &argc, argv);
  //   FIT_MODE = FIT_PAR_ONLY;
  // }

  if ((N = get_argument (argc, argv, "-high-speed"))) {
    // XXX include a parallax / no-parallax option
    if (N >= argc - 4) usage (argc, argv);
    RELASTRO_OP = OP_HIGH_SPEED;
    remove_argument (N, &argc, argv);
    PHOTCODE_A_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    PHOTCODE_B_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    RADIUS = atof(argv[N]);
    remove_argument (N, &argc, argv);
    HIGH_SPEED_DIR = abspath(argv[N], DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-hpm"))) {
    if (N >= argc - 2) usage (argc, argv);
    RELASTRO_OP = OP_HPM;
    remove_argument (N, &argc, argv);
    RADIUS = atof(argv[N]);
    remove_argument (N, &argc, argv);
    HIGH_SPEED_DIR = abspath(argv[N], DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }

  PARALLEL_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-output"))) {
    remove_argument (N, &argc, argv);
    PARALLEL_OUTPUT = TRUE;
    if ((RELASTRO_OP != OP_HIGH_SPEED) && (RELASTRO_OP != OP_HPM)) {
      fprintf (stderr, "-parallel-output only valid for -high-speed or -hpm modes\n");
      exit (1);
    }
  }

  if ((N = get_argument (argc, argv, "-update-simple"))) {
    remove_argument (N, &argc, argv);
    FIT_TARGET = TARGET_SIMPLE;
  }
  if ((N = get_argument (argc, argv, "-update-chips"))) {
    remove_argument (N, &argc, argv);
    FIT_TARGET = TARGET_CHIPS;
  }
  if ((N = get_argument (argc, argv, "-update-mosaics"))) {
    remove_argument (N, &argc, argv);
    FIT_TARGET = TARGET_MOSAICS;
  }
  if ((N = get_argument (argc, argv, "-set-chips"))) {
    remove_argument (N, &argc, argv);
    FIT_TARGET = SET_CHIPS;
  }
  if ((N = get_argument (argc, argv, "-set-stacks"))) {
    remove_argument (N, &argc, argv);
    FIT_TARGET = SET_STACKS;
  }

  FlagOutlier = FALSE;
  if ((N = get_argument (argc, argv, "-clip"))) {
    fprintf (stderr, "-clip is currently disabled\n");
    remove_argument (N, &argc, argv);
    CLIP_THRESH = atof (argv[N]);
    remove_argument (N, &argc, argv);
    FlagOutlier = TRUE;
  }

  ExcludeBogus = FALSE;
  ExcludeBogusRadius = 0.0;
  if ((N = get_argument (argc, argv, "-exclude-bogus"))) {
    remove_argument (N, &argc, argv);
    ExcludeBogusRadius = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ExcludeBogus = TRUE;
  }

  if (RELASTRO_OP == OP_NONE) usage (argc, argv);

  if (((RELASTRO_OP == OP_IMAGES) || (RELASTRO_OP == OP_PARALLEL_REGIONS) || (RELASTRO_OP == OP_PARALLEL_IMAGES)) && (FIT_TARGET == TARGET_NONE)) usage (argc, argv);

  /* specify portion of the sky : allow default of all sky? */
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
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // relastro will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL_NO_WAIT = FALSE;
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
    if ((N = get_argument (argc, argv, "-parallel-manual-nowait"))) {
      PARALLEL_MANUAL_NO_WAIT = TRUE;
      remove_argument (N, &argc, argv);
    }
  }
  // this is a test mode : rather than launching the relastro_client jobs remotely, they are 
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

  // If we are looking at the whole sky (or whole relevant sky), use the full image table
  // -- this save substantial memory.  this could be automatic if the skyregion covers
  // more than 2pi.
  USE_ALL_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-use-all-images"))) {
    remove_argument (N, &argc, argv);
    USE_ALL_IMAGES = TRUE;
  }

  KEEP_ALL_IMAGES_RA = FALSE;
  if ((N = get_argument (argc, argv, "-keep-all-images-ra"))) {
    remove_argument (N, &argc, argv);
    KEEP_ALL_IMAGES_RA = TRUE;
  }

  USE_BASIC_CHECK = FALSE;
  if ((N = get_argument (argc, argv, "-basic-image-search"))) {
    remove_argument (N, &argc, argv);
    USE_BASIC_CHECK = TRUE;
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  N_BOOTSTRAP_SAMPLES = 100;
  if ((N = get_argument (argc, argv, "-bootstrap-samples"))) {
    remove_argument (N, &argc, argv);
    N_BOOTSTRAP_SAMPLES = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    if ((N_BOOTSTRAP_SAMPLES > 0) && (N_BOOTSTRAP_SAMPLES < 20)) {
      fprintf (stderr, "-bootstrap-samples must be either 0 (no sampling) or >= 20\n");
      exit (2);
    }
  }

  // when repairing warp detections, check the model coordinates for consistency
  USE_IMAGE_COORDS_FOR_REPAIR = FALSE;
  if ((N = get_argument (argc, argv, "-use-image-coords-for-repair"))) {
    remove_argument (N, &argc, argv);
    USE_IMAGE_COORDS_FOR_REPAIR = TRUE;
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

  // eg g - r or r - i
  DCR_BLUE_COLOR_POS = NULL;
  DCR_BLUE_COLOR_NEG = NULL;
  if ((N = get_argument (argc, argv, "-dcr-blue-color"))) {
    remove_argument (N, &argc, argv);
    DCR_BLUE_COLOR_POS = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    DCR_BLUE_COLOR_NEG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // eg, z - y or i - z
  DCR_RED_COLOR_POS = NULL;
  DCR_RED_COLOR_NEG = NULL;
  if ((N = get_argument (argc, argv, "-dcr-red-color"))) {
    remove_argument (N, &argc, argv);
    DCR_RED_COLOR_POS = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    DCR_RED_COLOR_NEG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_RESET_LIST = NULL;
  if ((N = get_argument (argc, argv, "-reset-to-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_RESET_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "+photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *SuperCOSMOS_SKIP = strcreate("SCOS.103a.E,SCOS.4414.OG590,SCOS.4415.OG590,SCOS.IIIaF.OG590,SCOS.IIIaF.RG610,SCOS.IIIaF.RG630,SCOS.IIIaJ.GG385,SCOS.IIIaJ.GG395,SCOS.IVN.RG715,SCOS.IVN.RG9");

  PHOTCODE_SKIP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    char *RawSkip = strcreate(argv[N]);

    char *GotSkip = strstr (RawSkip, SuperCOSMOS_SKIP);
    if (!GotSkip) {
      int Ntotal = strlen(RawSkip) + strlen(SuperCOSMOS_SKIP) + 5;
      ALLOCATE (PHOTCODE_SKIP_LIST, char, Ntotal);
      snprintf (PHOTCODE_SKIP_LIST, Ntotal, "%s,%s", SuperCOSMOS_SKIP, RawSkip);
      free (RawSkip);
    } else {
      PHOTCODE_SKIP_LIST = RawSkip;
    }
    remove_argument (N, &argc, argv);
  }
  free (SuperCOSMOS_SKIP);

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-visual"))) {
    remove_argument (N, &argc, argv);
    relastroSetVisual(TRUE);
  }

  SAVEPLOT = FALSE;
  PLOTSTUFF = FALSE;
  if ((N = get_argument (argc, argv, "-plot"))) {
    PLOTSTUFF = TRUE;
    remove_argument (N, &argc, argv);
  }

  PLOTDELAY = 500000;
  if ((N = get_argument (argc, argv, "-plotdelay"))) {
    remove_argument (N, &argc, argv);
    PLOTDELAY = 1e6*atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // by default, require > 10pts to clip
  strcpy (STATMODE, "CHI_INNER_80_WTMEAN");
  if ((N = get_argument (argc, argv, "-statmode"))) {
    remove_argument (N, &argc, argv);
    strcpy (STATMODE, argv[N]);
    remove_argument (N, &argc, argv);
  }

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    remove_argument (N, &argc, argv);
    RESET = TRUE;
  }
  RESET_BAD_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-reset-bad-images"))) {
    remove_argument (N, &argc, argv);
    RESET_BAD_IMAGES = TRUE;
  }
  RESET_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-reset-images"))) {
    remove_argument (N, &argc, argv);
    RESET_IMAGES = TRUE;
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  SHOW_PARAMS = FALSE;
  if ((N = get_argument (argc, argv, "-params"))) {
    remove_argument (N, &argc, argv);
    SHOW_PARAMS = TRUE;
  }

  /* XXX update these for relevant plots */
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

  /* XXX update this */
  MIN_ERROR = 0.001;
  if ((N = get_argument (argc, argv, "-minerror"))) {
    remove_argument (N, &argc, argv);
    MIN_ERROR = atof (argv[N]);
    remove_argument (N, &argc, argv);
    /* require MIN_ERROR > 0 */
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
  
  // for now, make the default to ignore the photflags
  // XXX make it true by default instead?
  PhotFlagSelect = FALSE;
  if ((N = get_argument (argc, argv, "+photflags"))) {
    remove_argument (N, &argc, argv);
    PhotFlagSelect = TRUE;
  }

  PhotFlagBad = 0;
  if ((N = get_argument (argc, argv, "-photflagbad"))) {
    remove_argument (N, &argc, argv);
    PhotFlagBad = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PhotFlagPoor = 0;
  if ((N = get_argument (argc, argv, "-photflagpoor"))) {
    remove_argument (N, &argc, argv);
    PhotFlagPoor = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MinBadQF = 0.0;
  if ((N = get_argument (argc, argv, "-min-bad-psfqf"))) {
    remove_argument (N, &argc, argv);
    MinBadQF = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MaxMeanOffset = 10.0;
  if ((N = get_argument (argc, argv, "-max-mean-offset"))) {
    remove_argument (N, &argc, argv);
    MaxMeanOffset = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  NLOOP = 4;
  if ((N = get_argument (argc, argv, "-nloop"))) {
    remove_argument (N, &argc, argv);
    NLOOP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // e.g., -chiporderloop 3,4,5
  // NOTE: this must come after -nloop above
  ChipOrderLoop = NULL;
  ChipOrderLoopStr = NULL;
  CHIPORDER = 1;
  if ((N = get_argument (argc, argv, "-chiporder"))) {
    remove_argument (N, &argc, argv);
    CHIPORDER = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-chiporderloop"))) {
    remove_argument (N, &argc, argv);
    ChipOrderLoopStr = strcreate(argv[N]);
    ChipOrderLoop = ParseLoopOrder (argv[N], 1);
    remove_argument (N, &argc, argv);
  }

  ChipMapLoop = NULL;
  ChipMapLoopStr = NULL;
  CHIPMAP = 0;
  if ((N = get_argument (argc, argv, "-chipmap"))) {
    remove_argument (N, &argc, argv);
    CHIPMAP = atoi(argv[N]);
    remove_argument (N, &argc, argv);

  }
  if ((N = get_argument (argc, argv, "-chipmaploop"))) {
    remove_argument (N, &argc, argv);
    ChipMapLoopStr = strcreate(argv[N]);
    ChipMapLoop = ParseLoopOrder (argv[N], 0);
    remove_argument (N, &argc, argv);
  }
  

  // e.g., -loop-weights-2mass 1000,300,300,200,200,100
  // NOTE: this must come after -nloop above
  LoopWeight2MASS = NULL;
  LoopWeight2MASSstr = NULL;
  if ((N = get_argument (argc, argv, "-loop-weights-2mass"))) {
    remove_argument (N, &argc, argv);
    LoopWeight2MASSstr = strcreate(argv[N]);
    LoopWeight2MASS = ParseLoopWeights (argv[N]);
    remove_argument (N, &argc, argv);
  }
  LoopWeightTycho = NULL;
  LoopWeightTychostr = NULL;
  if ((N = get_argument (argc, argv, "-loop-weights-tycho"))) {
    remove_argument (N, &argc, argv);
    LoopWeightTychostr = strcreate(argv[N]);
    LoopWeightTycho = ParseLoopWeights (argv[N]);
    remove_argument (N, &argc, argv);
  }
  LoopWeightGAIA = NULL;
  LoopWeightGAIAstr = NULL;
  if ((N = get_argument (argc, argv, "-loop-weights-gaia"))) {
    remove_argument (N, &argc, argv);
    LoopWeightGAIAstr = strcreate(argv[N]);
    LoopWeightGAIA = ParseLoopWeights (argv[N]);
    remove_argument (N, &argc, argv);
  }

  GALAXY_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-galaxy-model"))) {
    remove_argument (N, &argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("FEAST-HIPPARCOS");

  // for testing, allow the galaxy model scale to be non-unity
  TEST_SCALE = 1.0;
  if ((N = get_argument (argc, argv, "-testing"))) {
    remove_argument (N, &argc, argv);
    TEST_SCALE = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  NTHREADS = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    NTHREADS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) usage (argc, argv);
  return TRUE;
}

void relastro_free (SkyTable *sky, SkyList *skylist) {
  FREE (REGION_FILE);
  FREE (IMAGE_TABLE);
  FREE (LoopWeight2MASSstr);
  FREE (LoopWeightTychostr);
  FREE (LoopWeightGAIAstr);
  FREE (LoopWeight2MASS);
  FREE (LoopWeightTycho);
  FREE (LoopWeightGAIA);

  FREE (ChipMapLoop);
  FREE (ChipMapLoopStr);
  FREE (ChipOrderLoop);
  FREE (ChipOrderLoopStr);

  FREE (PHOTCODE_SKIP_LIST);
  FREE (PHOTCODE_KEEP_LIST);
  FREE (PHOTCODE_RESET_LIST);
  FREE (DCR_RED_COLOR_POS);
  FREE (DCR_RED_COLOR_NEG);
  FREE (DCR_BLUE_COLOR_POS);
  FREE (DCR_BLUE_COLOR_NEG);

  FREE (PHOTCODE_A_LIST);
  FREE (PHOTCODE_B_LIST);
  FREE (HIGH_SPEED_DIR);
  FREE (BCATALOG);
  FREE (HOSTDIR);
  FREE (GALAXY_MODEL);

  // these are set in initialize
  FREE(photcodesKeep);
  FREE(photcodesSkip);  
  FREE(photcodesReset); 
  FREE(photcodesGroupA);
  FREE(photcodesGroupB);
  
  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  
  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
}

int args_client (int argc, char **argv) {

  int N;
  double trange;
  char *endptr;

  /* possible operations */
  RELASTRO_OP = TARGET_NONE;
  FIT_MODE = FIT_AVERAGE;

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_MANUAL_NO_WAIT = FALSE;
  PARALLEL_SERIAL = FALSE;

  BCATALOG = NULL;

  REGION_FILE = NULL;
  REGION_HOST_ID = 0;
  IMAGE_TABLE = NULL;
  PARALLEL_REGIONS_MANUAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage_client (argc, argv);

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_client (argc, argv);

  if ((N = get_argument (argc, argv, "-load-objects"))) {
    remove_argument (N, &argc, argv);
    BCATALOG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_LOAD_OBJECTS;
  }

  if ((N = get_argument (argc, argv, "-update-objects"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_UPDATE_OBJECTS;
  }

  if ((N = get_argument (argc, argv, "-update-offsets"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_UPDATE_OFFSETS;
  }

  if ((N = get_argument (argc, argv, "-repair-stacks"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_STACKS;
  }

  if ((N = get_argument (argc, argv, "-repair-object-id"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_OBJECT_ID;
  }

  if ((N = get_argument (argc, argv, "-repair-warps"))) {
    remove_argument (N, &argc, argv);
    RELASTRO_OP = OP_REPAIR_WARPS;
  }

  // the default is to only measure the scatter for the stacks, not to perform a new fit
  FIT_STACKS = FALSE;
  if ((N = get_argument (argc, argv, "-fit-stacks"))) {
    remove_argument (N, &argc, argv);
    FIT_STACKS = TRUE;
  }

  REPAIR_STACKS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-stacks-on-update"))) {
    remove_argument (N, &argc, argv);
    REPAIR_STACKS = TRUE;
  }
  CHECK_MEASURE_TO_IMAGE = FALSE;
  if ((N = get_argument (argc, argv, "-check-measures"))) {
    remove_argument (N, &argc, argv);
    CHECK_MEASURE_TO_IMAGE = TRUE;
  }

  // only update the fit statistics (systematic floor)
  IMSTATS_ONLY = FALSE;
  if ((N = get_argument (argc, argv, "-imstats-only"))) {
    remove_argument (N, &argc, argv);
    IMSTATS_ONLY = TRUE;
  }

  SKIP_PS1_CHIP = FALSE;
  if ((N = get_argument (argc, argv, "-skip-ps1-chip"))) {
    remove_argument (N, &argc, argv);
    SKIP_PS1_CHIP = TRUE;
  }
  SKIP_PS1_STACK = FALSE;
  if ((N = get_argument (argc, argv, "-skip-ps1-stack"))) {
    remove_argument (N, &argc, argv);
    SKIP_PS1_STACK = TRUE;
  }
  SKIP_HSC = FALSE;
  if ((N = get_argument (argc, argv, "-skip-hsc"))) {
    remove_argument (N, &argc, argv);
    SKIP_HSC = TRUE;
  }
  SKIP_CFH = FALSE;
  if ((N = get_argument (argc, argv, "-skip-cfh"))) {
    remove_argument (N, &argc, argv);
    SKIP_CFH = TRUE;
  }

  UPDATE_ALL_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-all-cameras"))) {
    remove_argument (N, &argc, argv);
    UPDATE_ALL_MEASURE = TRUE;
  }
  UPDATE_PS1_STACK_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-ps1-stack"))) {
    remove_argument (N, &argc, argv);
    UPDATE_PS1_STACK_MEASURE = TRUE;
  }
  UPDATE_PS1_CHIP_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-ps1-chip"))) {
    remove_argument (N, &argc, argv);
    UPDATE_PS1_CHIP_MEASURE = TRUE;
  }
  UPDATE_HSC_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-hsc"))) {
    remove_argument (N, &argc, argv);
    UPDATE_HSC_MEASURE = TRUE;
  }
  UPDATE_CFH_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-update-cfh"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CFH_MEASURE = TRUE;
  }
  if (RELASTRO_OP == OP_UPDATE_OFFSETS) {
    if (!UPDATE_ALL_MEASURE && !UPDATE_PS1_STACK_MEASURE && !UPDATE_PS1_CHIP_MEASURE && !UPDATE_HSC_MEASURE && !UPDATE_CFH_MEASURE) {
      fprintf (stderr, "for -update-offsets, need to select at least one of -update-ps1-stack, -update-ps1-chip, -update-hsc, -update-cfh\n");
      exit (2);
    }
  }

  // check for object fitting modes
  if ((N = get_argument (argc, argv, "-pm"))) {
    remove_argument (N, &argc, argv);
    FIT_MODE = FIT_PM_ONLY;
  }
  if ((N = get_argument (argc, argv, "-par"))) {
    remove_argument (N, &argc, argv);
    FIT_MODE = FIT_PAR_ONLY;
  }
  if ((N = get_argument (argc, argv, "-pmpar"))) {
    remove_argument (N, &argc, argv);
    FIT_MODE = FIT_PM_AND_PAR;
  }

  if ((N = get_argument (argc, argv, "-high-speed"))) {
    // XXX include a parallax / no-parallax option
    if (N >= argc - 5) usage_client (argc, argv);
    RELASTRO_OP = OP_HIGH_SPEED;
    remove_argument (N, &argc, argv);
    PHOTCODE_A_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    PHOTCODE_B_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    RADIUS = atof(argv[N]);
    remove_argument (N, &argc, argv);
    HIGH_SPEED_DIR = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-hpm"))) {
    if (N >= argc - 3) usage_client (argc, argv);
    RELASTRO_OP = OP_HPM;
    remove_argument (N, &argc, argv);
    RADIUS = atof(argv[N]);
    remove_argument (N, &argc, argv);
    HIGH_SPEED_DIR = abspath(argv[N], DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-testobj1"))) {
    if (N > argc - 3) usage_client (argc, argv);
    remove_argument (N, &argc, argv);
    OBJ_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_client (argc, argv);
    remove_argument (N, &argc, argv);
    CAT_ID_SRC = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_client (argc, argv); 
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-testobj2"))) {
    if (N > argc - 3) usage_client (argc, argv);
    remove_argument (N, &argc, argv);
    OBJ_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_client (argc, argv);
    remove_argument (N, &argc, argv);
    CAT_ID_DST = strtol(argv[N], &endptr, 0);
    if (*endptr) usage_client (argc, argv); 
    remove_argument (N, &argc, argv);
  }

  FlagOutlier = FALSE;
  if ((N = get_argument (argc, argv, "-clip"))) {
    fprintf (stderr, "-clip is currently disabled\n");
    remove_argument (N, &argc, argv);
    CLIP_THRESH = atof (argv[N]);
    remove_argument (N, &argc, argv);
    FlagOutlier = TRUE;
  }

  ExcludeBogus = FALSE;
  ExcludeBogusRadius = 0.0;
  if ((N = get_argument (argc, argv, "-exclude-bogus"))) {
    remove_argument (N, &argc, argv);
    ExcludeBogusRadius = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ExcludeBogus = TRUE;
  }

  GALAXY_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-galaxy-model"))) {
    remove_argument (N, &argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("FEAST-HIPPARCOS");

  // for testing, allow the galaxy model scale to be non-unity
  TEST_SCALE = 1.0;
  if ((N = get_argument (argc, argv, "-testing"))) {
    remove_argument (N, &argc, argv);
    TEST_SCALE = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (RELASTRO_OP == OP_NONE) usage_client (argc, argv);

  /* specify portion of the sky : allow default of all sky? */
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
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
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

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  N_BOOTSTRAP_SAMPLES = 100;
  if ((N = get_argument (argc, argv, "-bootstrap-samples"))) {
    remove_argument (N, &argc, argv);
    N_BOOTSTRAP_SAMPLES = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    if ((N_BOOTSTRAP_SAMPLES > 0) && (N_BOOTSTRAP_SAMPLES < 20)) {
      fprintf (stderr, "-bootstrap-samples must be either 0 (no sampling) or >= 20\n");
      exit (2);
    }
  }

  // when repairing warp detections, check the model coordinates for consistency
  USE_IMAGE_COORDS_FOR_REPAIR = FALSE;
  if ((N = get_argument (argc, argv, "-use-image-coords-for-repair"))) {
    remove_argument (N, &argc, argv);
    USE_IMAGE_COORDS_FOR_REPAIR = TRUE;
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

  DCR_BLUE_COLOR_POS = NULL;
  DCR_BLUE_COLOR_NEG = NULL;
  if ((N = get_argument (argc, argv, "-dcr-blue-color"))) {
    remove_argument (N, &argc, argv);
    DCR_BLUE_COLOR_POS = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    DCR_BLUE_COLOR_NEG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  DCR_RED_COLOR_POS = NULL;
  DCR_RED_COLOR_NEG = NULL;
  if ((N = get_argument (argc, argv, "-dcr-red-color"))) {
    remove_argument (N, &argc, argv);
    DCR_RED_COLOR_POS = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    DCR_RED_COLOR_NEG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_RESET_LIST = NULL;
  if ((N = get_argument (argc, argv, "-reset-to-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_RESET_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "+photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_SKIP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_SKIP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  // by default, require > 10pts to clip
  strcpy (STATMODE, "CHI_INNER_80_WTMEAN");
  if ((N = get_argument (argc, argv, "-statmode"))) {
    remove_argument (N, &argc, argv);
    strcpy (STATMODE, argv[N]);
    remove_argument (N, &argc, argv);
  }

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    remove_argument (N, &argc, argv);
    RESET = TRUE;
  }

  RESET_BAD_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-reset-bad-images"))) {
    remove_argument (N, &argc, argv);
    RESET_BAD_IMAGES = TRUE;
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  /* XXX update this */
  MIN_ERROR = 0.001;
  if ((N = get_argument (argc, argv, "-minerror"))) {
    remove_argument (N, &argc, argv);
    MIN_ERROR = atof (argv[N]);
    remove_argument (N, &argc, argv);
    /* require MIN_ERROR > 0 */
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
  
  // for now, make the default to ignore the photflags
  // XXX make it true by default instead?
  PhotFlagSelect = FALSE;
  if ((N = get_argument (argc, argv, "+photflags"))) {
    remove_argument (N, &argc, argv);
    PhotFlagSelect = TRUE;
  }

  PhotFlagBad = 0;
  if ((N = get_argument (argc, argv, "-photflagbad"))) {
    remove_argument (N, &argc, argv);
    PhotFlagBad = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PhotFlagPoor = 0;
  if ((N = get_argument (argc, argv, "-photflagpoor"))) {
    remove_argument (N, &argc, argv);
    PhotFlagPoor = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MinBadQF = 0.0;
  if ((N = get_argument (argc, argv, "-min-bad-psfqf"))) {
    remove_argument (N, &argc, argv);
    MinBadQF = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MaxMeanOffset = 10.0;
  if ((N = get_argument (argc, argv, "-max-mean-offset"))) {
    remove_argument (N, &argc, argv);
    MaxMeanOffset = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) usage_client (argc, argv);
  return TRUE;
}

void relastro_client_free (SkyTable *sky, SkyList *skylist) {
  FREE (PHOTCODE_SKIP_LIST);
  FREE (PHOTCODE_KEEP_LIST);
  FREE (PHOTCODE_RESET_LIST);
  FREE (DCR_RED_COLOR_POS);
  FREE (DCR_RED_COLOR_NEG);
  FREE (DCR_BLUE_COLOR_POS);
  FREE (DCR_BLUE_COLOR_NEG);

  FREE(PHOTCODE_A_LIST);
  FREE(PHOTCODE_B_LIST);
  FREE(HIGH_SPEED_DIR);
  FREE(BCATALOG);
  FREE(HOSTDIR);
  FREE(GALAXY_MODEL);

  // these are set in initialize
  FREE(photcodesKeep);
  FREE(photcodesSkip);  
  FREE(photcodesReset); 
  FREE(photcodesGroupA);
  FREE(photcodesGroupB);
  
  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  
  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
}

void usage (int argc, char **argv) {
  fprintf (stderr, "ERROR: USAGE: relastro -images -update-simple [options]\n");
  fprintf (stderr, "       OR:    relastro -images -update-chips [options]\n");
  fprintf (stderr, "       OR:    relastro -images -update-mosaic [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-regions -update-simple [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-regions -update-chips [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-regions -update-mosaic [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-images -update-simple [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-images -update-chips [options]\n");
  fprintf (stderr, "       OR:    relastro -parallel-images -update-mosaic [options]\n");
  fprintf (stderr, "       OR:    relastro -update-objects [options]\n");
  fprintf (stderr, "       OR:    relastro -high-speed [options]\n");
  fprintf (stderr, "       OR:    relastro -hpm [options]\n");
  fprintf (stderr, "       OR:    relastro -merge-source [options]\n\n");

  fprintf (stderr, "  specify one of the following modes: \n");
  fprintf (stderr, "  -update-objects\n");
  fprintf (stderr, "    -pm\n");
  fprintf (stderr, "    -par\n");
  fprintf (stderr, "    -pmpar\n");
  fprintf (stderr, "  -update-simple\n");
  fprintf (stderr, "  -update-chips\n");
  fprintf (stderr, "  -update-mosaics\n");
  fprintf (stderr, "  -high-speed (code[,code,code]) (code[,code,code]) (radius) (output catdir)\n");
  fprintf (stderr, "  -hpm (radius) (output catdir)\n");
  fprintf (stderr, "  -chipmap (MaxOrder)\n");
  fprintf (stderr, "  -merge-source (objID) (catID) into (objID) (catID)\n\n");

  fprintf (stderr, " additional options: \n");
  fprintf (stderr, "  -region RA RA DEC DEC\n");
  fprintf (stderr, "  -catalog (ra) (dec)\n\n");
  fprintf (stderr, "  -time (start)(stop)\n");
  fprintf (stderr, "  +photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -plot\n");
  fprintf (stderr, "  -plotdelay (seconds)\n");
  fprintf (stderr, "  -statmode (mode)\n");
  fprintf (stderr, "  -reset\n");
  fprintf (stderr, "  -reset-images\n");
  fprintf (stderr, "  -nloop (N) : number of image-fit iterations\n");
  fprintf (stderr, "  -update : apply new fit to database\n");
  fprintf (stderr, "  -params\n");
  fprintf (stderr, "  -plrange\n");
  fprintf (stderr, "  -minerror\n");
  fprintf (stderr, "  -instmag min max\n\n");
  fprintf (stderr, "  +photflags\n");
  fprintf (stderr, "  -photflagbad\n");
  fprintf (stderr, "  -photflagpoor\n");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  \n");

  fprintf (stderr, "remaining args: ");
  for (int i = 0; i < argc; i++) {
    fprintf (stderr, "%s ", argv[i]);
  }
  fprintf (stderr, "\n");

  exit (2);
} 

void usage_client (int argc, char **argv) {
  fprintf (stderr, "ERROR: USAGE: relastro_client -load\n");
  fprintf (stderr, "       OR:    relastro_client -update-offsets\n");
  fprintf (stderr, "       OR:    relastro_client -update-objects\n");
  fprintf (stderr, "       OR:    relastro_client -high-speed\n\n");

  fprintf (stderr, "  specify one of the following modes: \n");
  fprintf (stderr, "  -load : load the bright source detections for analysis\n");
  fprintf (stderr, "  -update-offsets : apply the updated image parameters\n");
  fprintf (stderr, "  -update-objects : calculate average astrometric properties of objects\n");
  fprintf (stderr, "  -high-speed (code[,code,code]) (code[,code,code]) (radius) (output catdir)\n");
  fprintf (stderr, "    support for the special high-speed mode\n");

  fprintf (stderr, " additional options: \n");
  fprintf (stderr, "  -pm : calculate proper motions\n");
  fprintf (stderr, "  -par : calculate parallaxes\n");
  fprintf (stderr, "  -pmpar : calculate motions and parallaxes\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax");
  fprintf (stderr, "  -catalog RA DEC");
  fprintf (stderr, "  -time (start)(stop)\n");
  fprintf (stderr, "  +photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -statmode (mode)\n");
  fprintf (stderr, "  -reset\n");
  fprintf (stderr, "  -minerror\n");
  fprintf (stderr, "  -instmag min max\n\n");
  fprintf (stderr, "  +photflags\n");
  fprintf (stderr, "  -photflagbad\n");
  fprintf (stderr, "  -photflagpoor\n");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  \n");

  fprintf (stderr, "remaining args: ");
  for (int i = 0; i < argc; i++) {
    fprintf (stderr, "%s ", argv[i]);
  }
  fprintf (stderr, "\n");

  exit (2);
} 

void usage_merge_source_id (char *name) {

  fprintf (stderr, "ERROR: invalid ID %s (remember to prefix 0x to hex IDs)\n", name);
  exit (2);
}

void usage_merge_source () {
  fprintf (stderr, "ERROR: USAGE: relastro -merge-source (objID) (catID) into (objID) (catID)\n");
  exit (2);
}

float *ParseLoopWeights (char *rawlist) {

  float *weights = NULL;
  ALLOCATE (weights, float, NLOOP);

  int Nloop = 0;

  /* parse the comma-separated list of photcodes */
  char *myList = strcreate(rawlist);
  char *list = myList;
  char *entry = NULL;
  char *ptr = NULL;
  while ((Nloop < NLOOP) && ((entry = strtok_r (list, ",", &ptr)) != NULL)) {
    list = NULL; // pass NULL on successive strtok_r calls

    weights[Nloop] = atof(entry);
    Nloop ++;
  }
  free (myList);

  if (Nloop == 0) {
    fprintf (stderr, "syntax error parsing weights: %s\n", rawlist);
    exit (3);
  }

  // this sets the last loops to match the last value...
  while (Nloop < NLOOP) {
    weights[Nloop] = weights[Nloop - 1];
    Nloop ++;
  }

  return weights;
}

int *ParseLoopOrder (char *rawlist, int minValue) {

  int *orders = NULL;
  ALLOCATE (orders, int, NLOOP);

  int Nloop = 0;

  /* parse the comma-separated list of photcodes */
  char *myList = strcreate(rawlist);
  char *list = myList;
  char *entry = NULL;
  char *ptr = NULL;
  while ((Nloop < NLOOP) && ((entry = strtok_r (list, ",", &ptr)) != NULL)) {
    list = NULL; // pass NULL on successive strtok_r calls

    orders[Nloop] = atoi(entry);
    if (orders[Nloop] < minValue) {
      fprintf (stderr, "order cannot be < %d: %s\n", minValue, rawlist);
      exit (3);
    }

    Nloop ++;
  }
  free (myList);

  if (Nloop == 0) {
    fprintf (stderr, "syntax error parsing orders: %s\n", rawlist);
    exit (3);
  }

  // this sets the last loops to match the last value...
  while (Nloop < NLOOP) {
    orders[Nloop] = orders[Nloop - 1];
    Nloop ++;
  }

  return orders;
}
