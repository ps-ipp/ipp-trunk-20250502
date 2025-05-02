# include "photdbc.h"

int copy_images (char *outdir, SkyList *skylist) {

  int status;
  off_t Nimage, Nsubset;
  off_t *LineNumber;
  char *ImageOut;
  unsigned int imageID;
  FITS_DB in;
  FITS_DB out;
  Image *image, *subset;
  struct stat filestat;
  char *path;
  char filename[1024];

  path = pathname (ImageCat);
  status = stat (path, &filestat);
  if (status == -1) Shutdown ("ERROR: CATDIR %s does not exist, no data in database", path);

  gfits_db_init (&in);
  status = dvo_image_lock (&in, ImageCat, 60.0, LCK_SOFT);
  if (!status) {
    fprintf (stderr, "No images in catalog %s\n", ImageCat);
    return (TRUE);
  }
  if (in.dbstate == LCK_EMPTY) {
    fprintf (stderr, "No images in catalog %s\n", ImageCat);
    dvo_image_unlock (&in);
    return (TRUE);
  }

  // load the input image data
  if (!dvo_image_load (&in, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", in.filename);

  // define output filename (replace CATDIR)
  ImageOut = strsubs (in.filename, CATDIR, outdir);
  if (ImageOut == NULL) Shutdown ("error with input or output catalog name");

  // lock the output catalog
  gfits_db_init (&out);
  status = dvo_image_lock (&out, ImageOut, 60.0, LCK_XCLD);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", ImageOut);
  if (out.dbstate != LCK_EMPTY) Shutdown ("ERROR: image table exists %s", ImageOut);

  out.mode   = CATMODE   ? dvo_catalog_catmode (CATMODE)     : in.mode;
  out.format = CATFORMAT ? dvo_catalog_catformat (CATFORMAT) : in.format;
  dvo_image_create (&out, ZERO_POINT);
    
  image = gfits_table_get_Image (&in.ftable, &Nimage, &in.scaledValue, &in.nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  BuildChipMatch (image, Nimage);
  // MARKTIME("build chip match: %f sec\n", dtime);

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", CATDIR);
  AstromOffsetTable *table_src = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table_src) {
    AstromOffsetTableMatchChips (image, Nimage, table_src);
  }

  subset = select_images (skylist, image, Nimage, &LineNumber, &Nsubset, FALSE);

  if (table_src) {
    AstromOffsetTable *table_tgt = AstromOffsetTableInit ();
    
    off_t i;
    for (i = 0; i < Nsubset; i++) {
      if (!subset[i].coords.offsetMap) continue;
      if (subset[i].coords.Npolyterms != -1) continue; // assert on this?
      AstromOffsetTableAddMapFromImage (table_tgt, &subset[i]);
    }
    snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", outdir);
    AstromOffsetMapSave (table_tgt, mapfile);
    free (table_tgt->imageIDtoTableSeq);
    free (table_tgt->map);
    free (table_tgt);
  }

  dvo_image_addrows (&out, subset, Nsubset);

  // note that imageID is unsigned int
  status = gfits_scan (&in.header, "IMAGEID", "%u", 1, &imageID);
  if (!status) {
    status = gfits_scan (&in.header, "NIMAGES", "%u", 1, &imageID);
    imageID++;
  }
  status = gfits_modify (&out.header, "IMAGEID", "%u", 1, imageID);

  dvo_image_update (&out, VERBOSE);
  dvo_image_unlock (&out);

  dvo_image_unlock (&in);

  // create the photcode file
  sprintf (filename, "%s/Photcodes.dat", outdir);
  SavePhotcodesFITS (filename);

  // free the astrom offset tables
  AstromOffsetTableFree (table_src);

  return (TRUE);
}

// globals: ImageCat, VERBOSE, CATMODE, CATFORMAT, ZeroPt
