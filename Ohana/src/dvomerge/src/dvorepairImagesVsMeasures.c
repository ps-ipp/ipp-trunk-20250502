# include "dvomerge.h"

// find images which are missing detections:
// * load the Images.dat file
// * create an index for imageID (seq = imageIDindex[i])
// * scan over all catalog files
// * load the cpm file (pad if short, identify the padded section)
// * loop over detections: increment detection count for each imageID

int dvorepairImagesVsMeasures (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table

  off_t i, j, Nmeasure, Nvalid, Nimage, Nindex, *imageIdx, index, imageSeq;
  int Nbytes, NbytesPerRow, Nbad, Ncheck, Ntol;
  int *detCounts;

  Image *image;
  Measure *measure;

  SkyTable *insky;
  SkyList *inlist;

  char *catdir = NULL;
  char *cpmFilename = NULL;
  char *imageFilename = NULL;

  Header cpmHeaderPHU;
  Header cpmHeaderTBL;
  FTable cpmFtable;

  FILE *cpmFile = NULL;

  DVOCatFormat catformat;

  if (argc != 3) {
    fprintf (stderr, "USAGE: dvorepair -images-vs-measures (catdir) (Ntol)\n");
    fprintf (stderr, "  catdir : database of interest\n");
    fprintf (stderr, "  Ntol : allow Ntol missing detections\n");
    exit (2);
  }

  fprintf (stderr, "is this mode tested?\n");
  exit (2);

  catdir = argv[1];
  Ntol = atoi(argv[2]);

  // load the image data
  ALLOCATE(imageFilename, char, strlen(catdir) + 12);
  sprintf (imageFilename, "%s/Images.dat", catdir);
  if ((image = LoadImages (&db, imageFilename, &Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage);

  // generate an index for imageIDs:
  Nindex = 0;
  for (i = 0; i < Nimage; i++) {
    Nindex = MAX(image[i].imageID, Nindex);
  }
  ALLOCATE (imageIdx, off_t, (Nindex + 1));
  memset (imageIdx, 0, (Nindex + 1)*sizeof(off_t));

  for (i = 0; i < Nimage; i++) {
    index = image[i].imageID;
    if (index == 0) {
      fprintf (stderr, "?");
      continue;
    }
    if (imageIdx[index]) {
      fprintf (stderr, "!");
      continue;
    }
    imageIdx[index] = i;
  }

  // generate a list of the detection counts:
  ALLOCATE (detCounts, int, Nimage);
  memset (detCounts, 0, Nimage*sizeof(int));

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (catdir, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(insky, "can't read SkyTable");
  SkyTableSetFilenames (insky, catdir, "cpt");

  // XXX apply this...generate the subset matching the user-selected region
  SkyRegion UserPatch;
  UserPatch.Rmin = 0.0;
  UserPatch.Rmax = 360.0;
  UserPatch.Dmin = -90.0;
  UserPatch.Dmax = +90.0;
  inlist = SkyListByPatch (insky, -1, &UserPatch);

  // SkyListPopulatedRange (&Ns, &Ne, inlist, 0);
  // depth = inlist[0].regions[Ns][0].depth;
  
  ALLOCATE(cpmFilename, char, strlen(catdir) + 64);

  // loop over the populated input regions
  Ncheck = 0;
  for (i = 0; i < inlist[0].Nregions; i++) {
    if (!inlist[0].regions[i][0].table) continue;

    if (1) {
      sprintf (cpmFilename, "%s/%s.cpm", catdir, inlist[0].regions[i][0].name);
      cpmFtable.header = &cpmHeaderTBL;

      // open cpm file
      cpmFile = fopen(cpmFilename, "r");
      if (!cpmFile) continue;
      // myAssert(cpmFile, "failed to open cpm file");
    
      // fprintf (stderr, "input: %s\n", inlist[0].regions[i][0].name);

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

      // examine all measurements and new objects as needed
      for (j = 0; j < Nvalid; j++) {
	index = measure[j].imageID;
	if (!index) {
	  fprintf (stderr, "?");
	  continue;
	}

	imageSeq = imageIdx[index];
	// XXX check the range?

	detCounts[imageSeq] ++;
      }
      fclose(cpmFile);
      gfits_free_header (&cpmHeaderPHU);
      gfits_free_header (&cpmHeaderTBL);
      free (measure);

      Ncheck ++;
      if (Ncheck % 1000 == 0) {
	fprintf (stderr, "%s...", inlist[0].regions[i][0].name);
      }
    }
  }
  fprintf (stderr, "\n");

  Nbad = 0;
  for (i = 0; i < Nimage; i++) {
    // careful: off_t math does not do well with subtractions...
    if (detCounts[i] + Ntol < image[i].nstar) {
      fprintf (stdout, "image %s (%d) : %d vs %d\n", image[i].name, image[i].imageID, detCounts[i], image[i].nstar);
      Nbad ++;
    }
  }
  if (!Nbad) {
    fprintf (stderr, "no bad images found\n");
  }

  exit (0);
}
