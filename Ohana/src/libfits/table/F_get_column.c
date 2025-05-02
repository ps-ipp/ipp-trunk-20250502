# include <ohana.h>
# include <gfitsio.h>
# include <inttypes.h>
# define SWAP_BYTE { \
  char tmp; \
  tmp = Pin[0]; Pin[0] = Pin[1]; Pin[1] = tmp; }
# define SWAP_WORD { \
  char tmp; \
  tmp = Pin[0]; Pin[0] = Pin[3]; Pin[3] = tmp; \
  tmp = Pin[1]; Pin[1] = Pin[2]; Pin[2] = tmp; }
# define SWAP_DBLE { \
  char tmp; \
  tmp = Pin[0]; Pin[0] = Pin[7]; Pin[7] = tmp; \
  tmp = Pin[1]; Pin[1] = Pin[6]; Pin[6] = tmp; \
  tmp = Pin[2]; Pin[2] = Pin[5]; Pin[5] = tmp; \
  tmp = Pin[3]; Pin[3] = Pin[4]; Pin[4] = tmp; }

void *gfits_get_bintable_column_data (Header *header, FTable *table, char *label, char *type, off_t *Nrow, int *Ncol) {
  void *data = gfits_get_bintable_column_data_raw (header, table, label, type, Nrow, Ncol, FALSE);
  return data;
}

void *gfits_get_bintable_column_data_raw (Header *header, FTable *table, char *label, char *type, off_t *Nrow, int *Ncol, char nativeOrder) {

  off_t Nx, Ny;
  int i, N, Nfields, Nval, Nbytes, Nstart, Nv, Nb;
  char tlabel[256], field[256], format[256], tmpline[64];
  char *Pin, *Pout, *array;
  double Bscale, Bzero;

  if (label == (char *) NULL) return (NULL);
  if (label[0] == 0) return (NULL);

  /* find label in header */
  tlabel[0] = 0;
  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) return (NULL);
  for (i = 1; strcasecmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, tlabel);
  }
  if (strcasecmp (label, tlabel)) return (NULL);
  N = i - 1;

  Bscale = 1; 
  Bzero  = 0;

  /* interpret format */
  snprintf (field, 256, "TSCAL%d", N);
  gfits_scan (header, field, "%lf", 1, &Bscale);
  snprintf (field, 256, "TZERO%d", N);
  gfits_scan (header, field, "%lf", 1, &Bzero);
  snprintf (field, 256, "TFORM%d", N);
  gfits_scan (header, field, "%s", 1, format);

  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return (NULL);
  
  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  /* scan columns to find insert point */
  Nstart = 0;
  for (i = 1; i < N; i++) {
    snprintf (field, 256, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  /* extract bytes from table into array */
  ALLOCATE (array, char, Nbytes*Nval*Ny);
  Pin  = table[0].buffer + Nstart;
  Pout = array;
  for (i = 0; i < Ny; i++, Pin += Nx, Pout += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

  // NOTE: we have already copied the data to 'array', so the blocks below
  // only need to swap if this is a direct copy
  Pin  = array;
  Pout = array;
  int directCopy = (Bzero == 0.0) && (Bscale == 1.0);

  /* convert data in-situ with correct type, byte swap and Bzero/Bscale */
  if (!strcmp (type, "char")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
      if (!directCopy) { *(char *)Pout = *(char *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "gfbyte")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
      if (!directCopy) { *(char *)Pout = *(char *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "short")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_BYTE; }
# endif
      if (!directCopy) { *(short *)Pout = *(short *)Pin*Bscale + Bzero; }
    }  
  }
  if (!strcmp (type, "int")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_WORD; }
# endif
      if (!directCopy) { *(int *)Pout = *(int *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "int64_t")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_DBLE; }
# endif
      if (!directCopy) { *(int64_t *)Pout = *(int64_t *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "float")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_WORD; }
# endif
      if (!directCopy) { *(float *)Pout = *(float *)Pin*Bscale + Bzero; }
    }
  }
  if (!strcmp (type, "double")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      if (!nativeOrder) { SWAP_DBLE; }
# endif
      if (!directCopy) { *(double *)Pout = *(double *)Pin*Bscale + Bzero; }
    }
  }

  // check that we supplied a valid type

  *Ncol = Nval;
  *Nrow = Ny;
  return (array);
}

// do a column byteswap in-situ (needed because uncompress returns an unswapped binary table
int gfits_byteswap_bintable_column (FTable *ftable, int column) {

  off_t Nx, Ny;
  int i, Nval, Nbytes, Nv, Nb;
  char field[256], format[256], tmpline[64], type[64];
  char *Pin, *Pout, *array;

  Header *header = ftable->header;

  /* interpret format */
  snprintf (field, 256, "TFORM%d", column);
  if (!gfits_scan (header, field, "%s", 1, format)) return FALSE;

  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return FALSE;
  
  /* convert data in-situ with correct type, byte swap and Bzero/Bscale */
  if (!strcmp (type, "char")) return TRUE;
  if (!strcmp (type, "gfbyte")) return TRUE;

  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  /* scan prior columns to find insert point */
  int Nstart = 0;
  for (i = 1; i < column; i++) {
    snprintf (field, 256, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  /* extract bytes from table into array */
  ALLOCATE (array, char, Nbytes*Nval*Ny);
  Pin  = ftable[0].buffer + Nstart;
  Pout = array;
  for (i = 0; i < Ny; i++, Pin += Nx, Pout += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

  // NOTE: we have already copied the data to 'array', so the blocks below
  // only need to swap if this is a direct copy
  Pin  = array;
  Pout = array;

  if (!strcmp (type, "short")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      SWAP_BYTE;
# endif
    }  
  }
  if (!strcmp (type, "int")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      SWAP_WORD;
# endif
    }
  }
  if (!strcmp (type, "int64_t")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      SWAP_DBLE;
# endif
    }
  }
  if (!strcmp (type, "float")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      SWAP_WORD;
# endif
    }
  }
  if (!strcmp (type, "double")) {
    for (i = 0; i < Nval*Ny; i++, Pin+=Nbytes, Pout+=Nbytes) {
# ifdef BYTE_SWAP
      SWAP_DBLE;
# endif
    }
  }

  // save the bytes back into the table buffer
  Pout = ftable[0].buffer + Nstart;
  Pin  = array;
  for (i = 0; i < Ny; i++, Pout += Nx, Pin += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

  return TRUE;
}

/***********************/
int gfits_get_bintable_column_type_by_N (Header *header, int N, char *type, int *Nval) {

  int Nbytes;
  char field[256], format[256];

  assert (N > 0);

  snprintf (field, 256, "TFORM%d", N);
  if (!gfits_scan (header, field, "%s", 1, format)) return FALSE;
  if (!gfits_bintable_format (format, type, Nval, &Nbytes)) return (FALSE);
  return (TRUE);
}

/***********************/
int gfits_get_bintable_column_type (Header *header, char *label, char *type, int *Nval) {

  int i, N, Nfields;
  char tlabel[256], field[256];

  if (label == (char *) NULL) return (FALSE);
  if (label[0] == 0) return (FALSE);

  /* find label in header */
  tlabel[0] = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  for (i = 1; strcasecmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, tlabel);
  }
  if (strcasecmp (label, tlabel)) return (FALSE);
  N = i - 1;

  if (!gfits_get_bintable_column_type_by_N (header, N, type, Nval)) return FALSE;
  return (TRUE);
}

/***********************/
int gfits_get_bintable_column_raw (Header *header, FTable *table, char *label, void **data, char nativeOrder) {

  char type[64];
  off_t Nrow;
  int Ncol;

  char *array = gfits_get_bintable_column_data_raw (header, table, label, type, &Nrow, &Ncol, nativeOrder);
  if (array == NULL) return (FALSE);

  *data = array;
  return TRUE;
}

/***********************/
int gfits_get_bintable_column (Header *header, FTable *table, char *label, void **data) {

  char type[64];
  off_t Nrow;
  int Ncol;

  char *array = gfits_get_bintable_column_data (header, table, label, type, &Nrow, &Ncol);
  if (array == NULL) return (FALSE);

  *data = array;
  return TRUE;
}

/* 
   valid BINTABLE column formats:
   L - logical
   X - bit
   I - 16 bit int
   J - 32 bit int
   A - char
   E - float 
   D - double
   B - unsigned bytes
   C - complex float
   M - complex double
   P - var length array descrpt (64 bit)
   
   all can be preceeded by integer N which specified number
   of entries in array
   
*/

/***********************/
int gfits_get_table_column_type (Header *header, char *label, char *type, int *Nval) {

  int i, N, Nfields, Nbytes;
  char tlabel[256], field[256], format[256];

  if (label == (char *) NULL) return (FALSE);
  if (label[0] == 0) return (FALSE);

  /* find label in header */
  tlabel[0] = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  for (i = 1; strcasecmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, tlabel);
  }
  if (strcasecmp (label, tlabel)) return (FALSE);
  N = i - 1;

  /* interpret format */
  snprintf (field, 256, "TFORM%d", N);
  gfits_scan (header, field, "%s", 1, format);

  if (!gfits_table_format (format, type, Nval, &Nbytes)) return (FALSE);

  return (TRUE);
}

/***********************/
int gfits_get_table_column (Header *header, FTable *table, char *label, void **data) {

  off_t Nx, Ny;
  int i, N, Nfields, Nval, Nbytes, Nstart, Nv, Nb;
  char tlabel[256], field[256], format[256], cformat[256], type[64], tmp[64];
  char *array, *Pin, *Pout, *line;

  if (label == (char *) NULL) return (FALSE);
  if (label[0] == 0) return (FALSE);
  array = NULL;

  /* find label in header */
  tlabel[0] = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  for (i = 1; strcasecmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, tlabel);
  }
  if (strcasecmp (label, tlabel)) return (FALSE);
  N = i - 1;

  /* interpret format */
  snprintf (field, 256, "TFORM%d", N);
  gfits_scan (header, field, "%s", 1, format);

  if (!gfits_table_format (format, type, &Nval, &Nbytes)) return (FALSE);
  strcpy (cformat, format);
  
  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1",  OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2",  OFF_T_FMT, 1,  &Ny);

  // find the starting byte for this column
  // FITS ASCII table is supposed to have TBCOLn to specify the starting point
  // but if it is missing, we can try to find by counting
  snprintf (field, 256, "TBCOL%d", N);
  int status = gfits_scan (header, field, "%d", 1, &Nstart);
  if (!status) {
    /* scan columns to find insert point */
    Nstart = 0;
    for (i = 1; i < N; i++) {
      snprintf (field, 256, "TFORM%d", i);
      gfits_scan (header, field, "%s", 1, format);
      gfits_table_format (format, tmp, &Nv, &Nb);
      Nstart += Nv*Nb;
    }
  } else {
    Nstart --;
  }

  /* allocate temporary line, init pointers */
  ALLOCATE (line, char, Nval*Nbytes+1);
  bzero (line, Nval*Nbytes+1);
  Pin  = table[0].buffer + Nstart;

  /* allocate output array, copy/scan line as needed */
  if (!strcmp (type, "char")) {
    ALLOCATE (array, char, Ny*Nval);
    Pout = array;
    for (i = 0; i < Ny; i++, Pin+=Nx, Pout+=Nval*Nbytes) {
      memcpy (Pout, Pin, Nval*Nbytes);
    }
  }
  if (!strcmp (type, "int")) {
    ALLOCATE (array, char, Ny*4);
    int *tmpPtr = (int *)array;
    for (i = 0; i < Ny; i++, Pin+=Nx, tmpPtr ++) {
      memcpy (line, Pin, Nval*Nbytes);
      sscanf (line, "%d", tmpPtr);
      // fprintf (stderr, "test: %d %d\n", Nscan, tmpValue);
    }
  }
  if (!strcmp (type, "int64_t")) {
    ALLOCATE (array, char, Ny*8);
    int64_t *tmpPtr = (int64_t *)array;
    for (i = 0; i < Ny; i++, Pin+=Nx, tmpPtr ++) {
      memcpy (line, Pin, Nval*Nbytes);
      sscanf (line, "%" PRId64, tmpPtr);
    }
  }
  if (!strcmp (type, "float")) {
    ALLOCATE (array, char, Ny*4);
    float *tmpPtr = (float *) array;
    for (i = 0; i < Ny; i++, Pin+=Nx, tmpPtr ++) {
      memcpy (line, Pin, Nval*Nbytes);
      sscanf (line, "%e", tmpPtr);
    }
  }
  if (!strcmp (type, "double")) {
    ALLOCATE (array, char, Ny*8);
    double *tmpPtr = (double *) array;
    for (i = 0; i < Ny; i++, Pin+=Nx, tmpPtr++) {
      memcpy (line, Pin, Nval*Nbytes);
      sscanf (line, "%le", tmpPtr);
    }
  }
  *data = array;
  free (line);

  return (TRUE);
}

/*
  valid TABLE column formats:
  FN.N  - floating point
  INN   - integer
  ANN   - NN char string
  
*/

