# include "uniphot.h"

int subset_images (FITS_DB *db) {

  off_t i, Nimage;
  int equiv;
  Image *image;

  // convert from the binary I/O format to the internal structure (Image)
  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  /* mark images to be calibrated */
  for (i = 0; i < Nimage; i++) {
      
    image[i].flags |= ID_IMAGE_PHOTOM_NOCAL;

    /* select images by photcode */
    equiv = GetPhotcodeEquivCodebyCode (image[i].photcode);
    if (equiv != photcode[0].code) continue;

    /* select images by time */
    if (TimeSelect) {
      if (image[i].tzero < TSTART) continue;
      if (image[i].tzero > TSTOP) continue;
    }
    image[i].flags &= ~ID_IMAGE_PHOTOM_NOCAL;
  }
  return (TRUE);
}
