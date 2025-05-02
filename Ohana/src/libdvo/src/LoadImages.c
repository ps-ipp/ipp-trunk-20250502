# include "dvo.h"
# include "dvodb.h"

// XXX: Maybe make this a command line option
int dvoUseImageCache = 1;

static char *lastFilename = NULL;
static time_t lastModified = 0;
static Image *imageCache = NULL;
static off_t cacheNimage = 0;
AstromOffsetTable *table = NULL;

static time_t getLastModified(char *filename);

Image *LoadImagesDVO (off_t *Nimage) {

  int status;
  char *catdir, filename[256];
  Image *image;
  FITS_DB db;
  
  /* VarConfig ("IMAGE_CATALOG", "%s", filename); */

  catdir = dvo_get_catdir ();
  snprintf (filename, 256, "%s/Images.dat", catdir);

  if (lastFilename) {
    if (dvoUseImageCache && !strcmp(lastFilename, filename)) {
      // Make sure the file hasn't changed since we loaded it
      if (getLastModified(filename) == lastModified) {
        *Nimage = cacheNimage;
        return  imageCache;
      }
    }
    free(lastFilename);
    lastFilename = NULL;
    free(imageCache);
    imageCache = NULL;
    cacheNimage = 0;
    lastModified = 0;
  }

  gfits_db_init (&db);
  db.lockstate = LCK_SOFT;
  db.timeout   = 120.0;

  if (!gfits_db_lock (&db, filename)) {
    gprint (GP_ERR, "error opening image catalog %s (1)\n", filename);
    return (NULL);
  }

  if (db.dbstate == LCK_EMPTY) {
    gprint (GP_ERR, "note: image catalog is empty\n");
    ALLOCATE (image, Image, 1);
    *Nimage = 1;
    return (image);
  }

  status = dvo_image_load (&db, TRUE, FALSE);
  gfits_db_close (&db);

  if (!status) {
    gprint (GP_ERR, "problem loading image database table\n");
    return (NULL);
  }

  image = gfits_table_get_Image (&db.ftable, Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    return (NULL);
  }
  if (dvoUseImageCache && image) {
    cacheNimage = *Nimage;
    imageCache = image;
    lastFilename = strcreate(filename);
    lastModified = getLastModified(filename);
  }

  // assign image->parent and image->coords.mosaic 
  BuildChipMatch (image, *Nimage);
  // MARKTIME("build chip match: %f sec\n", dtime);

  char mapfile[DVO_MAX_PATH];
  snprintf (mapfile, DVO_MAX_PATH, "%s/AstroMap.fits", catdir);

  if (table) AstromOffsetTableFree(table);
  table = AstromOffsetMapLoad (mapfile, 100000, FALSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (image, *Nimage, table);
  }

  return (image);
}

static time_t getLastModified(char *filename) {
  struct stat statbuf;
  if (!stat(filename, &statbuf)) {
    return statbuf.st_mtime;
  } else {
    return 0;
  }
}

void FreeImagesDVO(Image *images) {
  if (!dvoUseImageCache && (images != NULL)) {
    free(images);
    AstromOffsetTableFree (table);
  } else {
    // defer free until next LoadImages with a different or modified Images.dat
  }
}

