# include "delstar.h"

off_t   Nimage = 0;
e_time *startImage_exp = NULL;
e_time *stopImage_exp = NULL;
short  *photcodeImage_exp = NULL;
unsigned int *imageID_exp = NULL;
unsigned int *imageSeq_exp = NULL;

unsigned int *externID_stk = NULL;
short  *photcodeImage_stk = NULL;
unsigned int *imageID_stk = NULL;
unsigned int *imageSeq_stk = NULL;

int maxID = 0;
int *imageIndex = NULL;

int *Nimage_valid = NULL;
int *Nimage_invalid = NULL;

void sort_image_externID (unsigned int *E, short *C, unsigned int *I, unsigned int *Q, off_t N);
void sort_image_times (e_time *S, e_time *E, short *C, unsigned int *I, unsigned int *Q, off_t N);

// find mosaic frames (unique time periods & photcode name matches mosaic) 
void initImageIndex (ImageSubset *image, off_t Nimage_init) {

  off_t i;

  Nimage = Nimage_init;
  ALLOCATE (startImage_exp, e_time, Nimage);
  ALLOCATE (stopImage_exp, e_time, Nimage);
  ALLOCATE (photcodeImage_exp, short, Nimage);
  ALLOCATE (imageID_exp, unsigned int, Nimage);
  ALLOCATE (imageSeq_exp, unsigned int, Nimage);

  ALLOCATE (externID_stk, unsigned int, Nimage);
  ALLOCATE (photcodeImage_stk, short, Nimage);
  ALLOCATE (imageID_stk, unsigned int, Nimage);
  ALLOCATE (imageSeq_stk, unsigned int, Nimage);

  ALLOCATE (Nimage_valid, int, Nimage);
  ALLOCATE (Nimage_invalid, int, Nimage);
  memset (Nimage_valid, 0, sizeof(int)*Nimage);
  memset (Nimage_invalid, 0, sizeof(int)*Nimage);

  // save the image data in the local static arrays
  for (i = 0; i < Nimage; i++) {

    // set image time range (small boundary buffer) 
    e_time start = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    e_time stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    startImage_exp[i] = start;
    stopImage_exp[i] = stop;
    photcodeImage_exp[i] = image[i].photcode;
    imageID_exp[i] = image[i].imageID;
    imageSeq_exp[i] = i;

    externID_stk[i] = image[i].externID;
    photcodeImage_stk[i] = image[i].photcode;
    imageID_stk[i] = image[i].imageID;
    imageSeq_stk[i] = i;

    maxID = MAX(image[i].imageID, maxID);
  }

  // sort the index, start, and stop by the start times:
  sort_image_times (startImage_exp, stopImage_exp, photcodeImage_exp, imageID_exp, imageSeq_exp, Nimage);

  // sort the index, start, and stop by the start times:
  sort_image_externID (externID_stk, photcodeImage_stk, imageID_stk, imageSeq_stk, Nimage);
  
  ALLOCATE (imageIndex, int, maxID + 1);
  for (i = 0; i < maxID + 1; i++) {
    imageIndex[i] = -1;
  }
  for (i = 0; i < Nimage; i++) {
    if (image[i].imageID == 0) continue;
    imageIndex[image[i].imageID] = i;
  }

  return;
}

int *getImageIndex (int *maxIDout) {
  *maxIDout = maxID;
  return imageIndex;
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

// sort two times vectors and an index by first time vector
void sort_image_externID (unsigned int *E, short *C, unsigned int *I, unsigned int *Q, off_t N) {

# define SWAPFUNC(A,B){ unsigned int tmp_u; short tmp_c;	\
  tmp_u = I[A]; I[A] = I[B]; I[B] = tmp_u; \
  tmp_u = E[A]; E[A] = E[B]; E[B] = tmp_u; \
  tmp_c = C[A]; C[A] = C[B]; C[B] = tmp_c; \
  tmp_u = Q[A]; Q[A] = Q[B]; Q[B] = tmp_u; \
}
# define COMPARE(A,B)(E[A] < E[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// find the image by time. this search makes the following assumptions:
// 1) there is only a single EXPOSURE from a single camera with the given exposure time
// 2) there may be multiple IMAGES from that exposure, distinguished by photcode
// 3) the image data hav been sorted by start time
// 4) once we have (time < start[i]) we are past the desired exposure
int FindIDexp (off_t *ID, off_t *Seq, e_time time, short photcode) {

  off_t Nlo, Nhi, N, Ni;

  *ID = 0;
  *Seq = -1;

  // Find the last image ending BEFORE 'time' (time > start[i]).
  // Use bisection to find the overlapping images.
  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (stopImage_exp[N] < time) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nimage);
    }
  }

  // check for the matched images starting from Nlo 
  for (N = Nlo; N < Nimage; N++) { 
    if (time > stopImage_exp[N]) continue;
    if (time < startImage_exp[N]) return FALSE;
    // a possible image match (start <= time <= stop) 
    for (Ni = N; (Ni < Nimage) && (time <= stopImage_exp[Ni]); Ni++) {
      if (photcodeImage_exp[Ni] == photcode) {
	*ID = imageID_exp[Ni];
	*Seq = imageSeq_exp[Ni];
	return TRUE;
      }
    }
    return FALSE;
  }
  return FALSE;
}

// find the image by extern ID
// 1) extID is unique among images
// 3) the image data have been sorted by extID
// 4) once we have (extID < externID[i]) we are past the desired exposure
int FindIDstk (off_t *ID, off_t *Seq, short *photcode, off_t extID) {

  off_t Nlo, Nhi, N, Ni;

  *ID = 0;
  *Seq = -1;
  *photcode = 0;

  // Find the last image with externID < extID
  // Use bisection to find the overlapping images.
  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (externID_stk[N] < extID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nimage);
    }
  }

  // check for the matched images starting from Nlo 
  for (N = Nlo; N < Nimage; N++) { 
    if (extID > externID_stk[N]) continue;
    if (extID < externID_stk[N]) return FALSE;

    // try to find other matches?
    *photcode = photcodeImage_stk[N];
    *ID = imageID_stk[N];
    *Seq = imageSeq_stk[N];
    
    for (Ni = N + 1; Ni < Nimage; Ni++) {
      if (extID < externID_stk[Ni]) break;
      if (externID_stk[Ni] == extID) {
	fprintf (stderr, "WARNING: found duplicate extern IDs : ext ID %d, image ID 1: %d (%d), image ID 2: %d (%d)\n", externID_stk[N], imageID_stk[N], photcodeImage_stk[N], imageID_stk[Ni], photcodeImage_stk[Ni]);
      }
    }
    return TRUE;
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
  int Nphu   = 0;
  int Nexp   = 0;
  int Nstk   = 0;
  int Nskip  = 0;
  int Nbad_1 = 0;
  int Nbad_2 = 0;

  for (i = 0; i < Nimage_comp; i++) {
    if (image[i].photcode == 0) {
      Nphu ++;
      continue; // skip images with 0 photcode (eg, PHU)
    }

    if ((image[i].photcode > 10000) && (image[i].photcode < 10600)) Nexp ++;
    if ((image[i].photcode >= 11000) && (image[i].photcode <= 11600)) Nstk ++;

    if (Nimage_valid[i] + Nimage_invalid[i] == 0) Nskip ++;

    if (Nimage_invalid[i] > 0) Nfixed ++;
    
    float found_ratio = (Nimage_valid[i] + Nimage_invalid[i]) / (float) image[i].nstar;
    
    if (found_ratio < 0.99) Nbad_1 ++;
    if (found_ratio < 0.95) Nbad_2 ++;
  }

  fprintf (stderr, "Nimage: "OFF_T_FMT"\n", Nimage_comp);
  fprintf (stderr, "Nfixed: %d\n", Nfixed);
  fprintf (stderr, "Nphu: %d\n", Nphu);
  fprintf (stderr, "Nexp: %d\n", Nexp);
  fprintf (stderr, "Nstk: %d\n", Nstk);
  fprintf (stderr, "Nskip: %d\n", Nskip);
  fprintf (stderr, "Ndrop (>1%%): %d\n", Nbad_1);
  fprintf (stderr, "Ndrop (>5%%): %d\n", Nbad_2);
}

void SummaryImageStats_Full (Image *image, off_t Nimage_comp) {

  off_t i;

  // let's report the following:
  // Nimages with corrected image IDs
  // Nimages with Nsum / Nstar < 0.95
  // Nimages with Nsum / Nstar < 0.99

  int Nfixed = 0;
  int Nphu   = 0;
  int Nexp   = 0;
  int Nstk   = 0;
  int Nskip  = 0;
  int Nbad_1 = 0;
  int Nbad_2 = 0;

  BuildChipMatch (image, Nimage);

  for (i = 0; i < Nimage_comp; i++) {
    if (image[i].photcode == 0) {
      Nphu ++;
      continue; // skip images with 0 photcode (eg, PHU)
    }

    if ((image[i].photcode > 10000) && (image[i].photcode < 10600)) Nexp ++;
    if ((image[i].photcode >= 11000) && (image[i].photcode <= 11600)) Nstk ++;

    float found_ratio = (Nimage_valid[i] + Nimage_invalid[i]) / (float) image[i].nstar;

    if ((Nimage_valid[i] + Nimage_invalid[i] == 0) || (found_ratio < 0.995)) {
      double r, d;
      if (!strcmp(&image[i].coords.ctype[4], "-DIS")) {
	XY_to_RD (&r, &d, 0.0, 0.0, &image[i].coords);
      } else {
	XY_to_RD (&r, &d, 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
      }
      char *date = ohana_sec_to_date (image[i].tzero);
      fprintf (stderr, "skip : %s %d  %10.6f %10.6f  %5.3f  %s\n", date, image[i].photcode, r, d, found_ratio, image[i].name);
      free (date);
      Nskip ++;
    }

    if (Nimage_invalid[i] > 0) Nfixed ++;
   
    
    if (found_ratio < 0.99) Nbad_1 ++;
    if (found_ratio < 0.95) Nbad_2 ++;
  }

  fprintf (stderr, "Nimage: "OFF_T_FMT"\n", Nimage_comp);
  fprintf (stderr, "Nfixed: %d\n", Nfixed);
  fprintf (stderr, "Nphu: %d\n", Nphu);
  fprintf (stderr, "Nexp: %d\n", Nexp);
  fprintf (stderr, "Nstk: %d\n", Nstk);
  fprintf (stderr, "Nskip: %d\n", Nskip);
  fprintf (stderr, "Ndrop (>1%%): %d\n", Nbad_1);
  fprintf (stderr, "Ndrop (>5%%): %d\n", Nbad_2);
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
