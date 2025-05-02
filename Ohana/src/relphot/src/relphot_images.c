# include "relphot.h"

// 25.0 is the nominal zero point for measurements in the database
// for all measurements except PHOT_REF:
// measure.M contains: -2.5*log(DN) + 2.5*log(exptime) + 25.0 
// for measurements using PHOT_REF:
// measure.M contains: -2.5*log(DN) + 2.5*log(exptime) + True Zero Point (but exptime and ZP are known)

int relphot_images (SkyList *skylist) {

  int i, status, Ncatalog;
  Catalog *catalog = NULL;
  FITS_DB db;

  INITTIME;

  /* register database handle with shutdown procedure */
  set_db (&db);
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);

  /* lock and load the image db table */
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status && UPDATE) {
    fprintf (stderr, "error\n");
    Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  }

  // if the file is missing, db.dbstate will have a value of either:
  // LCK_EMPTY (if UPDATE) or LCK_MISSING (if !UPDATE)
  if ((db.dbstate == LCK_EMPTY) || (db.dbstate == LCK_MISSING)) {
    Shutdown ("ERROR: database %s contains no image data", CATDIR);
  }

  // read data from Image.dat file
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  MARKTIME("-- load image data: %f sec\n", dtime);

  // load regions and images based on specified sky patch and/or catalog 
  // NOTE: load_images transfers zero points from images to mosaics (initMosaicMcal)
  // and to tgroups (initTGroupMcal), resetting the image zero points to 0.0.

  // (if both tgroups and mosaics are used, the mosaics will hold the value and tgroups
  // will get values of 0.0.  This is not clearly the right way to go, but may not matter
  // if we reset everything).

  load_images (&db, skylist, &UserPatch, FALSE, USE_ALL_IMAGES);
  MARKTIME("-- load images: %f sec\n", dtime);

  /* unlock, if we can (else, unlocked below) */
  if (!UPDATE) dvo_image_unlock (&db); 

  initGridBins (); // allocates the empty array of corrections (elements are built below)

  if (CALIBRATE_STACKS_AND_WARPS || (NLOOP > 0)) {
    initGrid (); // allocate grid correction entries for existing photcodes

    /* load catalog data from region files (hostID is 0 since we are not a client */
    catalog = load_catalogs (skylist, &Ncatalog, 0, NULL, NULL);
    MARKTIME("-- load catalog data: %f sec\n", dtime);
  
    /* add in a loop over the catalogs calling dvo_catalog_chipcoords */

    /* match measurements with images, mosaics */
    initImageBins  (catalog, Ncatalog, TRUE);
    MARKTIME("-- make image bins: %f sec\n", dtime);

    initMosaicBins (catalog, Ncatalog, TRUE);
    initTGroupBins (catalog, Ncatalog);

    initMrel (catalog, Ncatalog);

    findImages (catalog, Ncatalog, TRUE);
    MARKTIME("-- set up image indexes: %f sec\n", dtime);

    findMosaics (catalog, Ncatalog, TRUE);
    MARKTIME("-- set up mosaic indexes: %f sec\n", dtime);

    findTGroups (catalog, Ncatalog);
    MARKTIME("-- set up mosaic indexes: %f sec\n", dtime);

    SAVEPLOT = FALSE;

    // mark ID_MEAS_AREA (user-selected chip region) and ID_MEAS_NOCAL (time range & active photcode)
    setExclusions (catalog, Ncatalog, TRUE);

    global_stats (catalog, Ncatalog, 0);

    if (PLOTSTUFF) {
      plot_star_coords (catalog, Ncatalog);
      // plot_mosaic_fields (catalog);
    }

    /* determine fit values */
    for (i = 0; i < NLOOP; i++) {
      SetZptIteration (i);

      rationalize_zeropoints (i);

      setMrel  (catalog, Ncatalog); // threaded (calls setMrelCatalog)

      setMcal  (catalog);
      setMmos  (catalog);
      setMgrp  (catalog);

      setMgrid (catalog, Ncatalog);
      MARKTIME("-- set Mrel, Mcal, Mmos, Mgrid : %f sec\n", dtime);
    
      if (PLOTSTUFF) {
	plot_scatter (catalog, Ncatalog); 
	plot_mosaics ();
	plot_images ();
	plot_stars (catalog, Ncatalog);
	plot_chisq (catalog, Ncatalog);
      }

      global_stats (catalog, Ncatalog, i);
      SetZeroPointModes (catalog, Ncatalog);

      MARKTIME("-- finished loop %d: %f sec\n", i, dtime);
      fprintf (stderr, "----- continue loop %d -----\n", i);
    }

    if (PLOTSTUFF) {
      plot_scatter (catalog, Ncatalog); 
      plot_mosaics ();
      plot_images ();
      plot_stars (catalog, Ncatalog);
      plot_chisq (catalog, Ncatalog);
    }
  
    // if (GRID_ZEROPT) dump_grid ();
    setMcal  (catalog);
    setMmos  (catalog);
    setMgrp  (catalog);

    setMcal  (catalog);
    setMmos  (catalog);
    setMgrp  (catalog);
    MARKTIME("-- finalize Mcal values: %f sec\n", dtime);

    setMcalFromMosaics (); // copy per-mosaic calibrations to the images
    setMcalFromTGroups (); // copy per-tgroup calibrations to the images

    // calculate and save per-chip residuals here:
    MagResidSave ("mag.resid.fits", catalog);

    // DumpAllMags ("all.mags.fits", catalog, Ncatalog);

    save_images_updates (&db);

    /* at this point, we have correct cal coeffs in the image/mosaic structures */
    for (i = 0; i < Ncatalog; i++) {
      // these tiny values are set by BrightCatalogSplit from load_catalogs
      free_tiny_values (&catalog[i]);
      dvo_catalog_free (&catalog[i]);
    }
    free (catalog);
    freeImageBins (Ncatalog, TRUE);
    freeMosaicBins (Ncatalog, TRUE);
    freeTGroupBins (Ncatalog);

    GridCorrectionSave ();

    // end of if (NLOOP > 0) block : this loop determines the offsets per chip 
  } else { 
    // If nloop == 0, the above pass is not performed, in which case
    // the Mcal values passed to the mosaics are not returned to the images...
    setMcalFromMosaics (); // copy per-mosaic calibrations to the images
    setMcalFromTGroups (); // copy per-tgroup calibrations to the images
  }

  // once the zero points have been calculated, they are reassigned to the
  // individual images.  At this point, the mosaic and tgroup values should not 
  // be used.
  freeMosaics ();
  freeTGroups ();
  MOSAIC_ZEROPT = FALSE;
  TGROUP_ZEROPT = FALSE;

  // only change the real database files if -update is requested
  if (!UPDATE) {
    dvo_image_unlock (&db); 
    freeImages (db.ftable.buffer);
    gfits_db_free (&db);
    freeGridBins ();
    return TRUE;
  }
  
  reload_images (&db);

  /* Load catalog data from region files, update Mrel include all data.  In a parallel
     context, this function writes the image parameters as a subset table for the remote
     clients */
  if (!CALIBRATE_STACKS_AND_WARPS) {
    reload_catalogs (skylist, 0, NULL); // calls setMrelFinal which setMrelOutput which calls setMrelCatalog
    MARKTIME("-- updated all catalogs: %f sec\n", dtime);
  }

  if (UPDATE_CATFORMAT) {
    // ensure the Image db format is updated
    db.format = dvo_catalog_catformat (UPDATE_CATFORMAT);
    gfits_modify (&db.header, "FORMAT", "%s", 1, UPDATE_CATFORMAT);
  }
  if (CALIBRATE_STACKS_AND_WARPS || (NLOOP > 0)) {
    // do not save changes if we did not make changes. 
    dvo_image_update (&db, VERBOSE);
  }
  dvo_image_unlock (&db); 

  freeImages (db.ftable.buffer);
  freeGridBins ();

  gfits_db_free (&db);

  return TRUE;
}

