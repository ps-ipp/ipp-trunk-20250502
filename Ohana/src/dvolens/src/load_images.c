# include "dvolens.h"

static off_t Nsubset = 0;
static Image *subset = NULL;

// MAX_ID requires 512M to store the image index
# define MAX_ID      0x8000000
static off_t         minImageID = MAX_ID;
static off_t         maxImageID = 0;

// as an alternative, we generate imageSeq, which directly maps imageID -> seq
static off_t *imageIDs = NULL; // list of all image IDs
static off_t *imageIdx = NULL; // list of index for image IDs 
static off_t *imageSeq = NULL; // list of index for image IDs 

void initImages (Image *image, off_t Nimage);

int load_images (SkyList *skylist) {

  FITS_DB db;
  
  gfits_db_init (&db);
  
  /* lock and load the image db table */
  int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
  if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
  // the raw FITS data is freed by dvo_image_load, leaving only the Image structure data
  
  INITTIME;

  // convert database table to internal structure
  off_t Nimage;
  Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  MARKTIME("  convert image table: %f sec\n", dtime);

  subset = select_images (skylist, image, Nimage, &Nsubset);
  MARKTIME("  select images: %f sec\n", dtime);

  // if we have generated an image subset and we are running UPDATE_OFFSETS, the we can free images here
  dvo_image_unlock (&db); 
  gfits_db_free (&db);

  initImages (subset, Nsubset);

  return TRUE;
}

void initImages (Image *image, off_t Nimage) {

  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIdx, off_t, Nimage);

  off_t i;
  for (i = 0; i < Nimage; i++) {
    imageIdx[i] = i;
    imageIDs[i] = image[i].imageID;
    minImageID = MIN(minImageID, image[i].imageID);
    maxImageID = MAX(maxImageID, image[i].imageID);
    myAssert (image[i].imageID < MAX_ID, "image IDs too large for index memory");
  }
  llsortpair (imageIDs, imageIdx, Nimage);

  ALLOCATE (imageSeq, off_t, maxImageID + 1);
  for (i = 0; i < maxImageID + 1; i++)  {
    imageSeq[i] = -1; // not yet assigned
  }

  for (i = 0; i < Nimage; i++) {
    off_t N = image[i].imageID;
    myAssert (imageSeq[N] == -1, "previously assigned");
    imageSeq[N] = i;
  }
}

Image *getimages (off_t *N) {

  *N = Nsubset;
  return subset;
}

void free_images (void) {

  if (!subset) return;

  FREE (subset);
  FREE (imageIDs);
  FREE (imageIdx);
  FREE (imageSeq);
}

off_t getImageByID (off_t ID) {
  if (imageSeq) {
    if (ID < minImageID) return (-1);
    if (ID > maxImageID) return (-1);
    off_t N = imageSeq[ID];
    return N;
  }

  // we have a pair of vectors (imageIDs, imageIdx) sorted by imageIDs
  // use bisection to find the specified image ID

  off_t Nlo, Nhi, N;

  Nlo = 0; Nhi = Nsubset;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (imageIDs[N] < ID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nsubset);
    }
  }

  for (N = Nlo; N < Nhi; N++) {
    if (imageIDs[N] == ID)
      return (imageIdx[N]);
  }

  return (-1);
}

