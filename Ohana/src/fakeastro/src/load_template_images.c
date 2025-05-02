# include "fakeastro.h"

Image *load_template_images (int *nrefimage) {

  FITS_DB db;

  gfits_db_init (&db);
  db.mode    = dvo_catalog_catmode (CATMODE);
  db.format  = dvo_catalog_catformat (CATFORMAT);
  int status = dvo_image_lock (&db, IMAGES_INPUT, 60.0, LCK_SOFT);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  // load the image table 
  if (db.dbstate == LCK_EMPTY) {
    dvo_image_unlock (&db); // unlock input
    fprintf (stderr, "ERROR: no template images in %s\n", IMAGES_INPUT);
    exit (3);
  }

  // this operation reads the PHU header (db.header)
  if (!dvo_image_load (&db, VERBOSE, TRUE)) {
    Shutdown ("can't read input image catalog %s", db.filename);
  }
  dvo_image_unlock (&db); // unlock input

  off_t Nimage;
  Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: no template images in %s (though db exists)\n", IMAGES_INPUT);
    exit (3);
  }

  BuildChipMatch (image, Nimage);

  int NREFIMAGE = 1000;
  int Nrefimage = 0;
  Image *refimage;
  ALLOCATE (refimage, Image, NREFIMAGE);

  int i, j;
  for (i = 0; i < Nimage; i++) {

    // we only want to make fake images for the exposures
    if (strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    refimage[Nrefimage] = image[i];

    // find a corresponding chip image:
    Image *childImage = NULL;
    for (j = i + 1; j < i + 65; j++) {
      if (image[j].parent == &image[i]) {
	childImage = &image[j];
	break;
      }
    }
    if (!childImage) continue;

    // XXX hard=wire the photcode range for now (just i-band)
    if (childImage->photcode < 10200) continue;
    if (childImage->photcode > 10277) continue;

    refimage[Nrefimage].secz     = childImage->secz;
    refimage[Nrefimage].McalPSF  = childImage->McalPSF;
    refimage[Nrefimage].McalAPER = childImage->McalAPER;
    refimage[Nrefimage].dMcal    = childImage->dMcal;
    refimage[Nrefimage].exptime  = childImage->exptime;
    refimage[Nrefimage].sidtime  = childImage->sidtime;
    refimage[Nrefimage].latitude = childImage->latitude;
    refimage[Nrefimage].fwhm_x   = childImage->fwhm_x;
    refimage[Nrefimage].fwhm_y   = childImage->fwhm_y;

    // this is a bit of a hack, but not too bad (very gpc1 specific):
    refimage[Nrefimage].photcode = 100 * (int) (childImage->photcode / 100);

    Nrefimage ++;
    CHECK_REALLOCATE (refimage, Image, NREFIMAGE, Nrefimage, 1000);
  }

  // free the input image database
  gfits_db_free (&db);

  *nrefimage = Nrefimage;
  return refimage;
}
