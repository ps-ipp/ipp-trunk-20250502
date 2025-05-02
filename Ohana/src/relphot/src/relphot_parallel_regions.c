# include "relphot.h"

int relphot_parallel_regions (SkyTable *sky) {

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

  // save Images.dat using the copied structure
  if (UPDATE_CATFORMAT) {
    // ensure the db format is updated
    db.format = dvo_catalog_catformat (UPDATE_CATFORMAT);
    gfits_modify (&db.header, "FORMAT", "%s", 1, UPDATE_CATFORMAT);
  }

  /* assign the images to the different region hosts */
  if (!assign_images (&db, regionHosts)) Shutdown ("error assigning images to region hosts");
  MARKTIME("-- assign images: %f sec\n", dtime);

  /* launch processing on the parallel region hosts */
  if (!launch_region_hosts (regionHosts)) Shutdown ("error launching region hosts");
  MARKTIME ("finished relphot -parallel-regions: %f sec total\n", dtime);

  // retrieve updated image parameters from the remote hosts (also set Image.mcal)
  if (!slurp_image_mags (regionHosts, -1)) Shutdown ("error loading image updates");

  if (!UPDATE) { 
    dvo_image_unlock (&db); 
    MARKTIME ("finished relphot -parallel-regions: %f sec total\n", dtime);
    fprintf (stderr, "NOTE: UPDATE is OFF (results are not saved)\n");
    exit (0);
  }

  SkyList *skylist = SkyListByBounds (sky, -1, regionHosts->Rmin, regionHosts->Rmax, regionHosts->Dmin, regionHosts->Dmax);
  UserPatch.Rmin = regionHosts->Rmin;
  UserPatch.Rmax = regionHosts->Rmax;
  UserPatch.Dmin = regionHosts->Dmin;
  UserPatch.Dmax = regionHosts->Dmax;

  // I have to save a copy because dvo_image_save and _unlock swap and free the data
  save_images_backup (&db);

  /* update catalogs (in parallel) */
  reload_catalogs (skylist, 0, NULL);

  // save the changes to the image parameters
  dvo_image_save (&db, VERBOSE);

  // dvo_image_save frees db.ftable.buffer (== image) and replaces it: do not free stored Image table
  clearImages ();

  dvo_image_unlock (&db); 
  MARKTIME ("finished relphot -parallel-regions: %f sec total\n", dtime);

  freeImages(db.ftable.buffer);
  gfits_db_free (&db);

  SkyListFree(skylist);
  FreeRegionHostTable (regionHosts);

  return TRUE;
}
