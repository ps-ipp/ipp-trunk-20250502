# include "uniphot.h"

int main (int argc, char **argv) {

  off_t Nimage;
  int status, Nfwhm;
  FITS_DB db;
  FWHMTable *fwhm;
  Image *image;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_setfwhm (argc, argv);

  set_db (&db);
  gfits_db_init (&db);
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);

  fwhm = load_fwhm_table (argv[1], &Nfwhm);

  // load images
  image  = load_images_setfwhm (&db, &Nimage);
  if (!UPDATE) dvo_image_unlock (&db); 
  
  // apply the newly loaded fwhm values to the corresponding images
  match_fwhm_to_images (image, Nimage, fwhm, Nfwhm);

  // write image table
  if (UPDATE) {
    dvo_image_save (&db, VERBOSE);
  }

  exit (0);
}
  

/* setphot : set the zero points for images in the db (perhaps based on external information)
   setphot (zpt_table)

 * load text table of zpts, time (photcode?)
 * load images
 * match images to zpts
 * set zpts
 * load catalogs
 * update detection (& averages?)

 */

