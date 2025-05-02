# include "addstar.h"

int UpdateDatabase_Image (AddstarClientOptions *options, Image *images, int Nimages, Coords *mosaic, Stars *stars, unsigned int Nstars) {

  int i, status;
  Catalog catalog;
  SkyList *skylist, *newlist;

  if (options[0].mode != M_IMAGE) {
    fprintf (stderr, "error: expecting only IMAGE mode\n");
    return (FALSE);
  }

  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
  if (options[0].update) catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
  
  // XXX this is probably not needed anymore
  SetAirmassQuality (options[0].quality_airmass);

  /*** update catalog: average, measure, etc ***/

  /* find correpsonding regions for image */
  skylist = NULL;
  newlist = NULL;
  for (i = 0; i < Nimages; i++) {
      newlist = SkyListByImage (ServerSky, -1, &images[i]);
      SkyListMerge (&skylist, newlist);
      SkyListFree (newlist);
  }

  ImageOptions (options, images, Nimages);

  /* reduce regions to existing subset, if necessary */
  if (options[0].only_match || options[0].existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }
  if (VERBOSE) fprintf (stderr, "writing to %d regions\n", skylist[0].Nregions);

  for (i = 0; i < skylist[0].Nregions; i++) {

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = skylist[0].filename[i];
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    if (options[0].update) catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }

    // Naverage_disk == 0 implies an empty catalog file
    // for only_match, skip empty catalogs 
    if ((catalog.Naverage_disk == 0) && options[0].only_match) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (options[0].closest) {
      find_matches_closest (skylist[0].regions[i], stars, Nstars, &catalog, options[0]);
    } else {
      find_matches (skylist[0].regions[i], stars, Nstars, &catalog, options[0]);
    }

    if (!options[0].only_images) {
      SetProtect (TRUE);
      if (options[0].update) {
	catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
	dvo_catalog_update (&catalog, VERBOSE);
      } else {
	dvo_catalog_save (&catalog, VERBOSE);
      }
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }

  if (options[0].calibrate) { FindCalibration (&images[0]); }
  
  /*** load image db, save new image ***/
  { 
    FITS_DB db;

    /*** update the image table ***/
    /* setup image table format and lock */
    gfits_db_init (&db);
    db.mode   = dvo_catalog_catmode (CATMODE);
    db.format = dvo_catalog_catformat (CATFORMAT);
    status = dvo_image_lock (&db, ImageCat, 60.0, LCK_XCLD);
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

    /* write out new image */
    dvo_image_addrows (&db, images, Nimages);
    SetProtect (TRUE);
    dvo_image_update (&db, VERBOSE);
    SetProtect (FALSE);
    dvo_image_unlock (&db);
  }
  free (mosaic);
  free (images);
  free (stars);
  return (TRUE);
}
