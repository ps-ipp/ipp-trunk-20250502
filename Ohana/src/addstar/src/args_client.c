# include "addstar.h"
static void help (void);

AddstarClientOptions args_client (int argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  // a global used by find_matches_refstars.c (value is 1 except for load2mass)
  NREFSTAR_GROUP = 1;

  /*** check for command line options ***/

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_IMAGE;
  if ((N = get_argument (argc, argv, "-ref"))) {
    options.mode = ADDSTAR_MODE_REFLIST;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-cat"))) {
    options.mode = ADDSTAR_MODE_REFCAT;
    remove_argument (N, &argc, argv);
  }

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? (REFCAT only) */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
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
  /* override any header PHOTCODE values */
  options.photcode = 0;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    options.photcode = GetPhotcodeCodebyName (argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* provide a time for dataset */
  options.timeref = 0; 
  if ((N = get_argument (argc, argv, "-time"))) {
    time_t tmp;
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tmp)) { 
      fprintf (stderr, "syntax error in time\n");
      exit (1);
    }
    options.timeref = tmp;
    remove_argument (N, &argc, argv);
  }
  /* provide a mosaic for distortion */
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
  options.skip_missed = FALSE;
  if ((N = get_argument (argc, argv, "-missed"))) {
    options.skip_missed = TRUE;
    remove_argument (N, &argc, argv);
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
  }

  /*** optional situations ***/
  /* choose high quality airmass vs low quality airmass (per-star vs per-image) */
  options.quality_airmass = FALSE;
  if ((N = get_argument (argc, argv, "-quality-airmass"))) {
    remove_argument (N, &argc, argv);
    options.quality_airmass = TRUE;
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
    options.quality_airmass = TRUE;
    SUBPIX = TRUE;
  }
  if (SUBPIX) load_subpix ();

  /* define 2MASS quality flags to keep */
  SELECT_2MASS_QUALITY = NULL;
  if ((N = get_argument (argc, argv, "-2massquality"))) {
    remove_argument (N, &argc, argv);
    SELECT_2MASS_QUALITY = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* accept bad header astrometry */
  ACCEPT_ASTROM = FALSE;
  if ((N = get_argument (argc, argv, "-accept"))) {
    ACCEPT_ASTROM = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* force read of image database with mismatched NSTARS & size */ 
  FORCE_READ = FALSE;
  if ((N = get_argument (argc, argv, "-force"))) {
    FORCE_READ = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* over-ride autointerpretation of input data format */ 
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

  if (argc != 2) {
    fprintf (stderr, "USAGE: addstarc (filename)\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  addstar (filename)\n");
  fprintf (stderr, "     add specified image (cmp format) to database\n\n");
  fprintf (stderr, "  addstar -ref (filename)");
  fprintf (stderr, "     add ASCII data (ra dec mag dmag) to database\n\n");
  fprintf (stderr, "  addstar -cat (catalog)");
  fprintf (stderr, "     add data from catalog (USNO/2MASS/GSC) to database\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -region ra ra dec dec 	  : only add data in specified region (-ref mode only)\n");
  fprintf (stderr, "  -p (photcode)         	  : specify photcode (override header)\n");
  fprintf (stderr, "  -time (YYYY/MM/DD,HH:MM:SS) : specify date/time (override header)\n");
  fprintf (stderr, "  -mosaic (filename)    	  : identify associated mosaic frame for chip image\n");
  fprintf (stderr, "  -fits                 	  : input file is FITS table, not TEXT table\n");
  fprintf (stderr, "  -existing-regions           : only add measurements to existing catalog files\n");
  fprintf (stderr, "  -only-match           	  : only add measurements to existing objects\n");
  fprintf (stderr, "  -missed               	  : skipped 'missed' entries\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -closest             	  : use closest-star algorith\n");
  fprintf (stderr, "  -nosort             	  : don't re-sort the measure entries (improves speed)\n");
  fprintf (stderr, "  -update             	  : only update the new rows (foreces -nosort)\n");
  fprintf (stderr, "  -image                	  : only insert image data\n");
  fprintf (stderr, "  -cal                  	  : perform zero-point calibration\n");
  fprintf (stderr, "  -skyprobe             	  : specify skyprobe mode\n");
  fprintf (stderr, "  -accept               	  : accept bad astrometry from header\n");
  fprintf (stderr, "  -force                	  : force read of database with inconsistent info\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -dump (mode)          	  : output test data\n");
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

