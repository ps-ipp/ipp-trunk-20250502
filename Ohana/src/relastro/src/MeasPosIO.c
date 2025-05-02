# include "relastro.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
MeasPos *MeasPosLoad(char *filename, off_t *nmeaspos) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nmeaspos = 0;
  MeasPos *measpos = NULL;

  INITTIME;

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

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return NULL;
  }

  LOGRTIME("MeasPosLoad_init %s: %f sec\n", filename, dtime);

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return (NULL);
  }
  fclose (f);
  LOGRTIME("MeasPosLoad_read %s: %f sec\n", filename, dtime);

  // a bit annoying : we read the entire block of data, then extract the columns, then set the image structure values.
  // this means I need 3 copies in memory at some point.  ugh.

  char type[16];

  GET_COLUMN (R,    	 "RA",           double);
  GET_COLUMN (D,   	 "DEC",          double);
  GET_COLUMN (objID, 	 "OBJ_ID",       int);
  GET_COLUMN (catID, 	 "CAT_ID",       int);
  GET_COLUMN (imageID, 	 "IMAGE_ID",     int);
  LOGRTIME("MeasPosLoad_getcol %s: %f sec\n", filename, dtime);

  ALLOCATE (measpos, MeasPos, Nrow);
  LOGRTIME("MeasPosLoad_alloc %s: %f sec\n", filename, dtime);

  for (i = 0; i < Nrow; i++) {
    measpos[i].R              = R    [i];
    measpos[i].D              = D    [i];
    measpos[i].objID          = objID[i];
    measpos[i].catID          = catID[i];
    measpos[i].imageID        = imageID[i];
  }
  LOGRTIME("MeasPosLoad_convert %s: %f sec\n", filename, dtime);

  fprintf (stderr, "loaded data for %lld objects (* filters)\n", (long long) Nrow);

  free (R    );
  free (D    );
  free (objID);
  free (catID);
  free (imageID);

  // free FITS table pieces...
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  LOGRTIME("MeasPosLoad_cleanup %s: %f sec\n", filename, dtime);

  *nmeaspos = Nrow;
  return measpos;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int MeasPosSave(char *filename, MeasPos *measpos, off_t Nmeaspos) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MEASPOS");

  gfits_define_bintable_column (&theader, "D", "RA",        "meas position, ra",  "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",       "meas position, dec", "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "OBJ_ID",    "object ID",          NULL,      1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "CAT_ID",    "catalog ID",         NULL,      1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "IMAGE_ID",  "image ID",           NULL,      1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  double *R, *D;
  unsigned int *objID, *catID, *imageID;

  // create intermediate storage arrays
  ALLOCATE (R,         double, 	       Nmeaspos);
  ALLOCATE (D,         double, 	       Nmeaspos);
  ALLOCATE (objID,     unsigned int,   Nmeaspos);
  ALLOCATE (catID,     unsigned int,   Nmeaspos);
  ALLOCATE (imageID,   unsigned int,   Nmeaspos);

  // assign the storage arrays
  for (i = 0; i < Nmeaspos; i++) {
    R    [i]   = measpos[i].R    ;
    D    [i]   = measpos[i].D    ;
    objID[i]   = measpos[i].objID;
    catID[i]   = measpos[i].catID;
    imageID[i] = measpos[i].imageID;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",        R,         Nmeaspos);
  gfits_set_bintable_column (&theader, &ftable, "DEC",       D,         Nmeaspos);
  gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",    objID,     Nmeaspos);
  gfits_set_bintable_column (&theader, &ftable, "CAT_ID",    catID,     Nmeaspos);
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",  imageID,   Nmeaspos);

  free (R    );
  free (D    );
  free (objID);
  free (catID);
  free (imageID);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open measmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for measpos %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for measpos %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for measpos %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for measpos %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file measpos %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file measpos %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing measpos file %s\n", filename);

  return TRUE;
}
