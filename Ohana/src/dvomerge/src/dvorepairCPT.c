# include "dvomerge.h"

// broken cpt file, valid cpm file: we can recover everything in the cpt file from the cpm file:
// * load the full cpm file
// * loop over detections
// * if ave_ref is new: create a new object
//   * determine RA & DEC from ext_id
//   * obj_id, cat_id are defined in detection
//   * 

// XXX absorb this code into a unified dvorepair program

int main (int argc, char **argv) {

  off_t Nmeasure, Nimage;
  int i, Nbytes, NaveMax, Naverage, NAVERAGE, Nave, Nold;
  int *found;

  Image *image, *thisImage;
  Average *average;
  Measure *measure;

  Matrix matrix;

  char *cpmFilename, *cptFilenameSrc, *cptFilenameTgt, *imageFilename;

  Header cptHeaderPHU, cptHeaderTBL;
  Header cpmHeaderPHU, cpmHeaderTBL;
  FTable cpmFtable, cptFtable;

  FILE *cptFileSrc = NULL;
  FILE *cptFileTgt = NULL;
  FILE *cpmFile = NULL;

  char catformat;

  if (argc != 5) {
    fprintf (stderr, "USAGE: dvorepair (images) (cpm) (cptInput) (cptOutput)\n");
    exit (2);
  }

  SetSignals ();

  imageFilename  = argv[1];
  cpmFilename    = argv[2];
  cptFilenameSrc = argv[3];
  cptFilenameTgt = argv[4];

  cpmFtable.header = &cpmHeaderTBL;
  cptFtable.header = &cptHeaderTBL;

  // XXX don't bother locking for now: this is totally manual..

  // load the image data
  if ((image = LoadImages (imageFilename, &Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage);

  // open cpm file
  cpmFile = fopen(cpmFilename, "r");
  myAssert(cpmFile, "failed to open cpm file");

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
  if (!gfits_fread_ftable_data (cpmFile, &cpmFtable, FALSE)) { 
    myAbort("can't read data for cpm table");
  }

  measure = FtableToMeasure (&cpmFtable, NULL, &Nmeasure, &catformat, FALSE);
  myAssert(measure, "failed to convert ftable to measure data");

  NaveMax = 0;
  NAVERAGE = 1000;
  ALLOCATE (average, Average, NAVERAGE);
  memset (average, 0, NAVERAGE*sizeof(Average));

  ALLOCATE (found, int, NAVERAGE);
  memset (found, 0, NAVERAGE*sizeof(int));

  // examine all measurements and new objects as needed
  for (i = 0; i < Nmeasure; i++) {
    Nave = measure[i].averef;
    if (found[Nave]) {
      average[Nave].Nmeasure ++;
      myAssert(average[Nave].objID == measure[i].objID, "objIDs do not match!");
      myAssert(average[Nave].catID == measure[i].catID, "catIDs do not match!");
      continue;
    }

    if (Nave >= NAVERAGE) {
      Nold = NAVERAGE;
      NAVERAGE = MAX(Nave + 1000, NAVERAGE + 1000);
      REALLOCATE (average, Average, NAVERAGE);
      memset (&average[Nold], 0, (NAVERAGE - Nold)*sizeof(Average));

      REALLOCATE (found, int, NAVERAGE);
      memset (&found[Nold], 0, (NAVERAGE - Nold)*sizeof(int));
    }

    NaveMax = MAX(Nave, NaveMax);

    found[Nave] = TRUE;

    // we are going to leave most of the elements of average unset: they are the result of 
    // the relastro analysis for this object and can be recreated with a call to relastro

    // fields we have to set:

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

  // have we created all objects in the range 0 - Naverage?
  for (i = 0; i < Naverage; i++) {
    myAssert(found[i], "failed to find one");
  }

  // open source cpt file
  cptFileSrc = fopen(cptFilenameSrc, "r");
  myAssert(cptFileSrc, "failed to open cpt file");

  // load the cpt header (use for CATID, RA,DEC range, filenames)
  if (!gfits_fread_header (cptFileSrc, &cptHeaderPHU)) {
    myAbort("failure to cpt header");
  }

  // update the output header
  gfits_modify (&cptHeaderPHU, "NSTARS",     "%d",      1,  Naverage);
  gfits_modify (&cptHeaderPHU, "NMEAS",      OFF_T_FMT, 1,  Nmeasure);
  gfits_modify (&cptHeaderPHU, "NMISS",      "%d",      1,  0);
  gfits_modify_alt (&cptHeaderPHU, "SORTED", "%t",      1,  FALSE);

  myAbort ("test for compression");

  /* convert internal to external format */
  if (!AverageToFtable (&cptFtable, average, Naverage, catformat, NULL, TRUE)) {
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
  if (!gfits_fwrite_ftable_range (cptFileTgt, &cptFtable, 0, Naverage, 0, Naverage)) {
    myAbort("can't write table data");
  }
  
  fclose(cptFileTgt);
  fclose(cptFileSrc);
  fclose(cpmFile);

  exit (0);
}
