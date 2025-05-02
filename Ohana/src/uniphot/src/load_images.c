# include "uniphot.h"

int load_images_uniphot (FITS_DB *db) {

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* read entire db table */
  if (!dvo_image_load (db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db[0].filename);

  return (TRUE);
}

Image *load_images_setfwhm (FITS_DB *db, off_t *Nimage) {

  Image *image;

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* read entire db table */
  if (!dvo_image_load (db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db[0].filename);

  /* use a vtable to keep the images to be calibrated */
  image = gfits_table_get_Image (&db[0].ftable, Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  fprintf (stderr, "loaded "OFF_T_FMT" images\n", *Nimage);

  return (image);
}
