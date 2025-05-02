# include "relphot.h"

static AstromOffsetTable *table = NULL;

// This function generates a subset of the images based on selections.  Input db has already
// been loaded with the raw fits table data
int load_images (FITS_DB *db, SkyList *skylist, SkyRegion *region, int unlockImages, int UseAllImages) {

  Image     *image, *subset;
  off_t      Nimage, Nsubset;
  off_t     *LineNumber;
  char      *inSubset;

  INITTIME;

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  MARKTIME("read image table: %f sec\n", dtime);

  BuildChipMatch (image, Nimage);
  MARKTIME("build chip match for %d images: %f sec\n", (int) Nimage, dtime);

  // allocate and init an array to identify the images included in the subset
  ALLOCATE (inSubset, char, Nimage);
  memset (inSubset, 0, Nimage);

  // select the images which overlap the selected sky regions
  // 'subset' points to a new copy of the data (different from 'image') if a subset is selected
  if (UseAllImages) {
    ALLOCATE (LineNumber, off_t, Nimage);
    for (off_t i = 0; i < Nimage; i++) {
      LineNumber[i] = i;
      inSubset[i] = TRUE;
    }
    subset  = image;
    Nsubset = Nimage;
  } else {
    subset = select_images (skylist, image, Nimage, inSubset, &LineNumber, &Nsubset, region);
    MARKTIME("selected %d overlapping images: %f sec\n", (int) Nsubset, dtime);

    if (Nimage == 0) {
      fprintf (stderr, "no images selected for analysis : problem with region or photcode?\n");
      exit (4);
    }

    if (Nsubset == Nimage) {
      free (subset);
      subset = image;
    }
  }

  // reset image values as needed.  always allow 'few' images to succeed, if possible (new
  // images / detections may have been added
  ResetImages (subset, Nsubset);

  // XXX consider allowing relphot to skip the AstroMap load (do we need precise astrometry here?)

  // NOTE: unlock & free when running relphot_client (update-objects), but not when running relphot_images
  // note that if we have MOSAIC_ZEROPT selected, we cannot free images as the full array is needed by initMosaic

  /* unlock, if we can (else, unlocked below) and free, if we can */
  if (unlockImages && !MOSAIC_ZEROPT) { 
    if (subset != image) {
      // if we have generated an image subset and we are running UPDATE_OFFSETS, the we can free images here
      free (image);
      db[0].ftable.buffer = NULL;
      BuildChipMatch (subset, Nsubset);
    }
    dvo_image_unlock (db); 
    // allocate and init an array to identify the images included in the subset
  }

  char mapfile[DVO_MAX_PATH];
  snprintf (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  table = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (subset, Nsubset, table);
  }

  // match chips to mosaics (if applicable)
  initMosaics (subset, Nsubset, image, inSubset, Nimage);
  MARKTIME("init mosaics: %f sec\n", dtime);
  free (inSubset);
  
  // save the subset of images in the static reference in ImageOps, set up indexes
  initImages (subset, LineNumber, Nsubset);
  MARKTIME("init images: %f sec\n", dtime);

  // assign chip images to tgroups
  initTGroups (subset, Nsubset);

  return TRUE;
}

// This function re-creates the image subset array using the array of LineNumbers saved
// initially by load_images.  This function is only called by relphot_images.c after the
// loops have been completed (if any).  I am not certain this function is doing anything
// at all : the values in the image subset are copied back to the master table in
// save_images_updates() so the values copied from the master to the subset should already
// be there.

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
  return (TRUE);
}

void put_astrom_table (AstromOffsetTable *myTable) {
  table = myTable;
}

void free_astrom_table () {
  if (!table) return;
  AstromOffsetTableFree (table);
  free (table);
}



// *** old example code ***
  // determine the populated SkyRegions overlapping the requested area
# if (0)    
  // XXX note : this chunk selects catalogs for a region across the 0,360 boundary (e.g., 350 10)
  // it has largely been obviated, but I'm keeping the code in case I need to reinstate it.
  if (region[0].Rmin > region[0].Rmax) {
    SkyRegion subregion;
    subregion = *region;
    subregion.Rmin = 0.0; subregion.Rmax = region[0].Rmax;
    skylist = SkyListByPatch (sky, -1, &subregion);

    // select the images which overlap the selected sky regions
    // 'subset' points to a new copy of the data (different from 'image')
    subset = select_images (skylist, image, Nimage, inSubset, &LineNumber, &Nsubset, &subregion);
    MARKTIME("selected %d overlapping images: %f sec\n", (int) Nsubset, dtime);

    Image *subsetExtra;
    off_t NsubsetExtra;
    off_t *LineNumberExtra, i;

    subregion.Rmin = region[0].Rmin; subregion.Rmax = 360.0; 
    SkyList *extraList = SkyListByPatch (sky, -1, &subregion);

    subsetExtra = select_images (extraList, image, Nimage, inSubset, &LineNumberExtra, &NsubsetExtra, &subregion);
    MARKTIME("selected %d overlapping images: %f sec\n", (int) Nsubset, dtime);

    REALLOCATE (subset, Image, MAX (Nsubset + NsubsetExtra, 1));
    REALLOCATE (LineNumber, off_t, MAX (Nsubset + NsubsetExtra, 1));
      
    for (i = 0; i < NsubsetExtra; i++) {
      subset[i+Nsubset] = subsetExtra[i];
      LineNumber[i+Nsubset] = LineNumberExtra[i];
    }
    Nsubset += NsubsetExtra;
    MARKTIME("selected %d overlapping images: %f sec\n", (int) NsubsetExtra, dtime);
  } 
# endif

