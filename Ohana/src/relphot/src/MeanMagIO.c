# include "relphot.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
MeanMag *MeanMagLoad(char *filename, off_t *nmeanmags) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nmeanmags = 0;
  MeanMag *meanmags = NULL;

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

  GET_COLUMN (M,    	 "MAG",          float);
  GET_COLUMN (dM,   	 "MAG_ERR",      float);
  GET_COLUMN (Mchisq, 	 "MAG_CHISQ",    float);
  GET_COLUMN (Nsec, 	 "NSEC",         int);
  GET_COLUMN (objID, 	 "OBJ_ID",       int);
  GET_COLUMN (catID, 	 "CAT_ID",       int);

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  ALLOCATE (meanmags, MeanMag, Nrow);
  for (i = 0; i < Nrow; i++) {
    meanmags[i].M              = M    	[i];
    meanmags[i].dM             = dM   	[i];
    meanmags[i].Mchisq         = Mchisq [i];
    meanmags[i].Nsec           = Nsec   [i];
    meanmags[i].objID          = objID  [i];
    meanmags[i].catID          = catID  [i];
  }
  fprintf (stderr, "loaded data for %lld objects (* filters)\n", (long long) Nrow);

  free (M     );
  free (dM    );
  free (Mchisq);
  free (Nsec  );
  free (objID );
  free (catID );

  *nmeanmags = Nrow;
  return meanmags;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int MeanMagSave(char *filename, MeanMag *meanmags, off_t Nmeanmags) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MEANMAGS");

  gfits_define_bintable_column (&theader, "E", "MAG",       "mean magnitude", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_ERR",   "mean magnitude error", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_CHISQ", "mean magnitude chisq", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "NSEC",      "secfilt sequence", NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "OBJ_ID",    "object ID",        NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "CAT_ID",    "catalog ID",       NULL, 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  float *M, *dM, *Mchisq;
  int *Nsec;
  unsigned int *objID, *catID;

  // create intermediate storage arrays
  ALLOCATE (M,         float, 	       Nmeanmags);
  ALLOCATE (dM,        float, 	       Nmeanmags);
  ALLOCATE (Mchisq,    float, 	       Nmeanmags);
  ALLOCATE (Nsec,      int,            Nmeanmags);
  ALLOCATE (objID,     unsigned int,   Nmeanmags);
  ALLOCATE (catID,     unsigned int,   Nmeanmags);

  // assign the storage arrays
  for (i = 0; i < Nmeanmags; i++) {
    M     [i]   = meanmags[i].M     ;
    dM    [i]   = meanmags[i].dM    ;
    Mchisq[i]   = meanmags[i].Mchisq;
    Nsec  [i]   = meanmags[i].Nsec  ;
    objID [i]   = meanmags[i].objID ;
    catID [i]   = meanmags[i].catID ;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "MAG",       M,         Nmeanmags);
  gfits_set_bintable_column (&theader, &ftable, "MAG_ERR",   dM,        Nmeanmags);
  gfits_set_bintable_column (&theader, &ftable, "MAG_CHISQ", Mchisq,    Nmeanmags);
  gfits_set_bintable_column (&theader, &ftable, "NSEC",      Nsec,      Nmeanmags);
  gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",    objID,     Nmeanmags);
  gfits_set_bintable_column (&theader, &ftable, "CAT_ID",    catID,     Nmeanmags);

  free (M     );
  free (dM    );
  free (Mchisq);
  free (Nsec  );
  free (objID );
  free (catID );

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for meanmags %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for meanmags %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for meanmags %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for meanmags %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanmags %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanmags %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing meanmags file %s\n", filename);

  return TRUE;
}
