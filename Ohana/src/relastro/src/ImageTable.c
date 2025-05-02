# include "relastro.h"

Image *ImageTableLoad(char *filename, off_t *nimage) {

  int status;
  off_t Nimage;
  FITS_DB db;

  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  
  /* lock and load the image db table */
  status = dvo_image_lock (&db, filename, 60.0, LCK_SOFT);
  if (!status) {
    Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  }

  // if the file is missing, db.dbstate will have a value of either:
  // LCK_EMPTY (if UPDATE) or LCK_MISSING (if !UPDATE)
  if ((db.dbstate == LCK_EMPTY) || (db.dbstate == LCK_MISSING)) {
    Shutdown ("ERROR: database %s contains no image data", CATDIR);
  }

  // read data from Image.dat file
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    Shutdown ("ERROR: failed to read images");
  }

  // gfits_db_free (&db);
  gfits_free_header (&db.header);
  gfits_free_matrix (&db.matrix);
  gfits_free_header (&db.theader);
  free (db.filename);

  *nimage = Nimage;
  return image;
}

int ImageTableSave (char *filename, Image *images, off_t Nimages) {

  int status;
  FITS_DB db;

  // setup image table format and lock 
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, filename, 60.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  // load or create the image table 
  if (db.dbstate != LCK_EMPTY) {
    Shutdown ("image catalog already exists %s", db.filename);
  }

  dvo_image_create (&db, GetZeroPoint());

  /* add the new images and save */
  dvo_image_addrows (&db, images, Nimages);
  dvo_image_update (&db, VERBOSE);
  dvo_image_unlock (&db);

  gfits_db_free (&db);

  return TRUE;
}
