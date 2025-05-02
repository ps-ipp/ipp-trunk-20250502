# include "dvorepair.h"

// dvorepairDeleteImagesByExternID and dvorepairDeleteImagesByExternID_v2 differ in only 2
// ways: 
// 1) _v2 does NOT attempt to delete detections
// 2) _v2 uses the 2nd word on each line in the externID list and requires 3 words per
// line, while the other version requires 9 words per line (and also uses the 2nd word)

// delete images based on a text table of the target IDs
// * load the delete table file (validate format)
// * load the Images.dat file
// * create an index for imageID (seq = imageIDindex[i])
// * create an index of the imageIDs to be deleted (delete (T/F) = imageDELindex[i])
// * determine the full RA & DEC range of the images to be deleted
// * scan over all catalog files in the specified RA & DEC range
// * load the cpm file (pad if short, identify the padded section)
// * create a new cpm file
// * loop over detections : only keep those not from images to be deleted
// * load the cpt file
// * rebuild the cpt file or delete the matching entries?

int SaveImageTable (Image *image, off_t Nimage, char *catdir, FITS_DB *oldDB);
Image *DeleteSelectedImages (Image *image, off_t Nimage, int *deleteImage, myIndexType *imageIDindex, off_t *NnewImage);
int DeleteLensing (Catalog *catalog, myIndexType *imageIDindex, int *deleteImage, int *nDelete);
int DeleteMeasure (Catalog *catalog, myIndexType *imageIDindex, int *deleteImage, int *nDelete);
int RepairAverage (Catalog *catalog);

int dvorepairDeleteImagesByExternID_v2 (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table
  
  int i;
  off_t Nimage;

  Image *image;
  char *imageFilename = NULL;
  char filename[DVO_MAX_PATH];

  if (argc != 3) {
    fprintf (stderr, "USAGE: dvorepair -delete-images-by-extern-id (catdir) (deleteList)\n");
    fprintf (stderr, "  catdir : database of interest\n");
    fprintf (stderr, "  deleteList : table from dvorepair giving images to be deleted\n");
    exit (2);
  }

  char *catdir = argv[1];
  char *delList = argv[2];

  sprintf (filename, "%s/Photcodes.dat", catdir);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading photcodes from %s\n", catdir);
    exit (1);
  }	

  // load the image data
  ALLOCATE(imageFilename, char, strlen(catdir) + 12);
  sprintf (imageFilename, "%s/Images.dat", catdir);
  if ((image = LoadImages (&db, imageFilename, &Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage); // needed for astrometric tranformations

  // generate an index for imageIDs
  myIndexType *externIDindex = myIndexInit();
  myIndexType *imageIDindex = myIndexInit();

  // set the min and max ID values for the two strucures:
  for (i = 0; i < Nimage; i++) {
    myIndexUpdateLimits (imageIDindex,  image[i].imageID);

    if (!image[i].externID) continue;
    myAssert (image[i].imageID, "image must have imageID > 0");
    myIndexUpdateLimits (externIDindex, image[i].externID);
  }

  myIndexSetRange (externIDindex);
  myIndexSetRange (imageIDindex);

  fprintf (stderr, "loaded %d images, image ID range: %d - %d, extern ID range: %d - %d\n", (int) Nimage, imageIDindex->minID, imageIDindex->maxID, externIDindex->minID, externIDindex->maxID);
			
  // deleteImage[i] will be TRUE if we want to delete that image
  int *deleteImage = NULL;
  ALLOCATE (deleteImage, int, Nimage);
  for (i = 0; i < Nimage; i++) {
    deleteImage[i] = FALSE;
  }

  // assign the externIDs and imageIDs to their index structures
  for (i = 0; i < Nimage; i++) {
    myIndexSetEntry (imageIDindex,  image[i].imageID,  i);

    if (!image[i].externID) continue;
    myIndexSetEntry (externIDindex, image[i].externID, i);
  }

  // read the list of externIDs to delete
  int NdeleteIDs;
  int *deleteIDs = ReadDeleteListExternID_v2(delList, &NdeleteIDs);

  int NdeleteImages = 0;
  for (i = 0; i < NdeleteIDs; i++) {
    int externID = deleteIDs[i];
    myAssert(externID > 0, "extern ID to delete cannot be <= 0");
    
    // we do not require all externIDs to be deleted to actually be in this db,
    // but skip any externIDs not in this db (myIndexGetEntry returns -1)
    int n = myIndexGetEntry(externIDindex, externID);
    if (n < 0) continue;

    if (deleteImage[n]) {
      fprintf (stderr, "multiple entries for extern ID %d == imageID %d, re-run\n", externID, image[n].imageID);
    }
    deleteImage[n] = TRUE;
    NdeleteImages ++;

    if (VERBOSE) fprintf (stderr, "delete %s extern ID: %d = image ID %d (seq %d)\n", image[n].name, externID, image[n].imageID, n);
  }

  if (NdeleteImages == 0) {
    fprintf (stderr, "no images to be deleted in this database, exiting\n");
    exit (0);
  }

  off_t NnewImage;
  Image *newImage = DeleteSelectedImages (image, Nimage, deleteImage, imageIDindex, &NnewImage);

  SaveImageTable (newImage, NnewImage, catdir, &db);

  myIndexFree (externIDindex);
  myIndexFree (imageIDindex);

  free (imageFilename);
  free (deleteImage);
  free (deleteIDs);

  gfits_db_free (&db);

  ohana_memcheck (TRUE);

  exit (0);
}


