# include "relastro.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
MeanPos *MeanPosLoad(char *filename, off_t *nmeanpos) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nmeanpos = 0;
  MeanPos *meanpos = NULL;

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

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return (NULL);
  }
  fclose (f);

  // a bit annoying : we read the entire block of data, then extract the columns, then set the image structure values.
  // this means I need 3 copies in memory at some point.  ugh.

  char type[16];

  GET_COLUMN (R,    	 "RA",           double);
  GET_COLUMN (D,   	 "DEC",          double);
  GET_COLUMN (objID, 	 "OBJ_ID",       int);
  GET_COLUMN (catID, 	 "CAT_ID",       int);

  ALLOCATE (meanpos, MeanPos, Nrow);
  for (i = 0; i < Nrow; i++) {
    meanpos[i].R              = R    [i];
    meanpos[i].D              = D    [i];
    meanpos[i].objID          = objID[i];
    meanpos[i].catID          = catID[i];
  }
  fprintf (stderr, "loaded data for %lld objects (* filters)\n", (long long) Nrow);

  free (R    );
  free (D    );
  free (objID);
  free (catID);

  // free FITS table pieces...
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  *nmeanpos = Nrow;
  return meanpos;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int MeanPosSave(char *filename, MeanPos *meanpos, off_t Nmeanpos) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MEANPOS");

  gfits_define_bintable_column (&theader, "D", "RA",        "mean position, ra",  "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",       "mean position, dec", "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "OBJ_ID",    "object ID",          NULL,      1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "CAT_ID",    "catalog ID",         NULL,      1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  double *R, *D;
  unsigned int *objID, *catID;

  // create intermediate storage arrays
  ALLOCATE (R,         double, 	       Nmeanpos);
  ALLOCATE (D,         double, 	       Nmeanpos);
  ALLOCATE (objID,     unsigned int,   Nmeanpos);
  ALLOCATE (catID,     unsigned int,   Nmeanpos);

  // assign the storage arrays
  for (i = 0; i < Nmeanpos; i++) {
    R    [i]   = meanpos[i].R    ;
    D    [i]   = meanpos[i].D    ;
    objID[i]   = meanpos[i].objID;
    catID[i]   = meanpos[i].catID;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",        R,         Nmeanpos);
  gfits_set_bintable_column (&theader, &ftable, "DEC",       D,         Nmeanpos);
  gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",    objID,     Nmeanpos);
  gfits_set_bintable_column (&theader, &ftable, "CAT_ID",    catID,     Nmeanpos);

  free (R    );
  free (D    );
  free (objID);
  free (catID);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for meanpos %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for meanpos %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for meanpos %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for meanpos %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanpos %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanpos %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing meanpos file %s\n", filename);

  return TRUE;
}
