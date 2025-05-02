# include "checkastro.h"

// we have an array of images: image[ImageIndex] (ImageIndex : 0 < Nimage)
static Image        *image;   // list of available images
static off_t        Nimage;   // number of available images

// if we read only a subset of the rows from the Image FITS, LineNumber tells us to which row
// each image belongs
static off_t       *LineNumber; // match of subset to full image table

static off_t        *N_onImage;   // number of measurements on image
static off_t        *N_ONIMAGE;   // allocated number of measurements on image   
static off_t        *Nref_onImage;   // number of reference measurements on image

static IDX_T       **MeasureToImage;     // link from catalog,measure to image
static IDX_T       **ImageToCatalog;   // catalog which supplied measurement on image
static IDX_T       **ImageToMeasure;   // measure reference for measurement on image

// to search by image ID, we sort (imageIDs, imageIdx) by imageIDs for an index
static off_t        *imageIDs; // list of all image IDs
static off_t        *imageIdx; // list of index for image IDs 

// generate the image ID index (for imageID -> image[i] lookups)
void initImages (Image *input, off_t *line_number, off_t N) {

  off_t i;

  image = input;
  LineNumber = line_number;
  Nimage = N;

  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIdx, off_t, Nimage);

  for (i = 0; i < Nimage; i++) {
    imageIdx[i] = i;
    imageIDs[i] = image[i].imageID;
  }
  llsortpair (imageIDs, imageIdx, Nimage);
}

off_t getImageByID (off_t ID) {

  // we have a pair of vectors (imageIDs, imageIdx) sorted by imageIDs
  // use bisection to find the specified image ID

  off_t Nlo, Nhi, N;

  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (imageIDs[N] < ID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nimage);
    }
  }

  for (N = Nlo; N < Nhi; N++) {
    if (imageIDs[N] == ID)
      return (imageIdx[N]);
  }

  return (-1);
}

// these are really image & catalog indexes
void initImageBins (Catalog *catalog, int Ncatalog) {

  off_t i, j;

  ALLOCATE (MeasureToImage, IDX_T *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (MeasureToImage[i], IDX_T, MAX (catalog[i].Nmeasure, 1));
    for (j = 0; j < catalog[i].Nmeasure; j++) MeasureToImage[i][j] = -1;
  }

  ALLOCATE (N_onImage,      off_t,   Nimage);
  ALLOCATE (N_ONIMAGE,      off_t,   Nimage);
  ALLOCATE (Nref_onImage,   off_t,   Nimage);
  ALLOCATE (ImageToCatalog, IDX_T *, Nimage);
  ALLOCATE (ImageToMeasure, IDX_T *, Nimage);

  for (i = 0; i < Nimage; i++) {
    N_onImage[i] =   0;
    N_ONIMAGE[i] =  30;
    Nref_onImage[i]   = 0;
    ImageToCatalog[i] = NULL;  // we allocate these iff they are needed in matchImage
    ImageToMeasure[i] = NULL;  // we allocate these iff they are needed in matchImage
  }
}

void freeImageBins (int Ncatalog) {

  off_t i;

  for (i = 0; i < Ncatalog; i++) {
    free (MeasureToImage[i]);
  }
  free (MeasureToImage);
  for (i = 0; i < Nimage; i++) {
    if (ImageToCatalog[i]) { free (ImageToCatalog[i]); }
    if (ImageToMeasure[i]) { free (ImageToMeasure[i]); }
  }
  free (ImageToCatalog);
  free (ImageToMeasure);
  free (N_onImage);
  free (N_ONIMAGE);
}

/* match measurements to images */
void findImages (Catalog *catalog, int Ncatalog) {

  off_t i, j;
  char *name;

  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Nmeasure; j++) {
      if (NphotcodesSkip > 0) {
	int k;
	int found = FALSE;
	for (k = 0; (k < NphotcodesSkip) && !found; k++) {
	  if (photcodesSkip[k][0].code == catalog[i].measureT[j].photcode) found = TRUE;
	  if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(catalog[i].measureT[j].photcode)) found = TRUE;
	}
	if (found) continue;
      }  
      matchImage (catalog, j, i);
    }
  }

  char output[128];
  snprintf (output, 128, "checkastrom.%+3.0f.%+3.0f.%03.0f.%03.0f.dat", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
  FILE *f = fopen (output, "w");
  if (!f) {
    fprintf (stderr, "problem opening %s for output\n", output);
  }

  // watch for chips with few stars
  int Nfew = 0;
  int Nbad = 0;
  for (i = 0; i < Nimage; i++) {
    // ignore the PHU entries
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    name = GetPhotcodeNamebyCode (image[i].photcode);

    /* only check exposure center */
    double Rexp = NAN, Dexp = NAN, Rccd = NAN, Dccd = NAN;

    XY_to_RD (&Rccd, &Dccd, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
    XY_to_RD (&Rexp, &Dexp, 0.0, 0.0, &image[i].parent->coords);

    char *date = ohana_sec_to_date(image[i].tzero);
    FILE *outfile = f ? f : stderr;
    fprintf (outfile, "%d %s : %10.6f %10.6f : %10.6f %10.6f "OFF_T_FMT" "OFF_T_FMT" %d %s %s\n",  
	     image[i].imageID, image[i].name, Rccd, Dccd, Rexp, Dexp, N_onImage[i], Nref_onImage[i], image[i].nstar,
	     date, name);
    free (date);

    if (N_onImage[i] < 20) {
      Nfew ++;
    } 
    if (N_onImage[i] < 15) {
      Nbad ++;
    } 
  }
  if (f) { fclose (f); }
  fprintf (stderr, OFF_T_FMT" total images, %d with < 20 measurements, %d with < 15\n", Nimage, Nfew, Nbad);
}

// this is the imageID-based match
void matchImage (Catalog *catalog, off_t meas, int cat) {

  off_t idx, ID;
  MeasureTiny *measure;

  measure = &catalog[cat].measureT[meas];
  unsigned int averef = measure->averef;
  Average *average = &catalog[cat].average[averef];
  assert (meas >= average->measureOffset);
  assert (meas <  average->measureOffset + average->Nmeasure);

  ID = measure[0].imageID;

  if (catalog[cat].measure) {
    Measure *measureBig = &catalog[cat].measure[meas];
    int TESTPT = FALSE;
    TESTPT |= CAT_ID_SRC && OBJ_ID_SRC && (measureBig->imageID == CAT_ID_SRC) && (measureBig->detID == OBJ_ID_SRC);
    TESTPT |= CAT_ID_DST && OBJ_ID_DST && (measureBig->imageID == CAT_ID_DST) && (measureBig->detID == OBJ_ID_DST);
    if (TESTPT) {
      fprintf (stderr, "got test det\n");
    }
  }

  idx = getImageByID (ID);
  if (idx == -1) {
    if (VERBOSE2) fprintf (stderr, "can't match detection to image?\n");
    return;
  }

  // index for (catalog, measure) -> image
  MeasureToImage[cat][meas] = idx;

  // if we need to allocate an image index table, do so here
  if (!ImageToCatalog[idx]) {
      ALLOCATE (ImageToCatalog[idx], IDX_T, N_ONIMAGE[idx]);
  }
  if (!ImageToMeasure[idx]) {
      ALLOCATE (ImageToMeasure[idx], IDX_T, N_ONIMAGE[idx]);
  }

  // index for image, Nentry -> catalog
  ImageToCatalog[idx][N_onImage[idx]] = cat;

  // index for image, Nentry -> measure
  ImageToMeasure[idx][N_onImage[idx]] = meas;

  // in bcatalog, we set this flag if the object includes a reference photcode
  if (average->flags & 0x10) {
    Nref_onImage[idx] ++;
  }

  // count the number of measurements on this image:
  N_onImage[idx] ++;

  if (N_onImage[idx] == N_ONIMAGE[idx]) {
    N_ONIMAGE[idx] += 30;
    REALLOCATE (ImageToCatalog[idx], IDX_T, N_ONIMAGE[idx]);
    REALLOCATE (ImageToMeasure[idx], IDX_T, N_ONIMAGE[idx]);
  }

  return;
}

void dump_measures (Average *average, Measure *measure) {

  off_t j, off;

  for (j = 0; j < average[0].Nmeasure; j++) {
    off = average[0].measureOffset + j;
    fprintf (stderr, "dR, dD, mag, dMag: %f, %f, %f, %f\n", measure[off].dR, measure[off].dD, measure[off].M, measure[off].dM);
  }
  return;
}

