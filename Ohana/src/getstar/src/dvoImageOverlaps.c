# include "dvoImageOverlaps.h"

int main (int argc, char **argv) {

  off_t NdbImages, Nimages;
  int status;
  off_t i, Nmatches, *matches;
  Image *images, *dbImages;
  FITS_DB db;

  SetSignals ();
  ConfigInit_overlaps (&argc, argv);
  args_overlaps (argc, argv);
  
  images = ReadImageFiles (argv[1], &Nimages); 

  /*** update the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    fprintf (stderr, "no images in database (%s)\n", ImageCat);
    exit (1);
  } else {
    if (!dvo_image_load (&db, VERBOSE, FALSE)) {
      Shutdown ("can't read image catalog %s", db.filename);
    }
  }
  dvo_image_unlock (&db);

  // convert database table to internal structure
  dbImages = gfits_table_get_Image (&db.ftable, &NdbImages, &db.scaledValue, &db.nativeOrder);
  if (!dbImages) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }
  
  // for (i = 1; i < 2; i++) {
  for (i = 0; i < Nimages; i++) {
    matches = MatchImage (dbImages, NdbImages, &images[i], &Nmatches);
    ListImageOverlaps (dbImages, &images[i], matches, Nmatches);
  }

  exit (0);
}
