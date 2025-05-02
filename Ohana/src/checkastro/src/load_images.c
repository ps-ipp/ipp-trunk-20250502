# include "checkastro.h"

int load_images (FITS_DB *db, SkyList *skylist) {

  Image     *image, *subset;
  off_t      Nimage, Nsubset;
  off_t     *LineNumber;

  INITTIME;

  // convert database table to internal structure
  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  MARKTIME("  convert image table: %f sec\n", dtime);

  BuildChipMatch (image, Nimage);

  // select the images which overlap the selected sky regions
  subset = select_images (skylist, image, Nimage, &LineNumber, &Nsubset);
  MARKTIME("  select images: %f sec\n", dtime);

  fprintf (stderr, "%llx\n", (long long) subset);

  initImages (subset, LineNumber, Nsubset);
  MARKTIME("  init images: %f sec\n", dtime);

  // unlock, if we can (else, unlocked below)
  int unlockImages = TRUE;
  if (unlockImages) dvo_image_unlock (db); 

  return TRUE;
}

