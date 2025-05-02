# include "dvomerge.h"

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
int IDmapSave(char *filename, IDmapType *IDmap) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_ID_MAP");

  gfits_define_bintable_column (&theader, "J", "OLD_IDS", "old image IDs", NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "NEW_IDS", "new image IDs", NULL, 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  // XXX does this damage the data (due to byte swaps?) 
  // XXX does it matter?
  gfits_set_bintable_column (&theader, &ftable, "OLD_IDS", IDmap->old, IDmap->Nmap);
  gfits_set_bintable_column (&theader, &ftable, "NEW_IDS", IDmap->new, IDmap->Nmap);

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

  return TRUE;
}

IDmapType *IDmapLoad (char *filename) {

  int Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;
  IDmapType *IDmap;

  ALLOCATE (IDmap, IDmapType, 1);
  IDmap->Nmap = 0;
  IDmap->old = NULL;
  IDmap->new = NULL;

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

  char type[16];

  GET_COLUMN (IDmap->old, "OLD_IDS", int);
  GET_COLUMN (IDmap->new, "NEW_IDS", int);
  IDmap->Nmap = Nrow;
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  create_IDmap_lookup (IDmap);

  gfits_free_header(&header);
  gfits_free_matrix(&matrix);

  gfits_free_header(&theader);
  gfits_free_table (&ftable);

  return IDmap;
}

