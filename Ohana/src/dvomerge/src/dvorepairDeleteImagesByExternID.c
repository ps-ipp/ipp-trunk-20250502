# include "dvorepair.h"
# define DEBUG 1

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

typedef struct {
  double Rmin;
  double Rmax;
  double Qmin; 
  double Qmax;
  double Dmin;
  double Dmax;
} DeleteRegion;


int dvorepairDeleteImagesByExternID (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);

  FITS_DB db;  // database handle pointing to input image table
  
  int i;
  off_t Nimage;

  Image *image;
  char *imageFilename = NULL;
  char filename[DVO_MAX_PATH];

  // CATDIR from first arg, not -D CATDIR
  CATDIR = argv[1];
  char *delList = argv[2];

  sprintf (filename, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading photcodes from %s\n", CATDIR);
    exit (1);
  }	

  // load the image data
  ALLOCATE(imageFilename, char, strlen(CATDIR) + 12);
  sprintf (imageFilename, "%s/Images.dat", CATDIR);
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
  int *deleteIDs = ReadDeleteListExternID(delList, &NdeleteIDs);

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

  SkyRegion UserPatch[2]; // 1 or 2 UserPatch areas are set by FindDeleteRegion
  int nUserPatch = FindDeleteRegion (UserPatch, image, Nimage, deleteImage);

  // delete detections in the external catalog.  things I need:
  // UserPatch, nUserPatch : definition of the region
  // deleteImage : boolean vector of length Nimage (delete or not?)
  // imageID : vector of length Nimage, with the image IDs in the same order as deleteImage

  dvorepairDeleteImagesByExternID_catalogs (UserPatch, nUserPatch, image, Nimage, deleteImage, imageIDindex);

  off_t NnewImage;
  Image *newImage = DeleteSelectedImages (image, Nimage, deleteImage, imageIDindex, &NnewImage);

  SaveImageTable (newImage, NnewImage, CATDIR, &db);

  myIndexFree (externIDindex);
  myIndexFree (imageIDindex);

  free (imageFilename);
  free (deleteImage);
  free (deleteIDs);

  gfits_db_free (&db);

  ohana_memcheck (TRUE);

  exit (0);
}

Image *DeleteSelectedImages (Image *image, off_t Nimage, int *deleteImage, myIndexType *imageIDindex, off_t *NnewImage) {

  int i;

  // we need to remove the (WRP) images, but we also want to remove the DIS [PHU] image entries if they have
  // had all of their children deleted

  // first, generate an array of the number of children for each parent

  int *Nchildren = NULL;
  ALLOCATE (Nchildren, int, Nimage);
  memset (Nchildren, 0, Nimage*sizeof(int));

  // parentID is not necessarily reliable (dvomerge breaks it, for example).  rebuild it
  // here by using BuildChipMatch (libdvo/src/mosaic_astrom.c):

  BuildChipMatch (image, Nimage);

  // reassign parentID values
  int NbadParentIDs  = 0;
  for (i = 0; i < Nimage; i++) {
    if (!image[i].parent) continue;
    
    if (image[i].parentID != image[i].parent[0].imageID) {
      NbadParentIDs ++;
      if (NbadParentIDs < 100) {
	int n = myIndexGetEntry (imageIDindex, image[i].parentID);
	fprintf (stderr, "bad parent ID: %s matchs %s not %s\n", image[i].name, image[i].parent[0].name, image[n].name);
      }
      image[i].parentID = image[i].parent[0].imageID;
    }
  }

  // count up the children for each parent
  for (i = 0; i < Nimage; i++) {
    if (!image[i].parentID) continue;
    if (strcmp(&image[i].coords.ctype[4], "-WRP")) {
      fprintf (stderr, "warning: child with non WRP coords : %s\n", image[i].name);
    }

    int n = myIndexGetEntry (imageIDindex, image[i].parentID);
    myAssert (n != -1, "invalid parentID?");
    myAssert (n < Nimage, "invalid parentID??");
    Nchildren[n] ++;
  }

  // now subtract the children which need to be deleted
  for (i = 0; i < Nimage; i++) {
    if (!deleteImage[i]) continue;
    if (!image[i].parentID) continue; // should I warn on this?

    int n = myIndexGetEntry (imageIDindex, image[i].parentID);
    myAssert (n != -1, "invalid parentID?");
    myAssert (n < Nimage, "invalid parentID??");
    Nchildren[n] --;
    myAssert (Nchildren[n] >= 0, "over deleted???");
  }
  
  // now check for any parent and delete it
  for (i = 0; i < Nimage; i++) {
    if (strcmp(&image[i].coords.ctype[4], "-DIS")) continue;
    myAssert (!image[i].parentID, "parent ID for DIS image??");
    if (!Nchildren[i]) deleteImage[i] = TRUE;
  }

  fprintf (stderr, "deleting the following images:\n");

  off_t Nnew = 0;
  Image *newImage = NULL;
  ALLOCATE (newImage, Image, Nimage);
  for (i = 0; i < Nimage; i++) {
    if (deleteImage[i]) { 
      fprintf (stderr, "DELETE: %s : %d : %d : %d\n", image[i].name, image[i].imageID, image[i].parentID, image[i].externID);
      continue;
    }
    newImage[Nnew] = image[i];
    Nnew ++;
  }

  REALLOCATE (newImage, Image, Nnew);

  *NnewImage = Nnew;
  return newImage;
}

int SaveImageTable (Image *image, off_t Nimage, char *catdir, FITS_DB *oldDB) {

  FITS_DB db;

  char ImageCatSrc[DVO_MAX_PATH];
  char ImageCatTgt[DVO_MAX_PATH];

  // we are creating a new image table (what about db ID?)
  snprintf (ImageCatSrc, DVO_MAX_PATH, "%s/Images.dat", catdir);
  snprintf (ImageCatTgt, DVO_MAX_PATH, "%s/Images.dat.broken", catdir);

  // rename the old cpt file:
  if (rename (ImageCatSrc, ImageCatTgt)) {
    perror ("tried to rename file");
    exit (2);
  }

  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode    = dvo_catalog_catmode ("SPLIT");
  db.format  = dvo_catalog_catformat ("PS1_V5");
  int status = dvo_image_lock (&db, ImageCatSrc, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) {
    fprintf (stderr, "ERROR: failure to lock image catalog %s", db.filename);
    exit (2);
  }

  /* load or create the image table */
  if (db.dbstate != LCK_EMPTY) {
    fprintf (stderr, "image table %s exists, exiting", db.filename);
    exit (2);
  }
  dvo_image_create (&db, GetZeroPoint());

  // I want to keep the old DVO_DBID
  char dbID[33];
  int imageIDmax;
  if (!gfits_scan (&oldDB->header, "DVO_DBID", "%s", 1, dbID)) {
    fprintf (stderr, "source image table is missing DVO_DBID, exiting\n");
    exit (3);
  }
  if (!gfits_scan (&oldDB->header, "IMAGEID", "%d", 1, &imageIDmax)) {
    fprintf (stderr, "max image ID is missing, exiting\n");
    exit (3);
  }
  gfits_modify (&db.header, "IMAGEID", "%d", 1, imageIDmax);      
  gfits_modify (&db.header, "DVO_DBID", "%s", 1, dbID);      

  // also save the merge history:
  int Nmerge;
  if (gfits_scan (&oldDB->header, "NMERGE", "%d", 1, &Nmerge)) {
    int i;
    gfits_modify (&db.header, "NMERGE", "%d", 1, Nmerge);
    for (i = 0; i < Nmerge; i++) {
      char field[80], value[80];
      snprintf (field, 80, "DM_%05d", i);
      if (!gfits_scan (&oldDB->header, field, "%s", 1, value)) {
	fprintf (stderr, "missing merge info %s\n", field);
	continue;
      }
      gfits_modify (&db.header, field, "%s", 1, value);
    }
  }

  /* add the new image and save */
  dvo_image_addrows (&db, image, Nimage);
  dvo_image_update (&db, VERBOSE);
  dvo_image_unlock (&db); /* unlock? */

  return TRUE;
}

int RepairTableCPT_V1(char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, myIndexType *imageIDindex, char catformat) {
  OHANA_UNUSED_PARAM(Nimage);

  off_t *averefMatch;
  off_t i, NaveMax, Naverage, NAVERAGE, NaverageOut, Nave, Nout, Nold;
  int *found, Nsecfilt;

  Average *average, *averageOut;

  Matrix matrix;

  Header cptHeaderPHU;
  Header cptHeaderTBL;
  FTable cptFtable;

  cptFtable.header = &cptHeaderTBL;

  NaveMax = 0;
  NAVERAGE = 1000;
  ALLOCATE (average, Average, NAVERAGE);
  memset (average, 0, NAVERAGE*sizeof(Average));

  ALLOCATE (found, int, NAVERAGE);
  memset (found, 0, NAVERAGE*sizeof(int));

  // examine all measurements and create new objects as needed
  // we start with a valid, sorted dvo database, so we use averef to link objects
  for (i = 0; i < Nmeasure; i++) {
    Nave = measure[i].averef;

    // we only allocate as many as we need
    if (Nave >= NAVERAGE) {
      Nold = NAVERAGE;
      NAVERAGE = MAX(Nave + 1000, NAVERAGE + 1000);
      REALLOCATE (average, Average, NAVERAGE);
      memset (&average[Nold], 0, (NAVERAGE - Nold)*sizeof(Average));

      REALLOCATE (found, int, NAVERAGE);
      memset (&found[Nold], 0, (NAVERAGE - Nold)*sizeof(int));
    }

    // this measure matches an existing average, just check it is OK and bump Nmeasure
    if (found[Nave]) {
      average[Nave].Nmeasure ++;
      myAssert(average[Nave].objID == measure[i].objID, "objIDs do not match!");
      myAssert(average[Nave].catID == measure[i].catID, "catIDs do not match!");
      continue;
    }

    NaveMax = MAX(Nave, NaveMax);

    found[Nave] = TRUE;

    // we are going to leave most of the elements of average unset: they are the result of 
    // the relastro analysis for this object and can be recreated with a call to relastro

    // need to find image so we can use ccd coordinates to determine RA & DEC
    int n = myIndexGetEntry (imageIDindex, measure[i].imageID);
    myAssert (n > -1, "impossible!");
    
    dvo_average_init (&average[Nave]);

    // I actually have measure->R,D, I could just use those...
    XY_to_RD (&average[Nave].R, &average[Nave].D, measure[i].Xccd, measure[i].Yccd, &image[n].coords);

    average[Nave].Nmeasure = 1;
    average[Nave].Nmissing = 0;

    // assume the resulting table set is unsorted
    average[Nave].measureOffset = -1;
    average[Nave].missingOffset = -1;

    average[Nave].objID = measure[i].objID;
    average[Nave].catID = measure[i].catID;
    average[Nave].extID = CreatePSPSObjectID(average[Nave].R, average[Nave].D);
  }
  Naverage = NaveMax + 1;

  // we now have an average table, but there will be holes due to deleted measurements 
  // create a new average table with only existing entries

  ALLOCATE (averageOut, Average, Naverage);
  memset (averageOut, 0, Naverage*sizeof(Average));

  ALLOCATE (averefMatch, off_t, Naverage);
  memset (averefMatch, 0, Naverage*sizeof(int));

  Nave = 0;
  for (i = 0; i < Naverage; i++) {
    if (!found[i]) continue;
    averageOut[Nave] = average[i]; // use a memcpy?
    averefMatch[i] = Nave;
    Nave ++;
  }
  NaverageOut = Nave;

  // modify measure.averef to match the new sequence
  for (i = 0; i < Nmeasure; i++) {
    Nave = measure[i].averef;
    Nout = averefMatch[Nave];
    myAssert(Nout < NaverageOut, "output averef is wrong");
    
    myAssert(average[Nave].objID == measure[i].objID, "objIDs do not match");
    myAssert(average[Nave].catID == measure[i].catID, "objIDs do not match");
    myAssert(averageOut[Nout].objID == measure[i].objID, "objIDs do not match");
    myAssert(averageOut[Nout].catID == measure[i].catID, "objIDs do not match");

    measure[i].averef = Nout;
  }

  fprintf (stderr, "cpt file : %d obj -> %d obj (%s -> %s)\n", (int) Naverage, (int) NaverageOut, cptFilenameSrc, cptFilenameTgt);

  // open source cpt file
  FILE *cptFile = fopen(cptFilenameSrc, "r");
  myAssert(cptFile, "failed to open cpt file");

  // load the cpt header (use for CATID, RA, DEC range, filenames)
  if (!gfits_fread_header (cptFile, &cptHeaderPHU)) {
    myAbort("failure to cpt header");
  }

  // update the output header
  gfits_modify (&cptHeaderPHU, "NSTARS",     OFF_T_FMT, 1,  NaverageOut);
  gfits_modify (&cptHeaderPHU, "NMEAS",      OFF_T_FMT, 1,  Nmeasure);
  gfits_modify (&cptHeaderPHU, "NMISS",      "%d",      1,  0);
  gfits_modify_alt (&cptHeaderPHU, "SORTED", "%t",      1,  FALSE);

  gfits_scan (&cptHeaderPHU, "NSECFILT",     "%d",      1,  &Nsecfilt);

  if (0) {
    char compressMode[256];
    if (gfits_scan (&cptHeaderTBL, "DVO_CMP", "%s", 1, compressMode)) {
      if (strcmp (compressMode, "NONE")) {
	myAbort ("fix compression");
      }
    }
  }

  /* convert internal to external format */
  if (!AverageToFtable (&cptFtable, averageOut, NaverageOut, catformat, NULL, TRUE)) {
    myAbort("trouble converting format");
  }
  fclose(cptFile);
  
  // rename the old cpt file:
  if (rename (cptFilenameSrc, cptFilenameTgt)) {
    perror ("tried to rename file");
    exit (2);
  }

  // create and write the output file
  cptFile = fopen(cptFilenameSrc, "w");
  myAssert(cptFile, "failed to open cpt file");
    
  // write PHU header
  if (!gfits_fwrite_header (cptFile, &cptHeaderPHU)) {
    myAbort("can't write primary header");
  }

  // write the PHU matrix; this is probably a NOP, do I have to keep it in?
  gfits_create_matrix (&cptHeaderPHU, &matrix);
  if (!gfits_fwrite_matrix  (cptFile, &matrix)) {
    myAbort("can't write primary matrix");
  }
  gfits_free_matrix (&matrix);

  // write the table data
  if (!gfits_fwrite_ftable_range (cptFile, &cptFtable, 0, NaverageOut, 0, NaverageOut)) {
    myAbort("can't write table data");
  }
  fclose(cptFile);

  gfits_free_table (&cptFtable);
  gfits_free_header (&cptHeaderPHU);
  gfits_free_header (&cptHeaderTBL);
  free (average);
  free (averageOut);

  free (found);
  free (averefMatch);

  { 
    Header cpsHeaderPHU;
    Header cpsHeaderTBL;
    FTable cpsFtable;

    SecFilt *secfilt = NULL;

    cpsFtable.header = &cpsHeaderTBL;

    // open source cpt file
    FILE *cpsFile = fopen(cpsFilenameSrc, "r");
    myAssert(cpsFile, "failed to open cps file");
    
    // load the cps header (use for CATID, RA,DEC range, filenames)
    if (!gfits_fread_header (cpsFile, &cpsHeaderPHU)) {
      myAbort("failure to cps header");
    }

    int Nrows = Nsecfilt*NaverageOut;
    ALLOCATE (secfilt, SecFilt, Nrows);

    for (i = 0; i < Nrows; i++) {
      dvo_secfilt_init (&secfilt[i], SECFILT_RESET_ALL);
    }

    /* convert internal to external format */
    if (!SecFiltToFtable (&cpsFtable, secfilt, Nrows, catformat, TRUE)) {
      myAbort("trouble converting format");
    }
    fclose(cpsFile);

    // rename the old cpt file:
    if (rename (cpsFilenameSrc, cpsFilenameTgt)) {
      perror ("tried to rename file");
      exit (2);
    }

    // create and write the output file
    cpsFile = fopen(cpsFilenameSrc, "w");
    myAssert(cpsFile, "failed to open cps file");
    
    // write PHU header
    if (!gfits_fwrite_header (cpsFile, &cpsHeaderPHU)) {
      myAbort("can't write primary header");
    }

    // write the PHU matrix; this is probably a NOP, do I have to keep it in?
    gfits_create_matrix (&cpsHeaderPHU, &matrix);
    if (!gfits_fwrite_matrix  (cpsFile, &matrix)) {
      myAbort("can't write primary matrix");
    }
    gfits_free_matrix (&matrix);

    // write the table data
    if (!gfits_fwrite_ftable_range (cpsFile, &cpsFtable, 0, Nrows, 0, Nrows)) {
      myAbort("can't write table data");
    }
    fclose(cpsFile);

    gfits_free_table (&cpsFtable);
    gfits_free_header (&cpsHeaderPHU);
    gfits_free_header (&cpsHeaderTBL);
    free (secfilt);
  }

  return (TRUE);
}

int RepairAverage (Catalog *catalog) {

  off_t i, j, Nave;

  // average is sorted so averef is valid
  // secfilt is also sorted so sequence is valid (but this is kind of moot since it will be re-calculated)
  // average.measureOffset is NOT valid
  // average.lensingOffset is NOT valid

  Average *average = catalog->average;
  Measure *measure = catalog->measure;
  Lensing *lensing = catalog->lensing;
  
  // reset the values of average.Nmeasure, average.Nlensing
  for (i = 0; i < catalog->Naverage; i++) {
    average[i].Nmeasure = 0;
    average[i].Nlensing = 0;
  }

  // re-calculate average.Nmeasure
  for (i = 0; i < catalog->Nmeasure; i++) {
    Nave = measure[i].averef;
    myAssert (measure[i].objID == average[Nave].objID, "invalid measure:average match");
    average[Nave].Nmeasure ++;
  }
    
  // re-calculate average.Nlensing
  for (i = 0; i < catalog->Nlensing; i++) {
    Nave = lensing[i].averef;
    myAssert (lensing[i].objID == average[Nave].objID, "invalid lensing:average match");
    average[Nave].Nlensing ++;
  }

  // create a copy of the average table, keeping only the entries with Nmeasure & Nlensing > 0
  ALLOCATE_PTR (averageNew, Average, catalog->Naverage);
  ALLOCATE_PTR (averefNew,  int,     catalog->Naverage);

  Nave = 0;
  for (i = 0; i < catalog->Naverage; i++) {
    averefNew[i] = -1;
    if (!average[i].Nmeasure && !average[i].Nlensing) continue;

    averageNew[Nave] = average[i];
    averefNew[i] = Nave;
    Nave ++;
  }
  off_t NaverageNew = Nave;

  // update measure.averef values
  for (i = 0; i < catalog->Nmeasure; i++) {
    Nave = averefNew[measure[i].averef];
    myAssert (Nave >= 0, "oops");
    measure[i].averef = Nave;
  }
    
  // update lensing.averef values
  for (i = 0; i < catalog->Nlensing; i++) {
    Nave = averefNew[lensing[i].averef];
    myAssert (Nave >= 0, "oops");
    lensing[i].averef = Nave;
  }
  
  // XXX need to update measureOffset and lensingOffset

  // measure[] should be blocked and sequential: 
  // measure[i+1].averef >= measure[i].averef

  // update average.measureOffset values
  Nave = -1;
  for (i = 0; i < catalog->Nmeasure; i++) {
    if (measure[i].averef == Nave) continue;
    Nave = measure[i].averef;
    averageNew[Nave].measureOffset = i;
  }

  // update average.lensingOffset values
  Nave = -1;
  for (i = 0; i < catalog->Nlensing; i++) {
    if (lensing[i].averef == Nave) continue;
    Nave = lensing[i].averef;
    averageNew[Nave].lensingOffset = i;
  }

  // check the result (measure -> average)
  for (i = 0; i < catalog->Nmeasure; i++) {
    Nave = measure[i].averef;
    myAssert(averageNew[Nave].objID == measure[i].objID, "objIDs do not match");
    myAssert(averageNew[Nave].catID == measure[i].catID, "catIDs do not match");
  }
  // check the result (average -> measure)
  for (i = 0; i < NaverageNew; i++) {
    int m = averageNew[i].measureOffset;
    for (j = 0; j < averageNew[i].Nmeasure; j++) {
      myAssert(averageNew[i].objID == measure[j+m].objID, "objIDs do not match");
      myAssert(averageNew[i].catID == measure[j+m].catID, "catIDs do not match");
      myAssert(measure[j+m].averef == i, "averef broken");
    }
  }

  // check the result (lensing -> average)
  for (i = 0; i < catalog->Nlensing; i++) {
    Nave = lensing[i].averef;
    myAssert(averageNew[Nave].objID == lensing[i].objID, "objIDs do not match");
    myAssert(averageNew[Nave].catID == lensing[i].catID, "catIDs do not match");
  }
  // check the result (average -> lensing)
  for (i = 0; i < NaverageNew; i++) {
    int m = averageNew[i].lensingOffset;
    for (j = 0; j < averageNew[i].Nlensing; j++) {
      myAssert(averageNew[i].objID == lensing[j+m].objID, "objIDs do not match");
      myAssert(averageNew[i].catID == lensing[j+m].catID, "catIDs do not match");
      myAssert(lensing[j+m].averef == i, "averef broken");
    }
  }

  free (catalog->secfilt);
  ALLOCATE (catalog->secfilt, SecFilt, NaverageNew*catalog->Nsecfilt);
  for (i = 0; i < NaverageNew*catalog->Nsecfilt; i++) {
    dvo_secfilt_init (&catalog->secfilt[i], SECFILT_RESET_ALL);
  }

  free (averefNew);
  free (catalog->average);
  catalog->average = averageNew;
  catalog->Naverage = NaverageNew;
  catalog->Naverage_disk = NaverageNew;

  catalog->Nsecfilt_disk = NaverageNew*catalog->Nsecfilt;

  return (TRUE);
}

// delete measure entries
int DeleteMeasure (Catalog *catalog, myIndexType *imageIDindex, int *deleteImage, int *nDelete) {

  int j;

  Measure *measure = catalog->measure;

  // allocate an output array of measures (to replace, if needed)
  ALLOCATE_PTR (measureNew, Measure, catalog->Nmeasure);

  int NmeasureNew = 0;
  int NmeasureDel = 0;

  // examine all measurements: find ones that need to be deleted
  for (j = 0; j < catalog->Nmeasure; j++) {
    int imageID = measure[j].imageID;
    if (!imageID) continue;
    // myAssert(imageID, "measure is missing an image ID");
    // this case is valid if we have REF detections (no associated image)

    int N = myIndexGetEntry(imageIDindex, imageID);
    if (N < 0) {
      // this detection comes from a non-existant image; delete
      NmeasureDel ++;
      continue;
    }

    // measure matches a bad image; delete
    if (deleteImage[N]) {
      NmeasureDel ++;
      continue;
    }

    // keep this measure
    measureNew[NmeasureNew] = measure[j];
    NmeasureNew ++;
  }

  free (catalog->measure);
  catalog->measure = measureNew;
  catalog->Nmeasure = NmeasureNew;
  catalog->Nmeasure_disk = NmeasureNew;

  *nDelete = NmeasureDel;

  return TRUE;
}

// delete lensing entries
int DeleteLensing (Catalog *catalog, myIndexType *imageIDindex, int *deleteImage, int *nDelete) {

  int j;

  Lensing *lensing = catalog->lensing;

  // allocate an output array of measures (to replace, if needed)
  ALLOCATE_PTR (lensingNew, Lensing, catalog->Nlensing);

  int NlensingNew = 0;
  int NlensingDel = 0;

  // examine all lensing: find ones that need to be deleted
  for (j = 0; j < catalog->Nlensing; j++) {
    int imageID = lensing[j].imageID;
    myAssert(imageID, "lensing is missing an image ID");

    int N = myIndexGetEntry(imageIDindex, imageID);
    if (N < 0) {
      // this detection comes from a non-existant image; delete
      NlensingDel ++;
      continue;
    }

    // lensing matches a bad image; delete
    if (deleteImage[N]) {
      NlensingDel ++;
      continue;
    }

    // keep this lensing
    lensingNew[NlensingNew] = lensing[j];
    NlensingNew ++;
  }

  free (catalog->lensing);
  catalog->lensing = lensingNew;
  catalog->Nlensing = NlensingNew;
  catalog->Nlensing_disk = NlensingNew;

  *nDelete = NlensingDel;

  return TRUE;
}

// return number of SkyRegions which get set
int FindDeleteRegion (SkyRegion *UserPatch, Image *image, off_t Nimage, int *deleteImage) {

  // Rmin,Rmax run from 0 - 360.0; for Rmin < 180.0, Rmin points Qmin,Qmax run from -180.0 - +180.0

  // find the RA & DEC range of the images we want to delete
  double Dmin =  +90.0;
  double Dmax =  -90.0;
  double Rmin = +360.0; 
  double Rmax =    0.0;
  double Qmin = +180.0;
  double Qmax = -180.0;

  int i;
  for (i = 0; i < Nimage; i++) {
    if (!deleteImage[i]) continue;

    double Rthis, Dthis, Qthis;
    XY_to_RD(&Rthis, &Dthis, 0, 0, &image[i].coords);
    Rthis = ohana_normalize_angle_to_midpoint (Rthis, 180.0); // Rthis:    0.0 - 360.0
    Qthis = ohana_normalize_angle_to_midpoint (Rthis,   0.0); // Qthis: -180.0 - 180.0
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    XY_to_RD(&Rthis, &Dthis, image[i].NX, 0, &image[i].coords);
    Rthis = ohana_normalize_angle_to_midpoint (Rthis, 180.0); // Rthis:    0.0 - 360.0
    Qthis = ohana_normalize_angle_to_midpoint (Rthis,   0.0); // Qthis: -180.0 - 180.0
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    XY_to_RD(&Rthis, &Dthis, 0, image[i].NY, &image[i].coords);
    Rthis = ohana_normalize_angle_to_midpoint (Rthis, 180.0); // Rthis:    0.0 - 360.0
    Qthis = ohana_normalize_angle_to_midpoint (Rthis,   0.0); // Qthis: -180.0 - 180.0
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    XY_to_RD(&Rthis, &Dthis, image[i].NX, image[i].NY, &image[i].coords);
    Rthis = ohana_normalize_angle_to_midpoint (Rthis, 180.0); // Rthis:    0.0 - 360.0
    Qthis = ohana_normalize_angle_to_midpoint (Rthis,   0.0); // Qthis: -180.0 - 180.0
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);
  }

  double dQ = Qmax - Qmin;
  double dR = Rmax - Rmin;

  UserPatch[0].Dmin = Dmin - 0.1;
  UserPatch[0].Dmax = Dmax + 0.1;
  UserPatch[1].Dmin = Dmin - 0.1;
  UserPatch[1].Dmax = Dmax + 0.1;

  int nPass;
  if (dR < dQ + 0.1) {
    UserPatch[0].Rmin = Rmin - 0.1;
    UserPatch[0].Rmax = Rmax + 0.1;
    fprintf (stderr, "R,D range: %f - %f, %f - %f\n", Rmin, Rmax, Dmin, Dmax);
    nPass = 1;
  } else {
    // Qmax is close to (but above) 0.0
    // Qmin + 360.0 maps to a point close to (but below) 360.0
    UserPatch[0].Rmin = 0.0;
    UserPatch[0].Rmax = Qmax + 0.1;
    UserPatch[1].Rmin = Qmin + 360.0 - 0.1;
    UserPatch[1].Rmax = 360.0;
    fprintf (stderr, "R,D range: %f - %f, %f - %f\n", Qmin, Qmax, Dmin, Dmax);
    nPass = 2;
  }

  return nPass;
}

int dvorepairDeleteImagesByExternID_catalogs (SkyRegion *UserPatch, int nUserPatch, Image *image, off_t Nimage, int *deleteImage, myIndexType *imageIDindex) {

  if (PARALLEL && !HOST_ID) {
    int status = dvorepairDeleteImagesByExternID_parallel (UserPatch, nUserPatch, image, Nimage, deleteImage);
    return status;
  }

  // load the sky table for the existing database
  SkyTable *insky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(insky, "can't read SkyTable");
  SkyTableSetFilenames (insky, CATDIR, "cpt");
  
  int NmeasureDelTotal = 0;
  int NlensingDelTotal = 0;

  int patch;
  for (patch = 0; patch < nUserPatch; patch++) {
    SkyList *inlist = SkyListByPatch (insky, -1, &UserPatch[patch]);
  
    fprintf (stderr, "%d cpt regions affected\n", (int) inlist->Nregions);

    // loop over the populated input regions
    int i;
    for (i = 0; i < inlist[0].Nregions; i++) {
      if (!inlist[0].regions[i][0].table) continue;

      // XXX deal with parallel db filenames
      // snprintf (filename, DVO_MAX_PATH, "%s/%s.cpt", CATDIR, inlist[0].regions[i][0].name);

      // does this host ID match the desired location for the table?
      if (!HostTableTestHost(inlist[0].regions[i], HOST_ID)) continue;

      // set the parameters which guide catalog open/load/create
      char hostfile[DVO_MAX_PATH];
      snprintf (hostfile, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, inlist[0].regions[i][0].name);
      char *filename = HOST_ID ? hostfile : inlist[0].filename[i];

      Catalog catalog;

      // set up the basic catalog info
      dvo_catalog_init (&catalog, TRUE);
      catalog.filename  = filename; 
      catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT | DVO_LOAD_LENSING;
      catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
      
      if (!dvo_catalog_open (&catalog, inlist[0].regions[i], VERBOSE, "w")) {
	fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
	exit (1);
      }
      if (!catalog.Naverage_disk) {
	if (VERBOSE) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
      }
      
      /*** delete the measure and lensing entries matching the list of imageIDs ***/
      int NmeasureDel = 0;
      DeleteMeasure (&catalog, imageIDindex, deleteImage, &NmeasureDel);

      int NlensingDel = 0;
      DeleteLensing (&catalog, imageIDindex, deleteImage, &NlensingDel);

      fprintf (stderr, "deleting %d measure, %d lensing (keep %d, %d)\n", NmeasureDel, NlensingDel, (int) catalog.Nmeasure, (int) catalog.Nlensing);

      if (!NmeasureDel && !NlensingDel) {
	if (VERBOSE) fprintf (stderr, "nothing to delete in %s\n", catalog.filename);
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
      }

      NmeasureDelTotal += NmeasureDel;
      NlensingDelTotal += NlensingDel;

      // the CPT and CPS tables need to be regenerated.  This must happen first because, in the process, we also update measure->averef
      // what about other tables?
      RepairAverage (&catalog);

      if (VERBOSE) fprintf (stderr, "saving catalog %s\n", catalog.filename);
      
      if (!dvo_catalog_backup (&catalog, ".undel", TRUE)) {
	fprintf (stderr, "ERROR: failed to make backup for catalog %s\n", catalog.filename);
	exit (1);
      }

      SetProtect (TRUE);
      if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
      if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
      SetProtect (FALSE);
      dvo_catalog_free (&catalog);
    }
    fprintf (stderr, "\n");
    SkyListFree(inlist);
  }

  fprintf (stderr, "Deleted %d measure, %d lensing detections\n", NmeasureDelTotal, NlensingDelTotal);
  return TRUE;
}

int dvorepairDeleteImagesByExternID_parallel (SkyRegion *UserPatch, int nUserPatch, Image *image, off_t Nimage, int *deleteImage) {

  // ensure that the paths are absolute path names
  char *abscatdir = abspath (CATDIR, DVO_MAX_PATH);

  // load the sky table for the existing database
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(sky, "can't read SkyTable");
  
  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  char filename[DVO_MAX_PATH];
  snprintf (filename, DVO_MAX_PATH, "%s/DeleteImages.fits", CATDIR);
  if (!DeleteImagesSave (filename, image, Nimage, deleteImage)) {
    fprintf (stderr, "ERROR: failure to save delete image info\n");
    exit (2);
  }

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // options / arguments that can affect relastro_client -update-objects:
    char *command = NULL;
    strextend (&command, "dvorepair_client -delete-images-by-extern-id %s -hostID %d -hostdir %s", abscatdir, table->hosts[i].hostID, table->hosts[i].pathname);

    strextend (&command, "-region %f %f %f %f", UserPatch[0].Rmin, UserPatch[0].Rmax, UserPatch[0].Dmin, UserPatch[0].Dmax);
    if (nUserPatch == 2) {
      strextend (&command, "-region %f %f %f %f", UserPatch[1].Rmin, UserPatch[1].Rmax, UserPatch[1].Dmin, UserPatch[1].Dmax);
    }

    if (VERBOSE) { strextend (&command, "-v"); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running dvorepair_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the dvorepair_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      
