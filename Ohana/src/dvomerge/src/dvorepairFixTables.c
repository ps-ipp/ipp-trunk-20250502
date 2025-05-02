# include "dvomerge.h"

// repair short cpm files, fix inconsistent cpt / cps / cpm files
// * scan over all catalog files in the specified RA & DEC range
// * load the cpm file (pad if short, identify the padded section)
// * create a new cpm file
// * loop over detections : only keep those not from images to be deleted
// * load the cpt file
// * rebuild the cpt file or delete the matching entries?

int dvorepairDeleteImageList (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table

  off_t i, j, Nmeasure, NmeasureNew, Ndelete, Nvalid, Nimage, Nindex, *imageIdx, index;
  int N, seq, Nbytes, NbytesPerRow, Ncheck, nPass, raPass;
  double Rthis, Dthis, Rmin, Rmax, Dmin, Dmax, Qthis, Qmin, Qmax, dR, dQ;

  Measure *measure;
  Measure *measureNew;

  SkyTable *insky;
  SkyList *inlist;

  char *cpmFilenameSrc = NULL;
  char *cptFilenameSrc = NULL;
  char *cpsFilenameSrc = NULL;
  char *cpmFilenameTgt = NULL;
  char *cptFilenameTgt = NULL;
  char *cpsFilenameTgt = NULL;

  Header cpmHeaderPHU;
  Header cpmHeaderTBL;
  FTable cpmFtable;

  FILE *cpmFile = NULL;

  char catformat;

  N = get_argument (argc, argv, "-fix-tables");
  myAssert(N == 1, "programming error: -fix-tables must be first from main");
  remove_argument (N, &argc, argv);

  // restrict to a portion of the sky
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: dvorepair -fix-tables (catdir) [-region Rmin Rmax Dmin Dmax]\n");
    fprintf (stderr, "  catdir : database of interest\n");
    exit (2);
  }

  char *catdir = argv[1];

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (catdir, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(insky, "can't read SkyTable");
  SkyTableSetFilenames (insky, catdir, "cpt");
  inlist = SkyListByPatch (insky, -1, &UserPatch);
  
  ALLOCATE(cpmFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cptFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cpsFilenameSrc, char, strlen(catdir) + 64);
  ALLOCATE(cpmFilenameTgt, char, strlen(catdir) + 64);
  ALLOCATE(cptFilenameTgt, char, strlen(catdir) + 64);
  ALLOCATE(cpsFilenameTgt, char, strlen(catdir) + 64);

  // loop over the tables
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

    measure = LoadTableCPM(cpmFilenameSrc, &Nmeasure, &Nexpect);

    // the CPT and CPS tables need to be regenerated.  This must happen first because, in the process, we also update measure->averef
    RepairTableCPT(cptFilenameSrc, cptFilenameTgt, cpsFilenameSrc, cpsFilenameTgt, measureNew, NmeasureNew, image, Nimage, catformat);

    // if the file is short, create
    if (Nmeasure != Nexpect) { 
      fprintf (stderr, "deleting %d of %d (keep %d) detections from %s -> %s\n", (int) Ndelete, (int) Nvalid, (int) NmeasureNew, cpmFilenameSrc, cpmFilenameTgt);

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
  SkyListFree(inlist);
  SkyTableFree(insky);

  free(cpmFilenameSrc);
  free(cptFilenameSrc);
  free(cpsFilenameSrc);
  free(cpmFilenameTgt);
  free(cptFilenameTgt);
  free(cpsFilenameTgt);

  exit (0);
}

// load this CPM file : is it short?
Measure *LoadTableCPM (char *cpmFilenameSrc, off_t *nmeasure, off_t *nexpect) {

  fprintf (stderr, "check %s\n", cpmFilenameSrc);

  // open cpm file
  cpmFile = fopen(cpmFilenameSrc, "r");
  if (!cpmFile) continue;
    
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
  gfits_scan(&cpmHeaderTBL, "NAXIS2", "%d", 1, &Nrows);

  measure = FtableToMeasure (&cpmFtable, NULL, &Nmeasure, &catformat, FALSE);
  myAssert(measure, "failed to convert ftable to measure data");
    
  Nvalid = (int)(cpmFtable.validsize / NbytesPerRow);
  Nvalid = MIN(Nmeasure, Nvalid);

  // close the input cpm file
  fclose(cpmFile);

  myAssert(Nrows >= Nvalid, "how can we find more entries than expected??");

  *nmeasure = Nvalid;
  *nexpect = Nrows;

  return measure;
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
    average[Nave].refColorRef = NAN;

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

