# include "addstar.h"

// If this function is unable to open the image table, it creates a new one
// set the database ID in the Image.dat header
int addstar_create_ID () {

  int status;
  FITS_DB db;

  /*** update the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 2.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    if (VERBOSE) fprintf (stderr, "can't find %s, creating a new one\n", ImageCat);
    dvo_image_create (&db, GetZeroPoint());
    // if (!gfits_table_to_vtable (&db[0].ftable, &db[0].vtable, 0, 0)) return (FALSE);
    dvo_image_save (&db, VERBOSE);
    dvo_image_unlock (&db);
    return TRUE;
  } 

  if (!dvo_image_load (&db, VERBOSE, FORCE_READ)) {
    Shutdown ("can't read image catalog %s", db.filename);
  }
  if (!dvo_image_createID (&db.header)) {
    fprintf (stderr, "failed to add database ID\n");
    exit (1);
  }
  fseeko (db.f, 0, SEEK_SET);

  status = fwrite (db.header.buffer, 1, db.header.datasize, db.f);
  if (status != db.header.datasize) {
    fprintf (stderr, "failed to write data to image header\n");
    exit (1);
  }

  dvo_image_unlock (&db);

  return TRUE;
}

// NOTE: Photcodes (and ZERO_POINT) and ImageCat are set in ConfigInit.c
