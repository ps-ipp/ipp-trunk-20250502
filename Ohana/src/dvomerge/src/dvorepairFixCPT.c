# include "dvomerge.h"

// broken cpt file, valid cpm file: we can recover everything in the cpt file from the cpm file:
// * load the full cpm file
// * loop over detections
// * if ave_ref is new: create a new object
//   * determine RA & DEC from ext_id
//   * obj_id, cat_id are defined in detection

int FixCPTfile (char *rootname, Image *image, off_t Nimage);

int dvorepairFixCPT (int argc, char **argv) {

  int i, j, Nfiles, NFILES;
  off_t Nimage;
  FITS_DB db;  // database handle pointing to input image table
  Image *image;
  char *listFilename, *imageFilename;
  char **files;

  if (argc != 3) {
    fprintf (stderr, "USAGE: dvorepair -fix-cpt (images) (filelist)\n");
    exit (2);
  }

  fprintf (stderr, "is this mode tested?\n");
  exit (2);

  imageFilename  = argv[1];
  listFilename   = argv[2];

  // XXX don't bother locking for now: this is totally manual..

  // load the image data
  if ((image = LoadImages (&db, imageFilename, &Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage);

  FILE *file = fopen (listFilename, "r");
  myAssert(file, "failed to open list");

  Nfiles = 0;
  NFILES = 100;
  ALLOCATE(files, char *, NFILES);
  for (i = 0; i < NFILES; i++) {
    ALLOCATE (files[i], char, 256);
    memset (files[i], 0, 256);
  }

  for (i = 0; TRUE; i++) {
    if (fscanf (file, "%s", files[i]) == EOF) {
      break;
    }
    if (i == NFILES - 1) {
      NFILES += 100;
      REALLOCATE(files, char *, NFILES);
      for (j = i + 1; j < NFILES; j++) {
	ALLOCATE (files[j], char, 256);
	memset (files[j], 0, 256);
      }
    }
  }
  Nfiles = i;
  fclose(file);
  
  for (i = 0; i < Nfiles; i++) {
    FixCPTfile(files[i], image, Nimage);
  }

  exit (0);
}

// fix the CPT file.  in the process, we rewrite the CPM file so that the averef entries are correctly set.
int FixCPTfile (char *rootFilename, Image *image, off_t Nimage) {

  char cptFilenameSrc[256], cptFilenameTgt[256], cpsFilenameSrc[256], cpsFilenameTgt[256], cpmFilenameSrc[256], cpmFilenameTgt[256];

  Header cpmHeaderPHU, cpmHeaderTBL;
  FTable cpmFtable;
  FILE  *cpmFile = NULL;

  off_t Nbytes, Nmeasure;

  Measure *measure;

  Matrix matrix;

  DVOCatFormat catformat;

  cpmFtable.header = &cpmHeaderTBL;

  sprintf (cpmFilenameSrc, "%s.cpm", rootFilename);
  sprintf (cpmFilenameTgt, "%s.cpm.fixed", rootFilename);
  sprintf (cptFilenameSrc, "%s.cpt", rootFilename);
  sprintf (cptFilenameTgt, "%s.cpt.fixed", rootFilename);
  sprintf (cpsFilenameSrc, "%s.cps", rootFilename);
  sprintf (cpsFilenameTgt, "%s.cps.fixed", rootFilename);

  // open cpm file
  cpmFile = fopen(cpmFilenameSrc, "r");
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
  // XXX this should not be running on broken CPM files
  if (!gfits_fread_ftable_data (cpmFile, &cpmFtable, FALSE)) { 
    myAbort("can't read data for cpm table");
  }

  myAbort ("fix cpts");
  measure = FtableToMeasure (&cpmFtable, NULL, &Nmeasure, &catformat, FALSE);
  myAssert(measure, "failed to convert ftable to measure data");

  // the CPT and CPS tables need to be regenerated.  This must happen first because, in the process, we also update measure->averef
  RepairTableCPT(cptFilenameSrc, cptFilenameTgt, cpsFilenameSrc, cpsFilenameTgt, measure, Nmeasure, image, Nimage, catformat);

  // convert internal to external format 
  if (!MeasureToFtable (&cpmFtable, NULL, measure, Nmeasure, catformat, TRUE)) {
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
  if (!gfits_fwrite_ftable_range (cpmFile, &cpmFtable, 0, Nmeasure, 0, Nmeasure)) {
    myAbort("can't write table data");
  }

  gfits_free_header (&cpmHeaderPHU);
  gfits_free_header (&cpmHeaderTBL);
  free (measure);
  fclose(cpmFile);
  
  return TRUE;
}
