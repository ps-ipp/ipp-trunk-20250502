# include "dvomerge.h"

Image *LoadImages (FITS_DB *db, char *filename, off_t *Nimage) {

  int status;
  Image *image;
  double ZeroPoint;
  
  myAssert(filename, "programming errror");
  myAssert(Nimage, "programming errror");
    
  gfits_db_init (db);
  db->lockstate = LCK_SOFT;
  db->timeout   = 120.0;

  if (!gfits_db_lock (db, filename)) {
    fprintf (stderr, "error opening image catalog %s (1)\n", filename);
    return (NULL);
  }

  if (db->dbstate == LCK_EMPTY) {
    fprintf (stderr, "note: image catalog is empty\n");
    ALLOCATE (image, Image, 1);
    *Nimage = 1;
    return (image);
  }

  status = dvo_image_load (db, TRUE, FALSE);
  gfits_db_close (db);

  if (!status) {
    fprintf (stderr, "problem loading image database table\n");
    return (NULL);
  }

  image = gfits_table_get_Image (&db->ftable, Nimage, &db->scaledValue, &db->nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    return (NULL);
  }

  gfits_scan (&db->header, "ZERO_PT", "%lf", 1, &ZeroPoint);
  SetZeroPoint (ZeroPoint);

  return (image);
}
