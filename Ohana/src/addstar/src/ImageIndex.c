# include "addstar.h"

void ImageIndexFree (ImageIndex *index) {
  if (!index) return;
  FREE (index->externID);
  FREE (index->imageID);
  FREE (index->found);
  FREE (index);
}

ImageIndex *ImageIndexLoad (char *filename) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  ImageIndex *index = NULL;
  ALLOCATE (index, ImageIndex, 1);

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image index file %s\n", filename);
    FREE (index);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image index header\n");
    fclose (f);
    FREE (index);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image index matrix\n");
    gfits_free_header (&header);
    FREE (index);
    fclose (f);
    return NULL;
  }
  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    gfits_free_header (&header);
    FREE (index);
    return NULL;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    gfits_free_header (&header);
    gfits_free_header (&theader);
    FREE (index);
    return (NULL);
  }
  fclose (f);

  char type[16];
  index->externID = gfits_get_bintable_column_data (&theader, &ftable, "EXTERN_ID", type, &Nrow, &Ncol);
  myAssert (!strcmp(type, "int"), "wrong column type");

  index->imageID  = gfits_get_bintable_column_data (&theader, &ftable, "IMAGE_ID", type, &Nrow, &Ncol);
  myAssert (!strcmp(type, "int"), "wrong column type");

  index->Nimage = Nrow;

  // find the range of externID values
  index->minID = -1;
  index->maxID = -1;
  for (i = 0; i < index->Nimage; i++) {
    if (index->minID == -1) index->minID = index->externID[i];
    index->minID = MIN(index->minID, index->externID[i]);
    index->maxID = MAX(index->maxID, index->externID[i]);
  }
  index->range = index->maxID- index->minID + 1;
  
  // init the index
  ALLOCATE (index->found, char, index->range);
  for (i = 0; i < index->range; i++) index->found[i] = FALSE;

  // generate the index (set this value to the imageID?)
  for (i = 0; i < index->Nimage; i++) {
    IDTYPE N = index->externID[i] - index->minID;
    index->found[N] = TRUE;
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  return index;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int ImageIndexSave (char *filename, ImageIndex *index) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "EXTERN_ID");
  gfits_define_bintable_column (&theader, "J", "IMAGE_ID", "image ID", NULL, 1.0, 0);
  gfits_define_bintable_column (&theader, "J", "EXTERN_ID", "extern ID", NULL, 1.0, 0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",       index->imageID,  index->Nimage);
  gfits_set_bintable_column (&theader, &ftable, "EXTERN_ID",      index->externID, index->Nimage);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open index file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image index %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image index %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image index %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image index %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image index %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot fsync file image index %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image index file %s\n", filename);

  return TRUE;
}

int CheckDuplicateImageIDs (Image *images, off_t Nimages) {

  IDTYPE i, offset;

  // load the image ID table
  char filename[DVO_MAX_PATH];
  snprintf (filename, DVO_MAX_PATH, "%s/ImageIndex.fits", CATDIR);

  ImageIndex *index = ImageIndexLoad (filename);

  if (!index) {
      fprintf (stderr, "image index file is not found, cannot check for image duplicates\n");
      exit (1);
  }

  for (i = 0; i < Nimages; i++) {
    IDTYPE ID = images[i].externID;
    if (ID == 0) continue;

    if (ID < index->minID) continue;
    if (ID > index->maxID) continue;

    offset = ID - index->minID;
    myAssert (offset >= 0, "offset out of range?");
    myAssert (offset <= index->range, "offset out of range?");

    if (!index->found[offset]) continue;
    fprintf (stderr, "duplicate external image ID %lld found, exiting\n", (long long) ID);
    exit (1);
  }
  
  // extend the storage arrays
  // no reason to extend 'found'
  REALLOCATE (index->imageID,  IDTYPE, index->Nimage + Nimages);
  REALLOCATE (index->externID, IDTYPE, index->Nimage + Nimages);

  // append the new ext IDs and save
  for (i = 0; i < Nimages; i++) {
    if (images[i].externID == 0) continue;
    index->imageID[index->Nimage] = images[i].imageID;
    index->externID[index->Nimage] = images[i].externID;
    index->Nimage ++;
  }

  // do this as an 'extend' operation
  ImageIndexSave (filename, index);
  ImageIndexFree (index);
  return TRUE;
}

int ImageIndexFileInit () {

  // load the image ID table
  char filename[DVO_MAX_PATH];
  snprintf (filename, DVO_MAX_PATH, "%s/ImageIndex.fits", CATDIR);

  ImageIndex *index = NULL;
  ALLOCATE (index, ImageIndex, 1);
  ALLOCATE (index->imageID,  IDTYPE, 1);
  ALLOCATE (index->externID, IDTYPE, 1);
  ALLOCATE (index->found, char, 1);
  index->Nimage = 0;

  // do this as an 'extend' operation
  ImageIndexSave (filename, index);
  ImageIndexFree (index);
  return TRUE;
}
