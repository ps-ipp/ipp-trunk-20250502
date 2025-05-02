# include "dvomerge.h"

/*** generate the IDmap for the given databases, and update the output image table ***/
int dvomergeImagesUpdate (IDmapType *IDmap, char *input, char *output) { 

  FITS_DB inDB;
  FITS_DB outDB;
  int    status;

  /*** load input1/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", input);
  gfits_db_init (&inDB);
  inDB.mode   = dvo_catalog_catmode (CATMODE);
  inDB.format = dvo_catalog_catformat (CATFORMAT);
  status      = dvo_image_lock (&inDB, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", inDB.filename);

  // load the image table 
  if (inDB.dbstate == LCK_EMPTY) {
    dvo_image_unlock (&inDB); // unlock input
    IDmap->old = NULL;
    IDmap->new = NULL;
    IDmap->Nmap = 0;
    return TRUE;
  }
  // this operation reads the PHU header (inDB.header)
  if (!dvo_image_load (&inDB, VERBOSE, TRUE)) {
    Shutdown ("can't read input image catalog %s", inDB.filename);
  }
  dvo_image_unlock (&inDB); // unlock input

  // read the header for the database ID?
  char *indbID = dmhImageReadID (&inDB);
  if (!indbID) { 
    Shutdown ("this database is missing a DVO database ID; please generate one before merging\n"); 
  } 

  /*** load output/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", output);
  gfits_db_init (&outDB);
  outDB.mode   = dvo_catalog_catmode (CATMODE);
  outDB.format = dvo_catalog_catformat (CATFORMAT);
  status       = dvo_image_lock (&outDB, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", outDB.filename);

  /* load the image table */
  if (outDB.dbstate == LCK_EMPTY) {
    dvo_image_create (&outDB, GetZeroPoint());
  } else {
    if (!dvo_image_load (&outDB, VERBOSE, TRUE)) {
      Shutdown ("can't read output image catalog %s", outDB.filename);
    }
  }

  dmhImage *history = dmhImageRead (&outDB);
  if (!history) { 
    Shutdown ("error reading history for output database\n"); 
  }

  // have we already merged this database?
  if (dmhImageCheck(history, indbID)) {
    // if so, then just match the image IDs 
    if (VERBOSE) fprintf (stderr, "already merged image table\n");
    if (MATCH_BY_EXTERN_ID) {
      dvo_image_match_dbs_by_extern_id(IDmap, &outDB, &inDB);
    } else {
      dvo_image_match_dbs_by_time_and_photcode(IDmap, &outDB, &inDB);
    }
    dvo_image_unlock (&outDB); // unlock output

    gfits_db_free (&inDB);
    gfits_db_free (&outDB);
    dmhImageFree (history);
    FREE (indbID);
    return TRUE;
  }

  // convert database table to internal structure & add to output image db
  dvo_image_merge_dbs(IDmap, &outDB, &inDB);

  // add the new image db to merge history
  // (updates header as well as history structure
  if (!dmhImageAdd (&outDB, history, indbID)) { 
    Shutdown ("error reading history for output database\n"); 
  }
    
  SetProtect (TRUE);
  dvo_image_save (&outDB, VERBOSE);
  SetProtect (FALSE);
  dvo_image_unlock (&outDB); // unlock output

  gfits_db_free (&inDB);
  gfits_db_free (&outDB);

  dmhImageFree (history);
  FREE (indbID);

  return TRUE;
}

/*** generate the map for the given 2 databases ***/
int dvomergeImagesGetMap (IDmapType *IDmap, char *input, char *output) { 

  FITS_DB inDB;
  FITS_DB outDB;
  int    status;

  if (VERIFY && VERIFY_CATALOG_ONLY) return TRUE;

  /*** load input1/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", input);
  gfits_db_init (&inDB);
  inDB.mode   = dvo_catalog_catmode (CATMODE);
  inDB.format = dvo_catalog_catformat (CATFORMAT);
  status      = dvo_image_lock (&inDB, ImageCat, 5400.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", inDB.filename);

  // load the image table
  if (inDB.dbstate == LCK_EMPTY) {
    dvo_image_unlock (&inDB); // unlock input
    IDmap->old = NULL;
    IDmap->new = NULL;
    IDmap->Nmap = 0;
    return TRUE;
  }

  // the input database is allowed to have no images 
  if (!dvo_image_load (&inDB, VERBOSE, TRUE)) {
    Shutdown ("can't read input image catalog %s", inDB.filename);
  }
  dvo_image_unlock (&inDB); // unlock input

  /*** load output/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", output);
  gfits_db_init (&outDB);
  outDB.mode   = dvo_catalog_catmode (CATMODE);
  outDB.format = dvo_catalog_catformat (CATFORMAT);
  status       = dvo_image_lock (&outDB, ImageCat, 5400.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", outDB.filename);

  /* load the image table */
  if (outDB.dbstate == LCK_EMPTY) {
    Shutdown ("for dvomerge -continue or dvomerge_client, the output database must exist");
  } 
  if (!dvo_image_load (&outDB, VERBOSE, TRUE)) {
    Shutdown ("can't read output image catalog %s", outDB.filename);
  }
  dvo_image_unlock (&outDB); // unlock output

  // convert database table to internal structure & add to output image db
  if (MATCH_BY_EXTERN_ID) {
    dvo_image_match_dbs_by_extern_id(IDmap, &outDB, &inDB);
  } else {
    dvo_image_match_dbs_by_time_and_photcode(IDmap, &outDB, &inDB);
  }
  // dvo_image_match_dbs(IDmap, &outDB, &inDB);

  gfits_db_free (&inDB);
  gfits_db_free (&outDB);

  return TRUE;
}
