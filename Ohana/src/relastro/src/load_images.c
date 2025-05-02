# include "relastro.h"

static AstromOffsetTable *table = NULL;

/* AstromOffsetMap is the map of Astrometry Offsets for non-polynomial astrometric
 * corrections for each chip.  
 */

int load_images (FITS_DB *db, SkyList *skylist, int UseFullOverlap, int UseAllImages) {

  Image     *image, *subset;
  off_t      Nimage, Nsubset;
  off_t     *LineNumber, i;

  INITTIME;

  // convert database table to internal structure
  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  MARKTIME("  convert image table: %f sec\n", dtime);

  // assign image->parent and image->coords.mosaic 
  BuildChipMatch (image, Nimage);
  MARKTIME("build chip match: %f sec\n", dtime);

  // select the images which overlap the selected sky regions
  if (UseAllImages) {
    ALLOCATE (LineNumber, off_t, Nimage);
    for (i = 0; i < Nimage; i++) LineNumber[i] = i;
    subset  = image;
    Nsubset = Nimage;
  } else {
    subset = select_images (skylist, image, Nimage, &LineNumber, &Nsubset, UseFullOverlap);
    MARKTIME("  select images: %f sec\n", dtime);

    if (Nsubset == Nimage) {
      free (subset);
      subset = image;
    }
  }
    
  /* unlock, if we can (else, unlocked below) and free, if we can */
  int unlockImages = !UPDATE || (RELASTRO_OP == OP_UPDATE_OFFSETS) || (RELASTRO_OP == OP_REPAIR_WARPS);
  if (unlockImages) { 
    if (subset != image) {
      // if we have generated an image subset and we are running UPDATE_OFFSETS, the we can free images here
      free (image);
      db[0].ftable.buffer = NULL;
      BuildChipMatch (subset, Nsubset);
    }
    dvo_image_unlock (db); 
  }

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  table = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (subset, Nsubset, table);
  } else {
    table = AstromOffsetTableInit ();
  }

  initImages (subset, LineNumber, Nsubset, !USE_ALL_IMAGES);
  MARKTIME("  init images: %f sec\n", dtime);
  
  initMosaics (subset, Nsubset);
  MARKTIME("  init mosaics: %f sec\n", dtime);
  
  return TRUE;
}

int reload_images (FITS_DB *db) {

  Image     *image;
  off_t     Nimage, Nx, i, *LineNumber;
  VTable    *vtable;

  image = getimages (&Nimage, &LineNumber);

  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, LineNumber, Nimage);
  vtable = &db[0].vtable;

  gfits_scan (vtable[0].header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  for (i = 0; i < Nimage; i++) {
    memcpy (vtable[0].buffer[i], &image[i], Nx);
  }
  return TRUE;
}

int save_astrom_table () {

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  AstromOffsetMapSave (table, mapfile);

  return TRUE;
}

AstromOffsetTable *get_astrom_table () {
  return table;
}

void put_astrom_table (AstromOffsetTable *myTable) {
  table = myTable;
}

void free_astrom_table () {
  if (!table) return;
  AstromOffsetTableFree (table);
  free (table);
}
