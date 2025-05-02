# include "addstar.h"
void help (void);

# define RESETTIME { gettimeofday (&startTimer, (void *) NULL); }

// LARGEFILES: this program currently limits Nstars (input file) to < 2^31
int main (int argc, char **argv) {

  int Nmatch, status, loadObjects;
  off_t i, Nimages;
  off_t Naverage, Nmeasure, Nlensing;
  Catalog catalog;
  FITS_DB db;
  AddstarClientOptions options;

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  SkyList *newlist = NULL;

  struct timeval startAddstar, stopAddstar;
  gettimeofday (&startAddstar, (void *) NULL);

  INITTIME;

  /* check for help request */
  if (get_argument (argc, argv, "-h"    )) help();
  if (get_argument (argc, argv, "-help" )) help();
  if (get_argument (argc, argv, "--help")) help();

  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args (argc, argv, options);

  if (options.mode == ADDSTAR_MODE_CREATE_ID) {
    addstar_create_ID ();
    exit (0);
  }

  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  if (sky == NULL) {
      fprintf (stderr, "ERROR: unable to load sky table data\n");
      exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  if (options.mode == ADDSTAR_MODE_RESORT) {
    resort_catalogs (&options, sky);
    exit (0);
  }

  MARKTIME ("init and config: %f sec\n", dtime); RESETTIME; 

  Catalog *newcat = NULL;
  Image *images = NULL;

  /*** load in the new data (images, stars) ***/
  switch (options.mode) {
    case ADDSTAR_MODE_IMAGE:
      newcat = LoadStars (argv[1], &images, &Nimages, &options);
      MARKTIME ("load smf: %f sec\n", dtime); RESETTIME; 

      // set and update the imageID sequence
      UpdateImageIDs (newcat, images, Nimages);

      for (i = 0; i < Nimages; i++) {
	newlist = SkyListByImage (sky, -1, &images[i]);
	SkyListMerge (&skylist, newlist);
	if (VERBOSE) fprintf (stderr, "added "OFF_T_FMT" regions to yield "OFF_T_FMT" total\n",  newlist[0].Nregions,  skylist[0].Nregions);
	SkyListFree (newlist);
      }
      ImageOptions (&options, images, Nimages);
      break;
    case ADDSTAR_MODE_REFLIST:
      newcat = grefstars (argv[1], options.photcode);
      skylist = SkyListForStars (sky, -1, newcat);
      break;
    case ADDSTAR_MODE_REFFITS:
      newcat = greffits (argv[1], options.photcode);
      skylist = SkyListForStars (sky, -1, newcat);
      break;
    case ADDSTAR_MODE_REFCAT:
      skylist = SkyListByPatch (sky, -1, &UserPatch);
      break;
    case ADDSTAR_MODE_FAKEIMAGE:
      images = fakeimage (argv[1], &Nimages, options.photcode);
      ALLOCATE (skylist, SkyList, 1);
      skylist[0].Nregions = 0;
      skylist[0].ownElements = FALSE;
      break;

    default:
      fprintf (stderr, "ERROR: invalid mode\n");
      exit (2);
  }

  // in these cases, limit the sky catalogs to an existing subset
  if (options.only_match || options.existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }
  if (VERBOSE) fprintf (stderr, "writing to "OFF_T_FMT" regions\n",  skylist[0].Nregions);

  /* don't load the object tables for only_images, unless we are getting the calibration. */
  loadObjects = !options.only_images || options.calibrate;

  /* match stars to existing catalog data (or otherwise manipulate catalog data) */
  Nmatch = Naverage = Nmeasure = Nlensing = 0;
  for (i = 0; loadObjects && (i < skylist[0].Nregions); i++) {

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename    = skylist[0].filename[i];
    catalog.catmode     = dvo_catalog_catmode (CATMODE);         // set the default catmode from config data
    catalog.catformat   = dvo_catalog_catformat (CATFORMAT);     // set the default catformat from config data
    catalog.catcompress = dvo_catalog_catcompress (CATCOMPRESS); // set the default catcompress from config data
    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_LENSING;
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();

    // XXX need to do something to enforce consistency.  we can change options.update
    // based on the value of catmode as below.  however, this seems to be a problem if
    // there is an inconsistency between the config values and the existing db.  not so
    // bad if the existing db is SPLIT, but a problem if we request SPLIT and the existing
    // db is MEF.
    if (catalog.catmode != DVO_MODE_SPLIT) options.update = FALSE;

    if (options.update) catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    // open as read or write, depending on desire
    if (options.only_images && options.calibrate) {
      if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "r")) {
	continue;
      }
    } else {
      // an error exit status here is a significant error (disk I/O or file access)
      // XXX should this be "a" for options.update?
      if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
	fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
	exit (2);
      }
    }
    if (VERBOSE) MARKTIME ("load cpt: %f sec\n", dtime); RESETTIME; 

    // Naverage_disk == 0 implies an empty catalog file
    // for only_match, skip empty catalogs
    if ((catalog.Naverage_disk == 0) && options.only_match) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    switch (options.mode) {
      case ADDSTAR_MODE_IMAGE:
	if (options.closest) {
	  Nmatch += find_matches_closest (skylist[0].regions[i], newcat, &catalog, options);
	} else {
	  Nmatch += find_matches (skylist[0].regions[i], newcat, &catalog, options);
	}
	break;

      case ADDSTAR_MODE_REFCAT:
	newcat = greference (argv[1], skylist[0].regions[i], options.photcode);
      case ADDSTAR_MODE_REFLIST:
      case ADDSTAR_MODE_REFFITS:
	{
	  if (options.closest) {
	    Nmatch += find_matches_closest_refstars (skylist[0].regions[i], newcat, &catalog, options);
	  } else {
	    Nmatch += find_matches_refstars (skylist[0].regions[i], newcat, &catalog, options);
	  }
	  break;
	}
      default:
	abort();
    }
    if (VERBOSE) MARKTIME ("match stars: %f sec\n", dtime); RESETTIME; 

    /* report total updated values */
    Naverage += catalog.Naverage;
    Nmeasure += catalog.Nmeasure;
    Nlensing += catalog.Nlensing;

    // write out catalog, if appropriate
    if (newcat->Naverage && !options.only_images) {
      SetProtect (TRUE);
      if (options.update) {
	catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_LENSING;
	dvo_catalog_update (&catalog, VERBOSE);
      } else {
	dvo_catalog_save (&catalog, VERBOSE);
      }
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
    if (VERBOSE) MARKTIME ("save cpt: %f sec\n", dtime); RESETTIME; 

    if (options.mode == ADDSTAR_MODE_REFCAT) dvo_catalog_free (newcat);
  }
  SkyListFree (skylist);

  // We only measure a single value for the entire mosaic (add all images to this function)
  if (options.calibrate) { FindCalibration (&images[0]); }

  /*** update the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    if (VERBOSE) fprintf (stderr, "can't find %s, creating a new one\n", ImageCat);
    dvo_image_create (&db, GetZeroPoint());
  } else {
    if (!dvo_image_load (&db, VERBOSE, FORCE_READ)) {
      Shutdown ("can't read image catalog %s", db.filename);
    }
  }

  /* add the new images and save */
  if (options.mode == ADDSTAR_MODE_IMAGE) {
    dvo_image_addrows (&db, images, Nimages);
    SetProtect (TRUE);
    dvo_image_update (&db, VERBOSE);
    SetProtect (FALSE);
  }
  FREE (images);
  dvo_image_unlock (&db); /* unlock? */
  gfits_db_free (&db); /* unlock? */

  gettimeofday (&stopAddstar, (void *) NULL);
  float dtime = DTIME (stopAddstar, startAddstar);
  fprintf (stderr, "SUCCESS: elapsed time %9.4f sec for "OFF_T_FMT" stars (%5d matches), "OFF_T_FMT" average, "OFF_T_FMT" measure, "OFF_T_FMT" lensing\n", dtime, newcat->Naverage, Nmatch,  Naverage,  Nmeasure, Nlensing);

  if (options.mode != ADDSTAR_MODE_REFCAT) {
    dvo_catalog_free (newcat);
    free (newcat);
  }

  // XXX test
  FreeConfig ();
  FreePhotcodeTable ();
  SkyTableFree (sky);
  exit (0);
}

/* names:
   catalog - existing object db table
   regions - sky area which may or may not contain data
   patch   - RA,DEC bounded portion of sky
*/

// add in case of failures:
// ohana_memcheck (FALSE);
