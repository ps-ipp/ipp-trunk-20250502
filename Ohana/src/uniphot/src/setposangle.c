# include "setposangle.h"

int main (int argc, char **argv) {

  off_t Nimage;
  int status;
  FITS_DB db;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_setposangle (argc, argv);

  set_db (&db);
  gfits_db_init (&db);
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);

  // load images (images are not modified, but we need the astrometry transformations
  Image *image  = load_images_setposangle (&db, &Nimage);
  dvo_image_unlock (&db); 
  
  // the imageSubset here is a reduced set of fields, not a reduced set of images
  ImageSubset *subset = ImagesToSubset (image, Nimage);

  status = update_dvo_setposangle (subset, Nimage);

  if (!status) exit (1);
  exit (0);
}
  

/* setposangle : set the posangle & pltscale for measurements in the db

   ** load images
   ** load catalogs
   ** update detection (& averages?)
 */
