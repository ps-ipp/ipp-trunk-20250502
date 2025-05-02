# include "addstar.h"
void help (void);

AddstarClientOptions args (int argc, char **argv, AddstarClientOptions options) {
  
  int i, N;
  int QUALITY_AIRMASS;

  // a global used by find_matches_refstars.c (value is 1 except for load2mass)
  NREFSTAR_GROUP = 1;
  // a global used by find_matches_closest.c (value is 1 except for loadsdss)
  NSTAR_GROUP = 1;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /*** check for command line options ***/

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_IMAGE;
  if ((N = get_argument (argc, argv, "-ref"))) {
    options.mode = ADDSTAR_MODE_REFLIST;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-reffits"))) {
    options.mode = ADDSTAR_MODE_REFFITS;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-cat"))) {
    options.mode = ADDSTAR_MODE_REFCAT;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-resort"))) {
    options.mode = ADDSTAR_MODE_RESORT;
    remove_argument (N, &argc, argv);
  }
  OLD_RESORT = FALSE;
  if ((N = get_argument (argc, argv, "-old-resort"))) {
    remove_argument (N, &argc, argv);
    OLD_RESORT = TRUE;
  }

  if ((N = get_argument (argc, argv, "-create-id"))) {
    options.mode = ADDSTAR_MODE_CREATE_ID;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-fakeimage"))) {
    options.mode = ADDSTAR_MODE_FAKEIMAGE;
    remove_argument (N, &argc, argv);
    FAKE_RA = atof (argv[N]);
    remove_argument (N, &argc, argv);
    FAKE_DEC = atof (argv[N]);
    remove_argument (N, &argc, argv);
    FAKE_THETA = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  OFFSET_ZPT = NAN;
  if ((N = get_argument (argc, argv, "-fakezpt"))) {
    remove_argument (N, &argc, argv);
    OFFSET_ZPT = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  options.filelist = FALSE;
  if ((N = get_argument (argc, argv, "-list"))) {
    options.filelist = TRUE;
    remove_argument (N, &argc, argv);
  }

  USE_NAME = NULL;
  if ((N = get_argument (argc, argv, "-use-name"))) {
    remove_argument (N, &argc, argv);
    USE_NAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PMM_CCD_TABLE = NULL;
  if ((N = get_argument (argc, argv, "-pmm"))) {
    remove_argument (N, &argc, argv);
    PMM_CCD_TABLE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  READ_XRAD_DATA = FALSE;
  if ((N = get_argument (argc, argv, "-xrad"))) {
    remove_argument (N, &argc, argv);
    READ_XRAD_DATA = TRUE;
  }
  DIFF_WITH_INV = FALSE;
  if ((N = get_argument (argc, argv, "-diff-inv"))) {
    remove_argument (N, &argc, argv);
    DIFF_WITH_INV = TRUE;
  }

  // number of worker threads for resort (must have at least one worker thread)
  NTHREADS = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    NTHREADS = MAX(0, atoi (argv[N]));
    remove_argument (N, &argc, argv);
  }

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? (REFCAT only) */
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
  } else {
      if (options.mode == ADDSTAR_MODE_IMAGE) goto allow;
      if (options.mode == ADDSTAR_MODE_FAKEIMAGE) goto allow;
      if (options.mode == ADDSTAR_MODE_REFLIST) goto allow;
      if (options.mode == ADDSTAR_MODE_CREATE_ID) goto allow;
      if (options.mode == ADDSTAR_MODE_REFCAT) {
	  fprintf (stderr, "you have requested uploading from a catalog to the entire sky in one pass\n");
      }
      if (options.mode == ADDSTAR_MODE_RESORT) {
	  fprintf (stderr, "you have requested resorting the entire sky in one pass\n");
      }
      fprintf (stderr, "this could be a time consuming operation.  type Ctrl-C within 5 seconds to cancel\n");
      for (i = 5; i > 0; i--) {
	  fprintf (stderr, "%d.. ", i);
	  usleep (1000000);
      }
      fprintf (stderr, "\n");
  }
allow:
  /* override any header PHOTCODE values */
  options.photcode = 0;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    options.photcode = GetPhotcodeCodebyName (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    options.photcode = GetPhotcodeCodebyName (argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* provide a time for dataset */
  TIMEREF = 0; 
  if ((N = get_argument (argc, argv, "-time"))) {
    time_t tmp;
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tmp)) { 
      fprintf (stderr, "syntax error in time\n");
      exit (1);
    }
    TIMEREF = tmp;
    remove_argument (N, &argc, argv);
  }
  /* provide a mosaic for distortion : this is somewhat weak -- it must also be added to the db */
  options.mosaic = FALSE;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    Header header;
    Coords MOSAIC;

    remove_argument (N, &argc, argv);
    if (!gfits_read_header (argv[N], &header)) {
      fprintf (stderr, "ERROR: can't read header for mosaic %s\n", argv[N]);
      exit (1);
    }
    if (!GetCoords (&MOSAIC, &header)) {
      fprintf (stderr, "ERROR: no astrometric solution in header\n");
      exit (1);
    }
    if (strcmp(&MOSAIC.ctype[4], "-DIS")) {
      fprintf (stderr, "ERROR: not a mosaic distortion header\n");
      exit (1);
    }
    saveMosaicCoords (&MOSAIC);
    remove_argument (N, &argc, argv);
    gfits_free_header (&header);
    options.mosaic = TRUE;
  }
  
  /*** modify behavior ***/
  /* only add to existing objects */
  options.existing_regions = FALSE;
  if ((N = get_argument (argc, argv, "-existing-regions"))) {
    options.existing_regions = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* only add to existing objects */
  options.only_match = FALSE;
  if ((N = get_argument (argc, argv, "-only-match"))) {
    options.only_match = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* don't add missed pts to Missed table (image only) */
  options.skip_missed = TRUE;
  if ((N = get_argument (argc, argv, "-missed"))) {
    options.skip_missed = TRUE;
    remove_argument (N, &argc, argv);
    fprintf (stderr, "ERROR: addstar no longer supports -missed\n");
    exit (2);
  }
  /* replace measurement, don't duplicate (ref/cat only) */
  options.replace = FALSE;
  if ((N = get_argument (argc, argv, "-replace"))) {
    options.replace = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* use 'closest star' matching by default */
  options.closest = TRUE;
  if ((N = get_argument (argc, argv, "-closest"))) {
    options.closest = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-all-matches"))) {
    options.closest = FALSE;
    remove_argument (N, &argc, argv);
  }

  /* don't re-sort the measure sequence */
  options.nosort = FALSE;
  if ((N = get_argument (argc, argv, "-nosort"))) {
    options.nosort = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* only add new rows (-update) or re-write complete measure table (forces -nosort) */
  options.update = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    options.update = TRUE;
    options.nosort = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-force-sort"))) {
    options.nosort = 3;  // temporary mode to mean 'force-sort'
    remove_argument (N, &argc, argv);
  }

  /* only add image potion to image table */
  options.only_images = FALSE;
  if ((N = get_argument (argc, argv, "-image"))) {
    options.only_images = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* apply average zpt offset calibration (image only) */
  options.calibrate = FALSE;
  if ((N = get_argument (argc, argv, "-cal"))) {
    options.calibrate = TRUE;
    remove_argument (N, &argc, argv);
    InitCalibration (FALSE);
  }
  if ((N = get_argument (argc, argv, "-excal"))) {
    options.calibrate = TRUE;
    remove_argument (N, &argc, argv);
    InitCalibration (FALSE);
  }
  if ((N = get_argument (argc, argv, "-incal"))) {
    options.calibrate = TRUE;
    remove_argument (N, &argc, argv);
    InitCalibration (TRUE);
  }

  /*** optional situations ***/
  /* choose high quality airmass vs low quality airmass (per-star vs per-image) */
  QUALITY_AIRMASS = TRUE;
  if ((N = get_argument (argc, argv, "-quality-airmass"))) {
    remove_argument (N, &argc, argv);
    QUALITY_AIRMASS = TRUE;
  }
  if ((N = get_argument (argc, argv, "-quick-airmass"))) {
    remove_argument (N, &argc, argv);
    QUALITY_AIRMASS = FALSE;
  }
  /* choose high quality airmass vs low quality airmass (per-star vs per-image) */
  SUBPIX = FALSE;
  if ((N = get_argument (argc, argv, "-subpix"))) {
    remove_argument (N, &argc, argv);
    SUBPIX = TRUE;
  }
  /* skyprobe means: subpix correction and quality airmass */ 
  if ((N = get_argument (argc, argv, "-skyprobe"))) {
    remove_argument (N, &argc, argv);
    QUALITY_AIRMASS = TRUE;
    SUBPIX = TRUE;
  }
  SetAirmassQuality (QUALITY_AIRMASS);
  if (SUBPIX) load_subpix ();

  /* define 2MASS quality flags to keep */
  SELECT_2MASS_QUALITY = NULL;
  if ((N = get_argument (argc, argv, "-2massquality"))) {
    remove_argument (N, &argc, argv);
    SELECT_2MASS_QUALITY = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* force using single time from PHU */
  FORCE_SINGLE_TIME = FALSE;
  if ((N = get_argument (argc, argv, "-force-single-time"))) {
    FORCE_SINGLE_TIME = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* accept bad header astrometry */
  ACCEPT_ASTROM = FALSE;
  if ((N = get_argument (argc, argv, "-accept"))) {
    ACCEPT_ASTROM = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-accept-astrom"))) {
    ACCEPT_ASTROM = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* accept proper-motion data from reference */
  ACCEPT_MOTION = FALSE;
  if ((N = get_argument (argc, argv, "-accept-motion"))) {
    ACCEPT_MOTION = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* accept bad header astrometry */
  ACCEPT_TIME = FALSE;
  if ((N = get_argument (argc, argv, "-accept-time"))) {
    ACCEPT_TIME = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* skip the stars */
  NO_STARS = FALSE;
  if ((N = get_argument (argc, argv, "-no-stars"))) {
    NO_STARS = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* skip the stars */
  NO_DUPLICATE_IMAGES = TRUE;
  if ((N = get_argument (argc, argv, "-dup-images"))) {
    NO_DUPLICATE_IMAGES = FALSE;
    remove_argument (N, &argc, argv);
  }
  /* skip the stars */
  IMAGE_ID_OVERRIDE = 0;
  if ((N = get_argument (argc, argv, "-image-id-override"))) {
    remove_argument (N, &argc, argv);
    IMAGE_ID_OVERRIDE = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* force read of image database with mismatched NSTARS & size */ 
  FORCE_READ = FALSE;
  if ((N = get_argument (argc, argv, "-force"))) {
    FORCE_READ = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* force read of image database with mismatched NSTARS & size */ 
  TEXTMODE = FALSE;
  if ((N = get_argument (argc, argv, "-textmode"))) {
    TEXTMODE = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  DUMP = NULL;
  if ((N = get_argument (argc, argv, "-dump"))) {
    remove_argument (N, &argc, argv);
    DUMP = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  HOST_ID = 0;
  HOSTDIR = NULL;
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
  if (PARALLEL) {
    if (options.mode != ADDSTAR_MODE_RESORT) {
      fprintf (stderr, "parallel mode is only valid for -resort mode\n");
      exit (2);
    }
  }

  if ((options.mode == ADDSTAR_MODE_CREATE_ID) && (argc == 1)) return (options);
  if ((options.mode == ADDSTAR_MODE_RESORT) && (argc == 1)) return (options);

  if ((options.mode == ADDSTAR_MODE_REFLIST) && (options.photcode == 0)) {
    fprintf (stderr, "photcode must be specified for -ref\n");
    exit (2);
  }
  if ((options.mode == ADDSTAR_MODE_REFFITS) && (options.photcode == 0)) {
    fprintf (stderr, "photcode must be specified for -reffits\n");
    exit (2);
  }

  if (argc == 2) return (options);

  fprintf (stderr, "USAGE: addstar (filename)\n");
  fprintf (stderr, "USAGE: addstar -list (filename)\n");
  fprintf (stderr, "USAGE: addstar -cat (catalog)\n");
  fprintf (stderr, "USAGE: addstar -ref (filename)\n");
  fprintf (stderr, "USAGE: addstar -fakeimage (ra) (dec) (theta) (name)\n");
  fprintf (stderr, "USAGE: addstar -resort (SkyRegion)\n");
  fprintf (stderr, "USAGE: addstar -add-id\n");
  fprintf (stderr, "USAGE: addstar -ppm (filename)\n");
  exit (2);
}

void help (void) {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  addstar (filename)\n");
  fprintf (stderr, "     add specified image (cmp format) to database\n\n");
  fprintf (stderr, "  addstar -list (filename)\n");
  fprintf (stderr, "     add list of images (cmp format) to database\n\n");
  fprintf (stderr, "  addstar -ref (filename)");
  fprintf (stderr, "     add ASCII data (ra dec mag dmag) to database\n\n");
  fprintf (stderr, "  addstar -cat (catalog)");
  fprintf (stderr, "     add data from catalog (USNO/2MASS/GSC) to database\n\n");
  fprintf (stderr, "  addstar -resort (SkyRegion)");
  fprintf (stderr, "     perform measure sorting for the specified catalog\n\n");
  fprintf (stderr, "  addstar -fakeimage (ra) (dec) (theta) (name)");
  fprintf (stderr, "     insert a fake image in the db\n\n");
  fprintf (stderr, "  addstar -pmm (filename)");
  fprintf (stderr, "     insert pmm table into database\n\n");
  fprintf (stderr, "  addstar -create-id");
  fprintf (stderr, "     add a dvodb ID to the image table (and exit)\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -region ra ra dec dec 	  : only add data in specified region (-ref mode only)\n");
  fprintf (stderr, "  -p (photcode)         	  : specify photcode (-ref / -cat / or override header)\n");
  fprintf (stderr, "  -photcode (photcode)    	  : specify photcode (-ref / -cat / or override header)\n");
  fprintf (stderr, "  -time (YYYY/MM/DD,HH:MM:SS) : specify date/time (override header)\n");
  fprintf (stderr, "  -mosaic (filename)    	  : identify associated mosaic frame for chip image\n");
  fprintf (stderr, "  -textmode                	  : input file is RAW TEXT table, not FITS table\n");
  fprintf (stderr, "  -existing-regions           : only add measurements to existing catalog files\n");
  fprintf (stderr, "  -only-match           	  : only add measurements to existing objects\n");
  fprintf (stderr, "  -missed               	  : skipped 'missed' entries\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -closest             	  : use closest-star algorith (default)\n");
  fprintf (stderr, "  -all-matches             	  : use all-matches algorith\n");
  fprintf (stderr, "  -nosort             	  : don't re-sort the measure entries (improves speed)\n");
  fprintf (stderr, "  -update             	  : only update the new rows (forces -nosort)\n");
  fprintf (stderr, "  -force-sort             	  : \n");
  fprintf (stderr, "  -image                	  : only insert image data\n");
  fprintf (stderr, "  -cal                  	  : perform zero-point calibration\n");
  fprintf (stderr, "  -excal                  	  : apply supplied zero-point calibration\n");
  fprintf (stderr, "  -incal                  	  : perform zero-point calibration\n");
  fprintf (stderr, "  -quality-airmass            : use per-star airmass values (default)\n");
  fprintf (stderr, "  -quick-airmass              : use per-image airmass values\n");
  fprintf (stderr, "  -subpix             	  : apply subpixel corrections\n");
  fprintf (stderr, "  -skyprobe             	  : specify skyprobe mode\n");
  fprintf (stderr, "  -2massquality            	  : define 2MASS quality flags to keep\n");
  fprintf (stderr, "  -accept               	  : accept bad astrometry from header\n");
  fprintf (stderr, "  -accept-astrom          	  : accept bad astrometry from header\n");
  fprintf (stderr, "  -accept-motion           	  : accept proper-motion data from reference\n");
  fprintf (stderr, "  -accept-time           	  : use TZERO supplied in header\n");
  fprintf (stderr, "  -use-name                	  : use the given name instead of the filename as the filename\n");
  fprintf (stderr, "  -no-stars                	  : skip the stars\n");
  fprintf (stderr, "  -dup-images             	  : skip the test for duplicate image IDs (test only!)\n");
  fprintf (stderr, "  -force                	  : force read of database with inconsistent info\n");
  fprintf (stderr, "  -threads (N)            	  : use N threads for resort option\n");
  fprintf (stderr, "  -fakezpt (offset)        	  : apply an artificial offset to the magnitudes (for testing)\n");
  fprintf (stderr, "  -textmode                	  : force textmode for file (ignore header clues)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -dump (mode)          	  : output test data. Currently supported values for mode are: 'rawdata', 'cal'.\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");

  exit (2);
}

/** addstar modes:
 
    addstar (image.smp)  - add cmp/smp image data to db
    addstar -ref (file.dat) (photcode) 
    addstar -cat (USNO/2MASS/GSC) -region (ra dec - ra dec)

    -replace : ref/cat - replace existing match (photcode/time)
    -match   : ref/cat - only add measures to existing averages

    ref types: 
    ASCII - RA,DEC,M,dM in a table

    addstar 

**/
