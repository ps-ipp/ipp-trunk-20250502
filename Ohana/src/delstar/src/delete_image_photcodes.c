# include "delstar.h"

int delete_image_photcodes (FITS_DB *db) {

  off_t i, j;
  off_t  Nimage;
  Image *image;
  Image *outimage;

  // xxx where does this go?
  int Nphotcodes = 0;
  PhotCode **photcodes = ParsePhotcodeList (PHOTCODE_LIST, &Nphotcodes, FALSE);

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  /* delete the identified images */
  off_t Noutimage = 0;
  ALLOCATE (outimage, Image, Nimage);
  for (i = 0; i < Nimage; i++) {
    int drop = FALSE;
    for (j = 0; !drop && (j < Nphotcodes); j++) {
      drop |= (photcodes[j][0].code == image[i].photcode);
    }
    if (drop) continue;
    outimage[Noutimage] = image[i];
    Noutimage ++;
  }
  free (image);
  
  if (VERBOSE) fprintf (stderr, "removing "OFF_T_FMT" images (leaving "OFF_T_FMT" of "OFF_T_FMT")\n",  Nimage - Noutimage, Noutimage, Nimage);
  // gfits_table_set_Image (&db[0].ftable, outimage, Noutimage, TRUE);

  gfits_modify (&db[0].theader, "NAXIS2", OFF_T_FMT, 1,  Noutimage);
  gfits_modify (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  Noutimage);
  db[0].theader.Naxis[1] = Noutimage;
  db[0].ftable.buffer = (char *) outimage;

  if (!dvo_image_save (db, VERBOSE)) return FALSE;
  if (!dvo_image_unlock (db)) return FALSE;

  return TRUE;
}
