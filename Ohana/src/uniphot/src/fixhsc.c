# include "fixhsc.h"

int main (int argc, char **argv) {

  off_t Nimage;
  int status;
  FITS_DB db;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_fixhsc (argc, argv);

  set_db (&db);
  gfits_db_init (&db);
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);

  // load images
  Image *image  = load_images_fixhsc (&db, &Nimage);

  // need to update image table here:
  update_images_fixhsc (image, Nimage);

  // write image table (even if some remote clients failed)
  if (UPDATE) {
    SetProtect (TRUE);
    dvo_image_save (&db, VERBOSE);
    dvo_image_unlock (&db); 
    SetProtect (FALSE);
  }
  
  status = update_dvo_fixhsc ();

  if (!status) exit (1);
  exit (0);
}
  

/* fixhsc : modify the photcode values based on the time perido

   current HSC photcodes: 
     g = 20000 - 20111 
     r = 21000 - 21111

   goal HSC photcodes:
     g ,North = 20000 - 20111 
     g ,South = 20200 - 20311 
     r ,North = 21000 - 21111
     r ,South = 21200 - 21311
     r2,North = 21400 - 21511
     r2,South = 21600 - 21711

   I can define rules which specify the goal photcode for a time period,
   then the code can use the value % 1000 to get the chip ID

   rule file:
   start stop photcode (start & stop in MJD, photcode = value for chip 00, e.g., 21200)

   ** load images (save subset with imageID, time, photcode, seq)
   ** load catalogs 
   ** update detection
   ** count detections / image ID?
 */
