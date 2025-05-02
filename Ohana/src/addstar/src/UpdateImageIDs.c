# include "addstar.h"

int UpdateImageIDs (Catalog *catalog, Image *images, off_t Nimages) {

  int i, status, isEmpty;
  unsigned int imageID;
  FITS_DB db;

  /*** update the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    if (VERBOSE) fprintf (stderr, "can't find %s, creating a new one\n", ImageCat);
    dvo_image_create (&db, GetZeroPoint());
    ImageIndexFileInit ();
    isEmpty = TRUE;
  } else {
    /* position to start of file */
    Fseek (db.f, 0, SEEK_SET);
    if (!gfits_fread_header (db.f, &db.header)) {
      Shutdown ("can't read image table phu %s", db.filename);
    }
    isEmpty = FALSE;
  }

  // note that imageID is unsigned int
  status = gfits_scan (&db.header, "IMAGEID", "%u", 1, &imageID);
  if (!status) {
    status = gfits_scan (&db.header, "NIMAGES", "%u", 1, &imageID);
  }

  // TEST IMAGE_ID OVER-RIDE:
  if (IMAGE_ID_OVERRIDE) {
    imageID = IMAGE_ID_OVERRIDE;
  }

  // XXX should the first image ID be 1, not 0?
  if (imageID == 0) imageID = 1;

  // update the image IDs already written to stars and images:
  for (i = 0; i < Nimages; i++) {
    images[i].imageID += imageID;
    if (images[i].parentID == UINT32_MAX) {
        images[i].parentID = 0;
    } else {
        images[i].parentID += imageID;
    }
  }

  // why is this not just a loop over catalog->Nmeasure and catalog->Nlensing??
  off_t j, m;
  for (i = 0; i < catalog->Naverage; i++) {
    m = catalog->average[i].measureOffset;
    for (j = 0; j < catalog->average[i].Nmeasure; j++, m++) {
      catalog->measure[m].imageID += imageID;
    }
    m = catalog->average[i].lensingOffset;
    for (j = 0; j < catalog->average[i].Nlensing; j++, m++) {
      catalog->lensing[m].imageID += imageID;
    }
  }

  // set and update the imageID sequence
  // the file holding the index is created above if this is an empty db
  if (NO_DUPLICATE_IMAGES) {
    CheckDuplicateImageIDs (images, Nimages);
  }

  imageID += Nimages;
  status = gfits_modify (&db.header, "IMAGEID", "%u", 1, imageID);

  if (isEmpty) {
    if (!dvo_image_addrows (&db, NULL, 0)) Shutdown ("failed to create image table");
    SetProtect (TRUE);
    if (!dvo_image_update (&db, VERBOSE)) Shutdown ("failed to update image table");
    SetProtect (FALSE);
  } else {
    // write just the PHU
    // position to start of file
    Fseek (db.f, 0, SEEK_SET);
    SetProtect (TRUE);
    if (!gfits_fwrite_header (db.f, &db.header)) {
      Shutdown ("can't write image table phu %s", db.filename);
    }
    SetProtect (FALSE);
  }

  dvo_image_unlock (&db);
  gfits_db_free (&db);

  return TRUE;
}
