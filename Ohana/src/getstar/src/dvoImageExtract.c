# include "dvoImageExtract.h"

int main (int argc, char **argv) {

  off_t Nimages, Nmatches, *matches;
  int status;
  Image *images;
  FITS_DB db;

  SetSignals ();
  ConfigInit_extract (&argc, argv);
  args_extract (argc, argv);

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
  images = gfits_table_get_Image (&db.ftable, &Nimages, &db.scaledValue, &db.nativeOrder);
  if (!images) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }
  
  matches = SelectImages (argv[1], images, Nimages, &Nmatches);
  WriteImages (OUTFILE, images, Nimages, matches, Nmatches);

  exit (0);
}

/* This program extracts images headers from a DVO database and writes them 
   in cmf format (headers + object tables).

   If multiple images are selected, they must be from the same exposures (how?).

   If images are selected with WRP astrometry, the associated DIS image header is first written to
   the output file.
 
   USAGE: dvoExtractImages (imageID[s]) -o output.cmf
*/
