# include "fakeastro.h"
int open_image_db (FITS_DB *db, char *ImageName);
int save_image_db (FITS_DB *db, Image *image, int Nimage);

int fakeastro_images () {

  INITTIME;

  FITS_DB dbfake, dbtrue;

  open_image_db (&dbfake, "Images.dat");
  open_image_db (&dbtrue, "Images.true.dat");

  SkyTable *skyTableInput = SkyTableLoadOptimal (CATDIR_INPUT, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (skyTableInput, CATDIR_INPUT, "cpt");

  SkyTable *skyTableOutput = SkyTableLoadOptimal (CATDIR_OUTPUT, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (skyTableOutput, CATDIR_OUTPUT, "cpt");

  IMAGE_ID = 1;

  int NrefImage;
  Image *refImage = load_template_images (&NrefImage); // subset of the DIS exposures
  
  ImageInfo imageInfo;

  imageInfo.NfakeImage = 0;
  imageInfo.NFAKEIMAGE = 1000;
  imageInfo.fakeImage = NULL;
  ALLOCATE (imageInfo.fakeImage, Image, imageInfo.NFAKEIMAGE);

  imageInfo.NtrueImage = 0;
  imageInfo.NTRUEIMAGE = 1000;
  imageInfo.trueImage = NULL;
  ALLOCATE (imageInfo.trueImage, Image, imageInfo.NTRUEIMAGE);

  fakeastro_images_region (&imageInfo, refImage, NrefImage, skyTableInput, skyTableOutput, &UserPatch);

  save_image_db (&dbfake, imageInfo.fakeImage, imageInfo.NfakeImage);
  save_image_db (&dbtrue, imageInfo.trueImage, imageInfo.NtrueImage);

  MARKTIME ("generate fake stars in %f sec\n", dtime)

  exit (0);
}

/* outline:

 * load reference image table
 * generate a set of fake chips for each real exposure boresite
 * select the real stars which land on the fake chips
 * generate a set of measurements for the fake chip
 * need to ingest the measurements into the new DVO as if they came from addstar

 * I can probably load stars for a decent fraction of the sky at once
 */


// -input catdir -output catdir -images images.fits  

int open_image_db (FITS_DB *db, char *ImageName) {

  char ImageCat[DVO_MAX_PATH];

  // we are creating a new image table (what about db ID?)
  snprintf (ImageCat, DVO_MAX_PATH, "%s/%s", CATDIR_OUTPUT, ImageName);

  /* setup image table format and lock */
  gfits_db_init (db);
  db->mode    = dvo_catalog_catmode (CATMODE);
  db->format  = dvo_catalog_catformat (CATFORMAT);
  int status = dvo_image_lock (db, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db->filename);

  /* load or create the image table */
  if (db->dbstate != LCK_EMPTY) {
    Shutdown ("image table %s exists, exiting", db->filename);
  }
  dvo_image_create (db, GetZeroPoint());

  return TRUE;
}

int save_image_db (FITS_DB *db, Image *image, int Nimage) {

  /* add the new image and save */
  dvo_image_addrows (db, image, Nimage);
  SetProtect (TRUE);
  dvo_image_update (db, VERBOSE);
  SetProtect (FALSE);
  dvo_image_unlock (db); /* unlock? */

  return TRUE;
}

