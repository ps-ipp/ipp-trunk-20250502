# include "relphot.h"

// apply the current calibration values to the measurements to determine the average
// magnitudes, applying the desired clipping analysis.  Note that this does not depend on
// the form of the image zero point measurement.  If there is a grid or map term, and this
// has been correctly propagated to the measurement, the average will respect that value.

int relphot_objects (SkyList *skylist, int hostID, char *hostpath) {

  FITS_DB db;

  INITTIME;

  set_db (&db);
  gfits_db_init (&db);

  /* lock and load the image db table */
  int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  MARKTIME("read image table: %f sec\n", dtime);
  // the raw FITS data is freed by dvo_image_load, leaving only the Image structure data

  /* load regions and images based on specified sky patch (default depth) */
  load_images (&db, skylist, &UserPatch, TRUE, USE_ALL_IMAGES);
  MARKTIME("loaded images: %f sec\n", dtime);

  // load grid corrections here (specified by -grid-meanfile)
  GridCorrectionLoad (GRID_MEANFILE);

  reload_catalogs (skylist, hostID, hostpath);

  freeImages(db.ftable.buffer);
  freeGridBins ();

  gfits_db_free (&db);

  return (TRUE);
}
