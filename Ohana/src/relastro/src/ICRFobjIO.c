# include "relastro.h"

// column OUT refers to a variable which is already defined
# define GET_COLUMN(OUT,NAME,TYPE) \
  OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
ICRFobj *ICRFobjLoad(char *filename) {

  int Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  ICRFobj *icrfobj = NULL;

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

  char type[16]; // used in GET_COLUMN

  ALLOCATE (icrfobj, ICRFobj, 1);

  GET_COLUMN (icrfobj->Rave,   	 "R_AVE",        double);
  GET_COLUMN (icrfobj->Dave,   	 "D_AVE",        double);
  GET_COLUMN (icrfobj->dRoff,  	 "R_OFF",        double);
  GET_COLUMN (icrfobj->dDoff,  	 "D_OFF",        double);
  fprintf (stderr, "loaded data for %lld objects (* filters)\n", (long long) Nrow);

  // free FITS table pieces...
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  icrfobj->Nicrfobj = Nrow;
  return icrfobj;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int ICRFobjSave(char *filename, ICRFobj *icrfobj) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "ICRFOBJ");

  gfits_define_bintable_column (&theader, "D", "R_AVE",      "mean position, ra",  "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "D_AVE",      "mean position, dec", "degrees", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "R_OFF",      "position offset, ra",  "arcsec", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "D_OFF",      "position offset, dec", "arcsec", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "R_AVE",       icrfobj->Rave ,         icrfobj->Nicrfobj);
  gfits_set_bintable_column (&theader, &ftable, "D_AVE",       icrfobj->Dave ,         icrfobj->Nicrfobj);
  gfits_set_bintable_column (&theader, &ftable, "R_OFF",       icrfobj->dRoff,         icrfobj->Nicrfobj);
  gfits_set_bintable_column (&theader, &ftable, "D_OFF",       icrfobj->dDoff,         icrfobj->Nicrfobj);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for icrfobj %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for icrfobj %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for icrfobj %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for icrfobj %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file icrfobj %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot fsync file icrfobj %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing icrfobj file %s\n", filename);

  return TRUE;
}
