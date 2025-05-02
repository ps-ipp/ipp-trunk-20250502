# include "relastro.h"

int relastro_parallel_regions () {

  int status;
  FITS_DB db;

  INITTIME;

  // load the RegionTable (UserRegion should not be used at this level)
  RegionHostTable *regionHosts = RegionHostTableLoad (CATDIR, REGION_FILE);
  if (!regionHosts) {
    fprintf (stderr, "ERROR: problem with region host table\n");
    exit (2);
  }

  // register database handle with shutdown procedure 
  set_db (&db);
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);

  // lock and load the image db table
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status && UPDATE) {
    fprintf (stderr, "error\n");
    Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  }

  if ((db.dbstate == LCK_EMPTY) || (db.dbstate == LCK_MISSING)) {
    // if the Image.dat file is missing, db.dbstate will have a value of either:
    // LCK_EMPTY (if UPDATE) or LCK_MISSING (if !UPDATE)
    Shutdown ("ERROR: database %s contains no image data", CATDIR);
  }

  // read data from Image.dat file
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  MARKTIME("-- load image data: %f sec\n", dtime);

  /* assign the images to the different region hosts */
  if (!assign_images (&db, regionHosts)) Shutdown ("error assigning images to region hosts");
  MARKTIME("-- assign images: %f sec\n", dtime);

  /* launch processing on the parallel region hosts */
  if (!launch_region_hosts (regionHosts)) Shutdown ("error launching region hosts");

  // If we are doing FrameCorrection, we have to loop here waiting for ICRF objects from the region 
  // hosts.  we then measure the frame correction and send back the result
  if (!FrameCorrectionParallelMaster (regionHosts)) Shutdown ("error running parallel frame correction");

  // retrieve updated image parameters from the remote hosts (also set Image.mcal)
  if (!slurp_image_pos (NULL, 0, regionHosts, -1)) Shutdown ("error loading image updates");

  if (!UPDATE) { 
    dvo_image_unlock (&db); 
    MARKTIME ("finished relastro -parallel-regions: %f sec total\n", dtime);
    fprintf (stderr, "NOTE: UPDATE is OFF (results are not saved)\n");
    exit (0);
  }

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylist = SkyListByBounds (sky, -1, regionHosts->Rmin, regionHosts->Rmax, regionHosts->Dmin, regionHosts->Dmax);

  if (PARALLEL) {
    // save the updated image parameters.  if we are NOT PARALLEL, we have to wait
    // until after UpdateObjectOffsets, or the image parameters are swapped and/or freed
    save_astrom_table ();
    dvo_image_save (&db, VERBOSE);
    dvo_image_unlock (&db); 
  }

  // iterate over catalogs to make detection coordinates consistant
  if (APPLY_OFFSETS) {
    UpdateObjectOffsets (skylist, 0, NULL);
  }

  if (!PARALLEL) {
    // save the changes to the image parameters
    save_astrom_table ();
    dvo_image_save (&db, VERBOSE);
    dvo_image_unlock (&db); 
  }

// testjump:

  MARKTIME ("finished relastro -parallel-regions: %f sec total\n", dtime);
  
  freeImages(db.ftable.buffer);
  gfits_db_free (&db);
  freeMosaics ();

  FreeRegionHostTable (regionHosts);
  relastro_free (sky, skylist);

  exit (0);
}

