# include "checkastro.h"

int checkastro_images (SkyList *skylist) {

  int status, Ncatalog;
  Catalog *catalog;
  FITS_DB db;

  INITTIME;

  /* register database handle with shutdown procedure */
  set_db (&db);
  gfits_db_init (&db);

  /* lock and load the image db table */
  status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  MARKTIME("load image data: %f sec\n", dtime);

  /* load regions and images based on specified sky patch (default depth) (require full overlap) */
  load_images (&db, skylist);
  MARKTIME("load images: %f sec\n", dtime);

  // photcodesKeep is used here to allow measurements from the images being calibrated
  // note if -reset-to-photcode is selected, photocodesKeep is replaced with below with photcodesReset
  catalog = load_catalogs (skylist, &Ncatalog, TRUE, 0, NULL);
  MARKTIME("load catalog data: %f sec\n", dtime);

  if (photcodesReset) {
    photcodesKeep  = photcodesReset;
    NphotcodesKeep = NphotcodesReset;
  }

  if (Ncatalog == 0) {
    fprintf (stderr, "ERROR: no valid data for checkastro, exiting\n");
    exit (2);
  }

  /* match measurements with images */
  initImageBins (catalog, Ncatalog);
  MARKTIME("make image bins: %f sec\n", dtime);

  findImages (catalog, Ncatalog);
  MARKTIME("set up image indexes: %f sec\n", dtime);

  exit (0);
}
