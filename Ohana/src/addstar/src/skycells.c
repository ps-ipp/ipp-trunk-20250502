# include "skycells.h"

int main (int argc, char **argv) {

  int status;
  FITS_DB db;

  SetSignals ();
  ConfigInit_skycells (&argc, argv);
  args_skycells (argc, argv);

  /*** update the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    if (VERBOSE) fprintf (stderr, "can't find %s, creating a new one\n", ImageCat);
    dvo_image_create (&db, 25.0);
  } else {
    if (!dvo_image_load (&db, VERBOSE, FALSE)) {
      Shutdown ("can't read image catalog %s", db.filename);
    }
  }

  // we have to put the database update calls deep down in the sky_tessellation code so we
  // can write out the skycells in limited-sized chunks.
  sky_tessellation (&db, LEVEL, NMAX, MODE, SCALE);

  dvo_image_unlock (&db);
  exit (0);
}

/*
 * - we start with the image database loaded into memory (db)
 * - we convert this to an empty vtable 
 * - add groups of images to the vtable
 * - we write out the vtable data
 * - zero-out the saved entries, 
 */

