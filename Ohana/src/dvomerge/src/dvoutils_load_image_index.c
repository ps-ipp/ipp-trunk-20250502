# include "dvoutils.h"

// load the array of EXTERN_ID from the ImageIndex.fits tables
ImageData *dvoutils_load_image_index (char *filename) {

  int Ncol;
  off_t Nrow, Nphot;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;
  char type[16];

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image index file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image index header\n");
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image index matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return NULL;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return NULL;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return (NULL);
  }
  fclose (f);

  ImageData *imdata = NULL;
  ALLOCATE (imdata, ImageData, 1);

  imdata->externID = gfits_get_bintable_column_data (&theader, &ftable, "EXTERN_ID", type, &Nrow, &Ncol);
  myAssert (!strcmp(type, "int"), "wrong column type");

  imdata->photcode = gfits_get_bintable_column_data (&theader, &ftable, "PHOTCODE", type, &Nphot, &Ncol);
  myAssert (Nphot == Nrow, "photcode & extern_id mis-match");

  imdata->Nimages = Nrow;

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  return imdata;
}

Image *dvoutils_load_image_table (char *filename, int *nimage) {

  FITS_DB db;

  gfits_db_init (&db);
  db.lockstate = LCK_SOFT;
  db.timeout   = 120.0;

  if (!gfits_db_lock (&db, filename)) {
    fprintf (stderr, "error opening image catalog %s (1)\n", filename);
    exit (3);
  }

  if (db.dbstate == LCK_EMPTY) {
    fprintf (stderr, "note: image catalog is empty\n");
    gfits_db_free  (&db);
    return NULL;
  }

  int status = dvo_image_load (&db, TRUE, FALSE);
  gfits_db_close (&db);
    
  if (!status) {
    fprintf (stderr, "problem loading image database table\n");
    exit (4);
  }

  off_t Nimage;
  Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (5);
  }

  *nimage = Nimage;
  return image;
}
