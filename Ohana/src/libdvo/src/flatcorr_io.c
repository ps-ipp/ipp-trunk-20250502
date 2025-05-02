# include "dvo.h"

// FlatCorrection -- description of a single flat-field correction image
// a flat-field correction is defined for a specific camera (defined how?) or photcode (this makes some sense)
// it has a period of validity

/* The flat-field correction is defined by two FITS tables / structures.  The first
 * (FlatCorrectionImage) describes the layout of the chips and which filters are being
 * corrected.  The second lists the actual corrections for each of the cells
 */

FlatCorrectionTable *FlatCorrectionLoad (char *filename, int VERBOSE) {

  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  FlatCorrectionTable *flatcorrTable;
  
  f = fopen (filename, "r");
  if (f == NULL) {
    if (VERBOSE) fprintf (stderr, "can't find Flat Correction file %s\n", filename);
    return (NULL);
  }

  /* load in table data */
  ftable.header = &theader;
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction header\n");
    fclose (f);
    return (NULL);
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return (NULL);
  }

  ALLOCATE (flatcorrTable, FlatCorrectionTable, 1);

  if (!gfits_fread_ftable (f, &ftable, "FLAT_CORRECTION_IMAGE")) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction table\n");
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    free (flatcorrTable);
    fclose (f);
    return (NULL);
  }
  flatcorrTable->image = gfits_table_get_FlatCorrectionImage (&ftable, &flatcorrTable->Nimage, NULL, NULL);
  if (!flatcorrTable->image) {
    fprintf (stderr, "ERROR: failed to read Flat Correction Images\n");
    exit (2);
  }
  gfits_free_header (&theader);
  // NOTE: do not free ftable or the data will be freed

  if (!gfits_fread_ftable (f, &ftable, "FLAT_CORRECTION")) {
    if (VERBOSE) fprintf (stderr, "can't read Flat Correction table\n");
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    fclose (f);
    return (NULL);
  }
  flatcorrTable->corr = gfits_table_get_FlatCorrection (&ftable, &flatcorrTable->Ncorr, NULL, NULL);
  if (!flatcorrTable->corr) {
    fprintf (stderr, "ERROR: failed to read Flat Corrections\n");
    exit (2);
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  // NOTE: do not free ftable or the data will be freed

  // convert the table format to map format:
  FlatCorrectionInternal (flatcorrTable);

  return (flatcorrTable);
}

int FlatCorrectionInternal(FlatCorrectionTable *flatcorrTable) {

  int i, j;

  // assert that the internal arrays are not yet allocated?

  // generate the arrays to hold the corrections & initialize
  ALLOCATE (flatcorrTable->offset, float **, flatcorrTable->Nimage);
  for (i = 0; i < flatcorrTable->Nimage; i++) {
    ALLOCATE (flatcorrTable->offset[i], float *, flatcorrTable->image[i].Nx);
    for (j = 0; j < flatcorrTable->image[i].Nx; j++) {
      ALLOCATE (flatcorrTable->offset[i][j], float, flatcorrTable->image[i].Ny);
      memset (flatcorrTable->offset[i][j], 0, flatcorrTable->image[i].Ny*sizeof(float));
    }
  }

  // create the lookup table (ID -> Seq)

  // find the max value of ID
  int MaxID = 0;
  for (i = 0; i < flatcorrTable->Nimage; i++) {
    MaxID = MAX(flatcorrTable->image[i].ID, MaxID);
  }
  MaxID ++; // we want the outer bound, not the last value

  // generate the index and init values to -1
  ALLOCATE (flatcorrTable->IDtoSeq, int, MaxID);
  for (i = 0; i < MaxID; i++) {
    flatcorrTable->IDtoSeq[i] = -1;
  }

  // assign the ID values for each image
  for (i = 0; i < flatcorrTable->Nimage; i++) {
    int ID = flatcorrTable->image[i].ID;
    assert (flatcorrTable->IDtoSeq[ID] == -1);
    flatcorrTable->IDtoSeq[ID] = i;
  }

  // assign the known specific correction values
  for (i = 0; i < flatcorrTable->Ncorr; i++) {
    int ID = flatcorrTable->corr[i].ID;
    int x = flatcorrTable->corr[i].x;
    int y = flatcorrTable->corr[i].y;
    int seq = flatcorrTable->IDtoSeq[ID];
    assert (seq != -1);

    flatcorrTable->offset[seq][x][y] = flatcorrTable->corr[i].offset;      
  }

  return TRUE;
}

int FlatCorrectionSave (FlatCorrectionTable *flatcorrTable, char *filename) {

  Header header;
  Matrix matrix;
  Header theaderImage;
  FTable ftableImage;

  Header theaderCorr;
  FTable ftableCorr;
  FILE *f;

  FlatCorrectionImage *image;
  FlatCorrection *corr;

  // these output functions byteswap their buffers. make a copy for output
  ALLOCATE (image, FlatCorrectionImage, flatcorrTable->Nimage);
  memcpy (image, flatcorrTable->image, flatcorrTable->Nimage*sizeof(FlatCorrectionImage));

  ALLOCATE (corr, FlatCorrection, flatcorrTable->Ncorr);
  memcpy (corr, flatcorrTable->corr, flatcorrTable->Ncorr*sizeof(FlatCorrection));

  /* make phu header (no matrix needed) */
  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  ftableImage.header = &theaderImage;
  gfits_table_set_FlatCorrectionImage (&ftableImage, image, flatcorrTable->Nimage, TRUE);

  ftableCorr.header = &theaderCorr;
  gfits_table_set_FlatCorrection (&ftableCorr, corr, flatcorrTable->Ncorr, TRUE);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) { 
    fprintf (stderr, "cannot open %s for output\n", filename);
    return (FALSE);
  }
  
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theaderImage);
  gfits_fwrite_table  (f, &ftableImage);
  gfits_fwrite_Theader (f, &theaderCorr);
  gfits_fwrite_table  (f, &ftableCorr);
  fclose (f);

  return (TRUE);
}

// eddie uses cell_x = (int) (X + X_PAD) / (base + X_PAD)
// where base = 600, X_PAD = 8, Y_PAD = 10.  denominator is thus (608,610), whic is (4880/8),(4864/8)
// these match Eddie's NCELL_X,Y / CHIP_DX,DY, so I think we are good with this:

# define X_PAD 8
# define Y_PAD 10

float FlatCorrectionOffset (FlatCorrectionTable *flatcorr, int ID, int X, int Y) {

  if (!flatcorr) return 0.0;

  // validate the flat_id (not out of range?)
  int seq = flatcorr->IDtoSeq[ID];

  // convert X,Y to Xbin, Ybin:
  int Xbin = MAX(MIN((X + X_PAD) * flatcorr->image[seq].Nx / flatcorr->image[seq].DX, flatcorr->image[seq].Nx - 1), 0);
  int Ybin = MAX(MIN((Y + Y_PAD) * flatcorr->image[seq].Ny / flatcorr->image[seq].DY, flatcorr->image[seq].Ny - 1), 0);

  // XXX warn if X,Y are out of range? not super important unless *way* out of range

  float offset = flatcorr->offset[seq][Xbin][Ybin];
  return offset;
}

// other needed operations:
// * assign an image to a flatcorr (based on photcode and obs_date)
