# include "dvorepair.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

// write out the IDmap data for clients to read
int DeleteImagesSave (char *filename, Image *image, off_t Nimage, int *deleteImage) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "DELETE_IMAGES");

  gfits_define_bintable_column (&theader, "J", "IMAGE_ID", "image IDs",    NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "I", "DELETE",   "delete image", NULL, 1.0, FT_BZERO_INT16);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  int *imageID;
  short *delImage;

  // copy the deleteImage data so gfits_set_bintable_column does not affect it
  ALLOCATE (imageID,  int,   Nimage);
  ALLOCATE (delImage, short, Nimage);

  int i;
  for (i = 0; i < Nimage; i++) {
    imageID[i] = image[i].imageID;
    delImage[i] = deleteImage[i];
  }

  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID", imageID,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "DELETE",   delImage, Nimage);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open ID map file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image subset %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image subset %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image subset %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image subset %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image subset %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file image subset %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image subset file %s\n", filename);

  free (delImage);
  free (imageID);

  return TRUE;
}

int DeleteImagesLoad (char *filename, DeleteImageDataType *deleteImageData) {

  int Ncol;
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
    fclose (f);
    return FALSE;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return FALSE;
  }
  fclose (f);

  char type[16];

  int *imageID;
  short *delImage;

  GET_COLUMN (imageID,  "IMAGE_ID", int);
  GET_COLUMN (delImage, "DELETE",   short);
  deleteImageData->Nimage = Nrow;
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  // generate an index for imageIDs
  deleteImageData->imageIDindex = myIndexInit();
  ALLOCATE (deleteImageData->deleteImage, int, deleteImageData->Nimage);

  // set the min and max ID values for the two strucures:
  int i;
  for (i = 0; i < deleteImageData->Nimage; i++) {
    myIndexUpdateLimits (deleteImageData->imageIDindex,  imageID[i]);
    deleteImageData->deleteImage[i] = delImage[i];
  }
  myIndexSetRange (deleteImageData->imageIDindex);

  // assign the externIDs and imageIDs to their index structures
  for (i = 0; i < deleteImageData->Nimage; i++) {
    myIndexSetEntry (deleteImageData->imageIDindex,  imageID[i],  i);
  }

  gfits_free_header(&header);
  gfits_free_matrix(&matrix);

  gfits_free_header(&theader);
  gfits_free_table (&ftable);

  return TRUE;
}

