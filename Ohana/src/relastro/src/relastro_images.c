# include "relastro.h"

int relastro_images (SkyList *skylist) {

  int i, status, Ncatalog;
  Catalog *catalog;
  FITS_DB db;

  INITTIME;

  /* register database handle with shutdown procedure */
  set_db (&db);
  gfits_db_init (&db);

  // final pass fit mode (use FIT_AVERAGE only for the image loop)
  int finalPassMode = FIT_MODE; // start with the globally-defined fit mode
  FIT_MODE = FIT_AVERAGE;

  int RESET_ON_UPDATE = RESET;  
  RESET = TRUE; // we need to reset when we load the bright catalog subset 

  /* lock and load the image db table */
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  MARKTIME("load image data: %f sec\n", dtime);

  /* load regions and images based on specified sky patch (default depth) (require full overlap) */
  load_images (&db, skylist, TRUE, USE_ALL_IMAGES);
  MARKTIME("load images: %f sec\n", dtime);

  /* load catalog data from region files : subselect high-quality measurements */

  // photcodesKeep is used here to allow measurements from the images being calibrated
  // note if -reset-to-photcode is selected, photocodesKeep is replaced with below with photcodesReset
  catalog = load_catalogs (skylist, &Ncatalog, TRUE, 0, NULL, NULL);
  MARKTIME("load catalog data: %f sec\n", dtime);

  // find ICRF QSOs for reference downstream (only if USE_ICRF_CORRECT)
  select_catalog_ICRF (catalog, Ncatalog);

  if (photcodesReset) {
    photcodesKeep  = photcodesReset;
    NphotcodesKeep = NphotcodesReset;
  }

  if (Ncatalog == 0) {
    fprintf (stderr, "ERROR: no valid data for relastro, exiting\n");
    exit (2);
  }

  /* match measurements with images */
  initImageBins (catalog, Ncatalog, TRUE);
  MARKTIME("make image bins: %f sec\n", dtime);

  findImages (catalog, Ncatalog, TRUE);
  MARKTIME("set up image indexes: %f sec\n", dtime);

  if (PLOTSTUFF) {
    // plot_star_coords (catalog, Ncatalog);
    // plot_mosaic_fields (catalog);
  }

  // set test points based on the starmap
  createStarMap (catalog, Ncatalog);

  // XXX NOTE : for 2mass reset, photcodesKeep should now limit to 2MASS measurements

  USE_IRLS = FALSE;  // do not use IRLS yet -- leads to excessive outlier rejections in the loops

  /* major modes */
  switch (FIT_TARGET) {
    case TARGET_SIMPLE:
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i);
	UpdateSimple (catalog, Ncatalog);
      }
      break;

    case TARGET_CHIPS:
      if (RESET_IMAGES) UpdateMeasures (catalog, Ncatalog);
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i); // calculate <R>,<D>; if (i > 0), apply Galaxy Motion Model (if desired)
	UpdateChips (catalog, Ncatalog, i);   // measure.X,Y -> R,D, fit image.coords
	MARKTIME("update chips: %f sec\n", dtime);
      }

      // measure scatter for stacks
      UpdateStacks (catalog, Ncatalog);
      MARKTIME("UpdateStacks: %f sec\n", dtime);

      // create summary plots of the process
      // relastroVisualSummaryChips();
      break;

    case SET_CHIPS:
      // we just want to fit the selected chips to the mean positions
      UpdateChips (catalog, Ncatalog, 0);   // measure.X,Y -> R,D, fit image.coords
      MARKTIME("update chips: %f sec\n", dtime);
      break;

    case SET_STACKS:
      // we just want to fit the selected stacks to the mean positions
      UpdateStacks (catalog, Ncatalog);
      MARKTIME("update stacks : %f sec\n", dtime);
      break;

    case TARGET_MOSAICS:
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i);
	UpdateMosaic (catalog, Ncatalog);
      }
      break;

    default:
      fprintf (stderr, "programming error at %s:%d", __FILE__, __LINE__);
      exit (2);
  }

  // free the image / measurement pointers
  freeImageBins (Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    dvo_catalog_free (&catalog[i]);
  }
  free (catalog);
  freeMosaics ();

  if (!UPDATE) { 
    freeStarMaps();
    dvo_image_unlock (&db); 
    freeImages (db.ftable.buffer);
    gfits_db_free (&db);
    return TRUE;
  }

  // If we did NOT use all images, then we applied the measured corrections to a subset of
  // images.  we now need to set the images associated with db to have those values so
  // they will be written out by the dvo_image_update() operations.  If we USE_ALL_IMAGES,
  // then we do not need to do this, and we should call dvo_image_save (not dvo_image_update)
  if (!USE_ALL_IMAGES) {
    reload_images (&db);
  }
  freeStarMaps();
    
  if (PARALLEL) {
    // save the updated image parameters
    // need to also save the image map table...
    save_astrom_table ();
    if (USE_ALL_IMAGES) {
      dvo_image_save (&db, VERBOSE);
    } else {
      dvo_image_update (&db, VERBOSE);
    }
    dvo_image_unlock (&db); 
    freeImages (db.ftable.buffer);
    gfits_db_free (&db);
  }

  // NOTE: if we have parallel partitions, then we need to save the Images.dat and
  // AstroMap.fits tables BEFORE the remote relastro_clients are launched (they load 
  // Images.dat and AstroMap.fits)

  // if we do NOT have parallel partitions, we must NOT write them out yet: the act of
  // writing out the files byte-swaps the data and makes the values invalid for the
  // following calls to UpdateObjectOffsets (which attempt to apply the image values from
  // the structure in memory)

  FIT_MODE = finalPassMode; // use the user-selected mode for the final pass

  // iterate over catalogs to make detection coordinates consistant
  if (APPLY_OFFSETS) {
    USE_IRLS = ALLOW_IRLS;  // now that we have fitted the images, choose the user's option
    RESET = RESET_ON_UPDATE;
    UpdateObjectOffsets (skylist, 0, NULL);
  }

  if (!PARALLEL) {
    // save the updated image parameters
    save_astrom_table ();
    if (USE_ALL_IMAGES) {
      dvo_image_save (&db, VERBOSE);
    } else {
      dvo_image_update (&db, VERBOSE);
    }
    dvo_image_unlock (&db); 
    freeImages(db.ftable.buffer);
    gfits_db_free (&db);
  }

  return TRUE;
}
