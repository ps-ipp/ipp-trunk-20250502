# include "delstar.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

MeasureEdge *MeasureEdgeLoad(char *filename, off_t *Nmeasure_edge) {

  int Ncol;
  off_t i;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *Nmeasure_edge= 0;
  MeasureEdge *measure_edge = NULL;

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

  GET_COLUMN (R      ,     "RA",        double);
  GET_COLUMN (D      ,     "DEC",       double);
  GET_COLUMN (objID  ,     "OBJ_ID",    int);
  GET_COLUMN (catID  ,     "CAT_ID",    int);
  GET_COLUMN (detID  ,     "DET_ID",    int);
  GET_COLUMN (imageID,     "IMAGE_ID",  int);

  ALLOCATE (measure_edge, MeasureEdge, Nrow);

  // merge the new arrays into the single array?
  for (i = 0; i < Nrow; i++) {
    measure_edge[i].R        = R[i]     ;
    measure_edge[i].D        = D[i]    ;
    measure_edge[i].objID    = objID[i]  ;
    measure_edge[i].catID    = catID[i]  ;
    measure_edge[i].detID    = detID[i]  ;
    measure_edge[i].imageID  = imageID[i];
  }
  fprintf (stderr, "loaded data for %lld measures\n", (long long) Nrow);

  free (R       );
  free (D       );
  free (objID   );
  free (catID   );
  free (detID   );
  free (imageID );

  *Nmeasure_edge = Nrow;
  return measure_edge;
}

int MeasureEdgeSave(char *filename, MeasureEdge *measure_edge, off_t Nmeasure_edge) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MEASURE_EDGE");

  // create the table layout
  gfits_define_bintable_column (&theader, "D",   "RA",       "tmp",        "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D",   "DEC",      "tmp",        "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "OBJ_ID",   "tmp",        "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "CAT_ID",   "tmp",        "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "DET_ID",   "tmp",        "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "IMAGE_ID", "tmp",        "tmp", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  double *R, *D;
  int *imageID, *objID, *catID, *detID; 

  ALLOCATE (R      , double, Nmeasure_edge);
  ALLOCATE (D      , double, Nmeasure_edge);
  ALLOCATE (objID  , int,    Nmeasure_edge);
  ALLOCATE (catID  , int,    Nmeasure_edge);
  ALLOCATE (detID  , int,    Nmeasure_edge);
  ALLOCATE (imageID, int,    Nmeasure_edge);

  // assign the storage arrays
  off_t i;
  for (i = 0; i < Nmeasure_edge; i++) {
    R[i]        = measure_edge[i].R;
    D[i]        = measure_edge[i].D;
    objID[i]    = measure_edge[i].objID;
    catID[i]    = measure_edge[i].catID;
    detID[i]    = measure_edge[i].detID;
    imageID[i]  = measure_edge[i].imageID;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",       R,       Nmeasure_edge);
  gfits_set_bintable_column (&theader, &ftable, "DEC",      D,       Nmeasure_edge);
  gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",   objID,   Nmeasure_edge);
  gfits_set_bintable_column (&theader, &ftable, "CAT_ID",   catID,   Nmeasure_edge);
  gfits_set_bintable_column (&theader, &ftable, "DET_ID",   detID,   Nmeasure_edge);
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID", imageID, Nmeasure_edge);

  free (R       );
  free (D       );
  free (objID   );
  free (catID   );
  free (detID   );
  free (imageID );

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

MeasureEdge *MeasureEdgeMerge (MeasureEdge *measure_edge_all, off_t *Nmeasure_edge_in, off_t *NMEASURE_EDGE_IN, MeasureEdge *measure_edge, off_t Nmeasure_edge) {

  off_t Nmeasure_edge_all = *Nmeasure_edge_in;
  off_t NMEASURE_EDGE_ALL = *NMEASURE_EDGE_IN;

  off_t Nstart = Nmeasure_edge_all;

  Nmeasure_edge_all += Nmeasure_edge;
  NMEASURE_EDGE_ALL += Nmeasure_edge;
  REALLOCATE (measure_edge_all, MeasureEdge, NMEASURE_EDGE_ALL);
  
  off_t i;
  for (i = 0; i < Nmeasure_edge; i++) {
    measure_edge_all[i + Nstart] = measure_edge[i];
  }

  *Nmeasure_edge_in = Nmeasure_edge_all;
  *NMEASURE_EDGE_IN = NMEASURE_EDGE_ALL;
  return measure_edge_all;
}

