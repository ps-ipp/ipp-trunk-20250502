# include "dvomerge.h"

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

int dvorepairDeleteImageList (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table

  off_t i, j, Nmeasure, NmeasureNew, Ndelete, Nvalid, Nimage, Nindex, *imageIdx, index;
  int seq, Nbytes, NbytesPerRow, Ncheck, nPass, raPass;
  double Rthis, Dthis, Rmin, Rmax, Dmin, Dmax, Qthis, Qmin, Qmax, dR, dQ;

  Image *image;
  Measure *measure;
  Measure *measureNew;

  Matrix matrix;

  SkyTable *insky;
  SkyList *inlist;

  char *cpmFilenameSrc = NULL;
  char *cptFilenameSrc = NULL;
  char *cpsFilenameSrc = NULL;
  char *cpmFilenameTgt = NULL;
  char *cptFilenameTgt = NULL;
  char *cpsFilenameTgt = NULL;

  char *imageFilename = NULL;

  Header cpmHeaderPHU;
  Header cpmHeaderTBL;
  FTable cpmFtable;

  FILE *cpmFile = NULL;

  DVOCatFormat catformat;

  if (argc != 3) {
    fprintf (stderr, "USAGE: dvorepair -delete-image-list (catdir) (deleteList)\n");
    fprintf (stderr, "  catdir : database of interest\n");
    fprintf (stderr, "  deleteList : table from dvorepair giving images to be deleted\n");
    exit (2);
  }

  fprintf (stderr, "is this mode tested?\n");
  exit (2);

  char *catdir = argv[1];
  char *delList = argv[2];

  // load the image data
  ALLOCATE(imageFilename, char, strlen(catdir) + 12);
  sprintf (imageFilename, "%s/Images.dat", catdir);
  if ((image = LoadImages (&db, imageFilename, &Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage);

  // generate an index for imageIDs

  // find the full range of the imageID values
  Nindex = 0;
  for (i = 0; i < Nimage; i++) {
    Nindex = MAX(image[i].imageID, Nindex);
  }

  // create the index vector
  ALLOCATE (imageIdx, off_t, (Nindex + 1));
  memset (imageIdx, 0, (Nindex + 1)*sizeof(off_t));

  // assign the imageIDs to the index vector
  for (i = 0; i < Nimage; i++) {
    index = image[i].imageID;
    myAssert(index, "image index cannot be 0");
    myAssert(!imageIdx[index], "stomping on an earlier index");
    imageIdx[index] = i;
  }

  // generate an index of the deletion image
  int *deleteIndex;
  ALLOCATE (deleteIndex, int, Nindex + 1);
  memset (deleteIndex, 0, (Nindex + 1)*sizeof(int));

  int NdeleteIDs;
  int *deleteIDs = ReadDeleteList(delList, &NdeleteIDs);

  for (i = 0; i < NdeleteIDs; i++) {
    index = deleteIDs[i];
    myAssert(index > 0, "image index cannot be <= 0");
    myAssert(index <= Nindex, "delete index out of range of real data");
    myAssert(!deleteIndex[index], "stomping on an earlier index");
    deleteIndex[index] = TRUE;
  }

  // find the RA & DEC range of the images we want to delete
  Rmin = 360.0; 
  Rmax = 0.0;
  Dmin = +90.0;
  Dmax = -90.0;
  Qmin = 360.0;
  Qmax =   0.0;
  for (i = 0; i < NdeleteIDs; i++) {
    index = deleteIDs[i];
    seq = imageIdx[index];

    XY_to_RD(&Rthis, &Dthis, 0, 0, &image[seq].coords);
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qthis = (Rthis < 180.0) ? Rthis : Rthis - 360.0;
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    // fprintf (stderr, "image @ %f,%f\n", Rthis, Dthis);

    XY_to_RD(&Rthis, &Dthis, image[seq].NX, 0, &image[seq].coords);
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qthis = (Rthis < 180.0) ? Rthis : Rthis - 360.0;
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    XY_to_RD(&Rthis, &Dthis, 0, image[seq].NY, &image[seq].coords);
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qthis = (Rthis < 180.0) ? Rthis : Rthis - 360.0;
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);

    XY_to_RD(&Rthis, &Dthis, image[seq].NX, image[seq].NY, &image[seq].coords);
    Rmin = MIN(Rthis, Rmin);
    Rmax = MAX(Rthis, Rmax);
    Dmin = MIN(Dthis, Dmin);
    Dmax = MAX(Dthis, Dmax);
    Qthis = (Rthis < 180.0) ? Rthis : Rthis - 360.0;
    Qmin = MIN(Qthis, Qmin);
    Qmax = MAX(Qthis, Qmax);
  }

  dQ = Qmax - Qmin;
  dR = Rmax - Rmin;

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (catdir, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(insky, "can't read SkyTable");
  SkyTableSetFilenames (insky, catdir, "cpt");
  
  // XXX apply this...generate the subset matching the user-selected region
  SkyRegion UserPatch;

  if (dR < dQ) {
    fprintf (stderr, "R,D range: %f - %f, %f - %f\n", Rmin, Rmax, Dmin, Dmax);
    nPass = 1;
  } else {
    fprintf (stderr, "R,D range: %f - %f, %f - %f\n", Qmin, Qmax, Dmin, Dmax);
    nPass = 2;
  }
  
  ALLOCATE(cpmFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cptFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cpsFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cpmFilenameTgt, char, strlen(catdir) + 64);
  ALLOCATE(cptFilenameTgt, char, strlen(catdir) + 64);
  ALLOCATE(cpsFilenameTgt, char, strlen(catdir) + 64);

  for (raPass = 0; raPass < nPass; raPass++) {
    
    UserPatch.Dmin = Dmin + 0.1;
    UserPatch.Dmax = Dmax + 0.1;

    if (dR < dQ) {
      UserPatch.Rmin = Rmin;
      UserPatch.Rmax = Rmax;
    } else {
      if (raPass == 0) {
	UserPatch.Rmin = 0.0;
	UserPatch.Rmax = Qmax;
      } else {
	UserPatch.Rmin = Qmin + 360.0;
	UserPatch.Rmax = 360.0;
      }
    }

    inlist = SkyListByPatch (insky, -1, &UserPatch);
  
    // SkyListPopulatedRange (&Ns, &Ne, inlist, 0);
    // depth = inlist[0].regions[Ns][0].depth;
  
    // loop over the populated input regions
    Ncheck = 0;
    for (i = 0; i < inlist[0].Nregions; i++) {
      if (!inlist[0].regions[i][0].table) continue;

      sprintf (cpmFilenameSrc, "%s/%s.cpm", catdir, inlist[0].regions[i][0].name);
      sprintf (cptFilenameSrc, "%s/%s.cpt", catdir, inlist[0].regions[i][0].name);
      sprintf (cpsFilenameSrc, "%s/%s.cps", catdir, inlist[0].regions[i][0].name);
      sprintf (cpmFilenameTgt, "%s/%s.cpm.fixed", catdir, inlist[0].regions[i][0].name);
      sprintf (cptFilenameTgt, "%s/%s.cpt.fixed", catdir, inlist[0].regions[i][0].name);
      sprintf (cpsFilenameTgt, "%s/%s.cps.fixed", catdir, inlist[0].regions[i][0].name);
      cpmFtable.header = &cpmHeaderTBL;

      /*** read and examine the CPM file ***/
      {
	// fprintf (stderr, "check %s\n", cpmFilenameSrc);

	// open cpm file
	cpmFile = fopen(cpmFilenameSrc, "r");
	if (!cpmFile) continue;
	// myAssert(cpmFile, "failed to open cpm file");
    
	// load the cpm header
	if (!gfits_fread_header (cpmFile, &cpmHeaderPHU)) {
	  myAbort("failure to cpm header");
	}

	// move to TBL header
	Nbytes = cpmHeaderPHU.datasize + gfits_data_size (&cpmHeaderPHU);
	fseeko (cpmFile, Nbytes, SEEK_SET);

	// read cpm TBL header
	if (!gfits_fread_header (cpmFile, &cpmHeaderTBL)) { 
	  myAbort("can't read header for cpm table");
	}

	// read Measure table data : format is irrelevant here */
	if (!gfits_fread_ftable_data (cpmFile, &cpmFtable, TRUE)) { 
	  myAbort("can't read data for cpm table");
	}

	gfits_scan(&cpmHeaderTBL, "NAXIS1", "%d", 1, &NbytesPerRow);

	measure = FtableToMeasure (&cpmFtable, NULL, &Nmeasure, &catformat, FALSE);
	myAssert(measure, "failed to convert ftable to measure data");
    
	Nvalid = (int)(cpmFtable.validsize / NbytesPerRow);
	Nvalid = MIN(Nmeasure, Nvalid);

	// close the input cpm file
	fclose(cpmFile);

	// allocate an output array of measures (to replace, if needed)
	ALLOCATE (measureNew, Measure, Nvalid);

	NmeasureNew = 0;
	Ndelete = 0;

	// examine all measurements: find ones that need to be deleted
	for (j = 0; j < Nvalid; j++) {
	  index = measure[j].imageID;
	  myAssert(index, "measure is missing an image ID");

	  if (deleteIndex[index]) {
	    Ndelete ++;
	    continue;
	  }
	  measureNew[NmeasureNew] = measure[j];
	  NmeasureNew ++;
	}
      }

      fprintf (stderr, "deleting %d of %d (keep %d) detections from %s -> %s\n", (int) Ndelete, (int) Nvalid, (int) NmeasureNew, cpmFilenameSrc, cpmFilenameTgt);

      // if we actually want to delete any measurements, write out a new cpm file
      if (Ndelete > 0) {

	// the CPT and CPS tables need to be regenerated.  This must happen first because, in the process, we also update measure->averef
	RepairTableCPT(cptFilenameSrc, cptFilenameTgt, cpsFilenameSrc, cpsFilenameTgt, measureNew, NmeasureNew, image, Nimage, catformat);

	// convert internal to external format 
	if (!MeasureToFtable (&cpmFtable, NULL, measureNew, NmeasureNew, catformat, TRUE)) {
	  myAbort("trouble converting format");
	}

	// create and write the output file
	cpmFile = fopen(cpmFilenameTgt, "w");
	myAssert(cpmFile, "failed to open cpt file");
	
	// write PHU header
	if (!gfits_fwrite_header (cpmFile, &cpmHeaderPHU)) {
	  myAbort("can't write primary header");
	}

	// write the PHU matrix; this is probably a NOP, do I have to keep it in?
	gfits_create_matrix (&cpmHeaderPHU, &matrix);
	if (!gfits_fwrite_matrix  (cpmFile, &matrix)) {
	  myAbort("can't write primary matrix");
	}
	gfits_free_matrix (&matrix);
	
	// write the table data
	if (!gfits_fwrite_ftable_range (cpmFile, &cpmFtable, 0, NmeasureNew, 0, NmeasureNew)) {
	  myAbort("can't write table data");
	}
	fclose (cpmFile);
      }
      gfits_free_header (&cpmHeaderPHU);
      gfits_free_header (&cpmHeaderTBL);
      free (measure);
      free (measureNew);

      Ncheck ++;
      if (Ncheck % 1000 == 0) {
	fprintf (stderr, "%s...", inlist[0].regions[i][0].name);
      }
    }
    fprintf (stderr, "\n");
    SkyListFree(inlist);
  }

  free (imageFilename);
  free (imageIdx);
  free (deleteIndex);
  free (deleteIDs);

  free(cpmFilenameSrc);
  free(cptFilenameSrc);
  free(cpsFilenameSrc);
  free(cpmFilenameTgt);
  free(cptFilenameTgt);
  free(cpsFilenameTgt);

  SkyTableFree(insky);

  exit (0);
}

int RepairTableCPT(char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, char catformat) {

  off_t *averefMatch;
  off_t i, NaveMax, Naverage, NAVERAGE, NaverageOut, Nave, Nout, Nold;
  int *found, Nsecfilt;

  Image *thisImage;
  Average *average, *averageOut;

  Matrix matrix;

  Header cptHeaderPHU;
  Header cptHeaderTBL;
  FTable cptFtable;

  FILE *cptFileSrc = NULL;
  FILE *cptFileTgt = NULL;

  cptFtable.header = &cptHeaderTBL;

  NaveMax = 0;
  NAVERAGE = 1000;
  ALLOCATE (average, Average, NAVERAGE);
  memset (average, 0, NAVERAGE*sizeof(Average));

  ALLOCATE (found, int, NAVERAGE);
  memset (found, 0, NAVERAGE*sizeof(int));

  // examine all measurements and new objects as needed
  for (i = 0; i < Nmeasure; i++) {
    Nave = measure[i].averef;

    if (Nave >= NAVERAGE) {
      Nold = NAVERAGE;
      NAVERAGE = MAX(Nave + 1000, NAVERAGE + 1000);
      REALLOCATE (average, Average, NAVERAGE);
      memset (&average[Nold], 0, (NAVERAGE - Nold)*sizeof(Average));

      REALLOCATE (found, int, NAVERAGE);
      memset (&found[Nold], 0, (NAVERAGE - Nold)*sizeof(int));
    }

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
    thisImage = MatchImage (image, Nimage, measure[i].t, measure[i].photcode, measure[i].imageID);
    XY_to_RD (&average[Nave].R, &average[Nave].D, measure[i].Xccd, measure[i].Yccd, &thisImage[0].coords);
    average[Nave].R = ohana_normalize_angle (average[Nave].R);

    average[Nave].Nmeasure = 1;
    average[Nave].Nmissing = 0;

    // assume the resulting table set is unsorted
    average[Nave].measureOffset = -1;
    average[Nave].missingOffset = -1;
    average[Nave].refColorBlue = NAN;
    average[Nave].refColorRed = NAN;

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
  cptFileSrc = fopen(cptFilenameSrc, "r");
  myAssert(cptFileSrc, "failed to open cpt file");

  // load the cpt header (use for CATID, RA,DEC range, filenames)
  if (!gfits_fread_header (cptFileSrc, &cptHeaderPHU)) {
    myAbort("failure to cpt header");
  }

  // update the output header
  gfits_modify (&cptHeaderPHU, "NSTARS",     OFF_T_FMT, 1,  NaverageOut);
  gfits_modify (&cptHeaderPHU, "NMEAS",      OFF_T_FMT, 1,  Nmeasure);
  gfits_modify (&cptHeaderPHU, "NMISS",      "%d",      1,  0);
  gfits_modify_alt (&cptHeaderPHU, "SORTED", "%t",      1,  FALSE);

  gfits_scan (&cptHeaderPHU, "NSECFILT",     "%d",      1,  &Nsecfilt);

  myAbort ("test for compression");

  /* convert internal to external format */
  if (!AverageToFtable (&cptFtable, averageOut, NaverageOut, catformat, NULL, TRUE)) {
    myAbort("trouble converting format");
  }

  // create and write the output file
  cptFileTgt = fopen(cptFilenameTgt, "w");
  myAssert(cptFileTgt, "failed to open cpt file");
    
  // write PHU header
  if (!gfits_fwrite_header (cptFileTgt, &cptHeaderPHU)) {
    myAbort("can't write primary header");
  }

  // write the PHU matrix; this is probably a NOP, do I have to keep it in?
  gfits_create_matrix (&cptHeaderPHU, &matrix);
  if (!gfits_fwrite_matrix  (cptFileTgt, &matrix)) {
    myAbort("can't write primary matrix");
  }
  gfits_free_matrix (&matrix);

  // write the table data
  if (!gfits_fwrite_ftable_range (cptFileTgt, &cptFtable, 0, NaverageOut, 0, NaverageOut)) {
    myAbort("can't write table data");
  }
  
  fclose(cptFileTgt);
  fclose(cptFileSrc);

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

    FILE *cpsFileSrc = NULL;
    FILE *cpsFileTgt = NULL;

    SecFilt *secfilt = NULL;

    cpsFtable.header = &cpsHeaderTBL;

    // open source cpt file
    cpsFileSrc = fopen(cpsFilenameSrc, "r");
    myAssert(cpsFileSrc, "failed to open cps file");
    
    // load the cps header (use for CATID, RA,DEC range, filenames)
    if (!gfits_fread_header (cpsFileSrc, &cpsHeaderPHU)) {
      myAbort("failure to cps header");
    }

    int Nrows = Nsecfilt*NaverageOut;
    ALLOCATE (secfilt, SecFilt, Nrows);

    /* convert internal to external format */
    if (!SecFiltToFtable (&cpsFtable, secfilt, Nrows, catformat, TRUE)) {
      myAbort("trouble converting format");
    }

    // create and write the output file
    cpsFileTgt = fopen(cpsFilenameTgt, "w");
    myAssert(cpsFileTgt, "failed to open cps file");
    
    // write PHU header
    if (!gfits_fwrite_header (cpsFileTgt, &cpsHeaderPHU)) {
      myAbort("can't write primary header");
    }

    // write the PHU matrix; this is probably a NOP, do I have to keep it in?
    gfits_create_matrix (&cpsHeaderPHU, &matrix);
    if (!gfits_fwrite_matrix  (cpsFileTgt, &matrix)) {
      myAbort("can't write primary matrix");
    }
    gfits_free_matrix (&matrix);

    // write the table data
    if (!gfits_fwrite_ftable_range (cpsFileTgt, &cpsFtable, 0, Nrows, 0, Nrows)) {
      myAbort("can't write table data");
    }
  
    fclose(cpsFileTgt);
    fclose(cpsFileSrc);

    gfits_free_header (&cpsHeaderPHU);
    gfits_free_header (&cpsHeaderTBL);
    free (secfilt);
  }

  return (TRUE);
}

