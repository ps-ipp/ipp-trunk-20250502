# include "fixhsc.h"

Image *load_images_fixhsc (FITS_DB *db, off_t *Nimage_load) {

  Image *image;

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* read entire db table */
  if (!dvo_image_load (db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db[0].filename);

  /* use a vtable to keep the images to be calibrated */
  image = gfits_table_get_Image (&db[0].ftable, Nimage_load, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  fprintf (stderr, "loaded "OFF_T_FMT" images\n", *Nimage_load);

  return (image);
}

void update_images_fixhsc (Image *image, off_t Nimage) {

  for (off_t i = 0; i < Nimage; i++) {

    // skip photcode == 0 images (PHU)
    short photcode = image[i].photcode;
    if (!photcode) continue;

    // we have a measure with a given photcode and time:
    e_time time = image[i].tzero;

    int newbase = get_rules_fixhsc (time);
    if (!newbase) continue;
    
    // the rule is the new base photcode
    // this is robust against multiple passes
    short chipcode = photcode % 200;

    short newcode  = newbase + chipcode;
    image[i].photcode = newcode;
  }
}

