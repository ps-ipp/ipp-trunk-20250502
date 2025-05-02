# include "relphot.h"

// XXX there are two versions of this code:
// save_image_updates () is called by relphot_images
// save_image_backup () is called by relphot_parallel_regions

// rationalize these to a single function...

int save_images_updates (FITS_DB *db) {

  if (!SAVE_IMAGE_UPDATES) return TRUE;

  FITS_DB dbX;
  gfits_db_init (&dbX);
  dbX.lockstate = LCK_XCLD;
  dbX.timeout   = 60.0;
  dbX.mode      = db->mode;
  dbX.format    = db->format;

  char filename[1024];
  snprintf_nowarn (filename, 1024, "%s.bck", ImageCat); // ImageCat is global
  if (!gfits_db_lock (&dbX, filename)) {
    fprintf (stderr, "can't lock backup image image catalog\n");
    return FALSE;
  }
    
  // apply the changes from the image subset to the full table:

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer

  off_t Nimage, Nx;
  Image *image = gfits_table_get_Image (&db->ftable, &Nimage, &db->scaledValue, &db->nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }
  gfits_scan (db->ftable.header, "NAXIS1", OFF_T_FMT, 1,  &Nx);

  off_t *LineNumber;
  off_t Nsubset;
  Image *subset = getimages (&Nsubset, &LineNumber);
  for (off_t i = 0; i < Nsubset; i++) {
    if (LineNumber[i] == -1) continue;
    memcpy (&image[LineNumber[i]], &subset[i], Nx);
  }

  // copy Images.dat data (all or only the subset / vtable elements?)  I think I need to dump
  // the entire Image table, but with the updates in place I think this says: re-work the
  // ftable/vtable usage in this program to be more sensible...
  gfits_copy_header (&db->header,  &dbX.header);
  gfits_copy_matrix (&db->matrix,  &dbX.matrix);
  gfits_copy_header (&db->theader, &dbX.theader);
  gfits_copy_ftable (&db->ftable,  &dbX.ftable);

  dbX.ftable.header = &dbX.theader;
  dbX.virtual = FALSE;

  // save Images.dat using the copied structure
  if (UPDATE_CATFORMAT) {
    // ensure the db format is updated
    dbX.format = dvo_catalog_catformat (UPDATE_CATFORMAT);
    gfits_modify (&dbX.header, "FORMAT", "%s", 1, UPDATE_CATFORMAT);

    char photcodeFile[1024];
    sprintf (photcodeFile, "%s/Photcodes.dat", CATDIR);
    SavePhotcodesFITS (photcodeFile);
  }

  dvo_image_save (&dbX, VERBOSE);
  dvo_image_unlock (&dbX); 
  gfits_db_free (&dbX);
  return TRUE;
}

int save_images_backup (FITS_DB *db) {

  FITS_DB dbX;
  char filename[1024];

  gfits_db_init (&dbX);
  dbX.lockstate = LCK_XCLD;
  dbX.timeout   = 60.0;
  dbX.mode      = db->mode;
  dbX.format    = db->format;

  snprintf_nowarn (filename, 1024, "%s.bck", ImageCat);
  if (!gfits_db_lock (&dbX, filename)) {
    fprintf (stderr, "can't lock backup image image catalog\n");
    return (FALSE);
  }
    
  // copy Images.dat data (all or only the subset / vtable elements?)  I think I need to dump
  // the entire Image table, but with the updates in place I think this says: re-work the
  // ftable/vtable usage in this program to be more sensible...
  gfits_copy_header (&db->header,  &dbX.header);
  gfits_copy_matrix (&db->matrix,  &dbX.matrix);
  gfits_copy_header (&db->theader, &dbX.theader);
  gfits_copy_ftable (&db->ftable,  &dbX.ftable);

  dbX.ftable.header = &dbX.theader;

  dvo_image_save (&dbX, VERBOSE);
  dvo_image_unlock (&dbX); 

  gfits_db_free (&dbX);
  return TRUE;
}
