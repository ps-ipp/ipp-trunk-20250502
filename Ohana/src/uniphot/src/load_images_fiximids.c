# include "fiximids.h"

off_t   Nimage = 0;
e_time *startImage = NULL;
e_time *stopImage = NULL;
short  *photcodeImage = NULL;
unsigned int *imageID = NULL;
unsigned int *imageSeq = NULL;
int *Nimage_valid = NULL;
int *Nimage_invalid = NULL;

Image *load_images_fiximids (FITS_DB *db, off_t *Nimage_load) {

  Image *image;

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* read entire db table */
  if (!dvo_image_load (db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db[0].filename);

  /* use a vtable to keep the images to be calibrated */
  image = gfits_table_get_Image (&db[0].ftable, Nimage_load, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  fprintf (stderr, "loaded "OFF_T_FMT" images\n", *Nimage_load);

  return (image);
}

// find mosaic frames (unique time periods & photcode name matches mosaic) 
void initImageIndex (ImageSubset *image, off_t Nimage_init) {

  off_t i;

  Nimage = Nimage_init;
  ALLOCATE (startImage, e_time, Nimage);
  ALLOCATE (stopImage, e_time, Nimage);
  ALLOCATE (photcodeImage, short, Nimage);
  ALLOCATE (imageID, unsigned int, Nimage);
  ALLOCATE (imageSeq, unsigned int, Nimage);

  ALLOCATE (Nimage_valid, int, Nimage);
  ALLOCATE (Nimage_invalid, int, Nimage);

  memset (Nimage_valid, 0, Nimage*sizeof(int));
  memset (Nimage_invalid, 0, Nimage*sizeof(int));

  // save the image data in the local static arrays
  for (i = 0; i < Nimage; i++) {

    /* set image time range (small boundary buffer) */
    e_time start = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    e_time stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    startImage[i] = start;
    stopImage[i] = stop;
    photcodeImage[i] = image[i].photcode;
    imageID[i] = image[i].imageID;
    imageSeq[i] = i;
  }

  // sort the index, start, and stop by the start times:
  sort_image_times (startImage, stopImage, photcodeImage, imageID, imageSeq, Nimage);
  
  return;
}

// I need an index to go from image to associated mosaic (if existant)
// This code is copied from relastro/src/MosaicOps.c

// find the image by time. this search makes the following assumptions:
// 1) there is only a single EXPOSURE from a single camera with the given exposure time
// 2) there may be multiple IMAGES from that exposure, distinguished by photcode
// 3) the image data has been sorted by start time
// 4) once we have (time > start[i]) we are past the images of interest
int FindImageID (off_t *ID, off_t *Seq, e_time time, short photcode) {

  off_t Nlo, Nhi, N, Ni;

  *ID = 0;
  *Seq = -1;

  // Find the last image ending BEFORE 'time' (time > start[i]).
  // Use bisection to find the overlapping images.
  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (stopImage[N] < time) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nimage);
    }
  }

  // check for the matched images starting from Nlo 
  for (N = Nlo; N < Nimage; N++) { 
    if (time > stopImage[N]) continue;
    if (time < startImage[N]) return FALSE;
    // a possible image match (start <= time <= stop) 
    for (Ni = N; (Ni < Nimage) && (time <= stopImage[Ni]); Ni++) {
      if (photcodeImage[Ni] == photcode) {
	*ID = imageID[Ni];
	*Seq = imageSeq[Ni];
	return TRUE;
      }
    }
    return FALSE;
  }
  return FALSE;
}

// increment the count of valid detections for this image
void BumpValidImage (int Seq) {
  myAssert (Seq >= 0, "impossible seq");
  myAssert (Seq < Nimage, "impossible seq");
  Nimage_valid[Seq] ++;
}

// increment the count of invalid detections for this image
void BumpInvalidImage (int Seq) {
  myAssert (Seq >= 0, "impossible seq");
  myAssert (Seq < Nimage, "impossible seq");
  Nimage_invalid[Seq] ++;
}

void CompareImageCounts (ImageSubset *image, off_t Nimage_comp) {

  off_t i;

  for (i = 0; i < Nimage_comp; i++) {
    // XXX can I limit the images to those in range of the user region?

    if (Nimage_valid[i] + Nimage_invalid[i] == 0) continue;     // XXX for now, skip (optionally?) images with no matched detections
    if (image[i].photcode == 0) continue; // skip images with 0 photcode (eg, PHU)

    char *date = ohana_sec_to_date (image[i].tzero);
    char *code = GetPhotcodeNamebyCode (image[i].photcode);
    fprintf (stderr, "image %s %s : %d  %d  %d  : %f\n", date, code, image[i].nstar, Nimage_valid[i], Nimage_invalid[i], 
	     (Nimage_valid[i] + Nimage_invalid[i]) / (float) image[i].nstar);
  }
}

void SummaryImageStats (ImageSubset *image, off_t Nimage_comp) {

  off_t i;

  // let's report the following:
  // Nimages with corrected image IDs
  // Nimages with Nsum / Nstar < 0.95
  // Nimages with Nsum / Nstar < 0.99

  int Nfixed = 0;
  int Nbad_1 = 0;
  int Nbad_2 = 0;

  for (i = 0; i < Nimage_comp; i++) {
    if (Nimage_valid[i] + Nimage_invalid[i] == 0) continue;     // XXX for now, skip (optionally?) images with no matched detections
    if (image[i].photcode == 0) continue; // skip images with 0 photcode (eg, PHU)

    if (Nimage_invalid[i] > 0) Nfixed ++;
    
    float found_ratio = (Nimage_valid[i] + Nimage_invalid[i]) / (float) image[i].nstar;
    
    if (found_ratio < 0.99) Nbad_1 ++;
    if (found_ratio < 0.95) Nbad_2 ++;
  }

  fprintf (stderr, "Nfixed: %d, Ndrop (>1%%) %d, Ndrop (>5%%) %d\n", Nfixed, Nbad_1, Nbad_2);
}

// sort two times vectors and an index by first time vector
void sort_image_times (e_time *S, e_time *E, short *C, unsigned int *I, unsigned int *Q, off_t N) {

# define SWAPFUNC(A,B){ e_time tmp_t; unsigned int tmp_u; short tmp_c;	\
  tmp_t = S[A]; S[A] = S[B]; S[B] = tmp_t; \
  tmp_t = E[A]; E[A] = E[B]; E[B] = tmp_t; \
  tmp_c = C[A]; C[A] = C[B]; C[B] = tmp_c; \
  tmp_u = I[A]; I[A] = I[B]; I[B] = tmp_u; \
  tmp_u = Q[A]; Q[A] = Q[B]; Q[B] = tmp_u; \
}
# define COMPARE(A,B)(S[A] < S[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

int ImageValidLoad(char *filename) {

  int Ncol;
  off_t i;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return FALSE;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return FALSE;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return FALSE;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    if (VERBOSE) fprintf (stderr, "can't read table header\n");
    fclose (f);
    return FALSE;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    if (VERBOSE) fprintf (stderr, "can't read table data\n");
    fclose (f);
    return FALSE;
  }
  fclose (f);

  char type[16];

  GET_COLUMN (Nvalid,     "NVALID",   int);
  GET_COLUMN (Ninvalid,   "NINVALID", int);

  myAssert (Nrow == Nimage, "wrong size imstats result file\n");

  // merge the new arrays into the single array?
  for (i = 0; i < Nrow; i++) {
    Nimage_valid[i] += Nvalid[i];
    Nimage_invalid[i] += Ninvalid[i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (Nvalid);
  free (Ninvalid);

  return TRUE;
}

int ImageValidSave(char *filename) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_VALID");

  // create the table layout
  gfits_define_bintable_column (&theader, "J",   "NVALID",     "tmp", 	   "tmp", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J",   "NINVALID",   "tmp", 	   "tmp", 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "NVALID",     Nimage_valid,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NINVALID",   Nimage_invalid, Nimage);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file for output %s\n", filename);
    return FALSE;
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);
  fflush (f);

  return TRUE;
}
