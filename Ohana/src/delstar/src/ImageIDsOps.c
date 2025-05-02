# include "delstar.h"
# define BZERO_INT16 1.0*0x8000
# define BZERO_INT32 1.0*0x80000000
# define BZERO_INT64 1.0*0x8000000000000000

IndexArray *ImageIDLoad(char *filename) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return NULL;
  }

  IndexArray *imageID = NULL;
  ALLOCATE (imageID, IndexArray, 1);

  gfits_scan (&header, "MIN_ID", OFF_T_FMT, 1, &imageID->minID);
  gfits_scan (&header, "MAX_ID", OFF_T_FMT, 1, &imageID->maxID);
  gfits_scan (&header, "RANGE",  OFF_T_FMT, 1, &imageID->range);

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return NULL;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return NULL;
  }
  fclose (f);

  char type[16];
  unsigned short *value = gfits_get_bintable_column_data (&theader, &ftable, "VALUE", type, &Nrow, &Ncol);
  myAssert (!strcmp(type, "short"), "wrong column type");
  myAssert (Nrow == imageID->range, "wrong number of rows in table?");

  // XXX free the fits table data here 

  ALLOCATE (imageID->value, off_t, imageID->range);
  for (i = 0; i < imageID->range; i++) {
    imageID->value[i] = value[i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (value);

  return imageID;
}

int ImageIDSave(char *filename, IndexArray *imageID) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_modify (&header, "MIN_ID", OFF_T_FMT, 1, imageID->minID);
  gfits_modify (&header, "MAX_ID", OFF_T_FMT, 1, imageID->maxID);
  gfits_modify (&header, "RANGE",  OFF_T_FMT, 1, imageID->range);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_ID");

  gfits_define_bintable_column (&theader, "I", "VALUE", "value", NULL, 1.0, BZERO_INT16);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  unsigned short *value;

  // create intermediate storage arrays
  ALLOCATE (value, unsigned short, imageID->range);

  // assign the storage arrays
  for (i = 0; i < imageID->range; i++) {
    value[i]   = imageID->value[i];
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "VALUE", value, imageID->range);

  free (value);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image ID file for output %s\n", filename);
    return FALSE;
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);
  fflush (f);

  return TRUE;
}
