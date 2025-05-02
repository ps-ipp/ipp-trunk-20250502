# include <ohana.h>
# include <gfitsio.h>
# define OHANA_MEMCHECK 0
# define CHECK_MEMBLOCKS 0

# define SWAP_NONE 

# ifdef BYTE_SWAP 
# define SWAP_BYTE { \
  char tmp; \
  tmp = Pout[0]; Pout[0] = Pout[1]; Pout[1] = tmp; }
# define SWAP_WORD { \
  char tmp; \
  tmp = Pout[0]; Pout[0] = Pout[3]; Pout[3] = tmp; \
  tmp = Pout[1]; Pout[1] = Pout[2]; Pout[2] = tmp; }
# define SWAP_DBLE { \
  char tmp; \
  tmp = Pout[0]; Pout[0] = Pout[7]; Pout[7] = tmp; \
  tmp = Pout[1]; Pout[1] = Pout[6]; Pout[6] = tmp; \
  tmp = Pout[2]; Pout[2] = Pout[5]; Pout[5] = tmp; \
  tmp = Pout[3]; Pout[3] = Pout[4]; Pout[4] = tmp; }
# else
# define SWAP_BYTE 
# define SWAP_WORD 
# define SWAP_DBLE 
# endif

/***********************/
int gfits_set_bintable_column (Header *header, FTable *table, char *label, void *data, off_t Nrow) {

  off_t Nx, Ny;
  int i, N, Nfields;
  int Nval, Nbytes, Nstart, Nv, Nb;
  char tlabel[256], field[256], format[256], type[64], tmpline[64];
  char *Pin, *Pout, *array;
  double Bscale, Bzero;

# if (OHANA_MEMCHECK)
  memset (tlabel,  0x7f, 256);
  memset (field,   0x7f, 256);
  memset (format,  0x7f, 256);
  memset (type,    0x7f, 64);
  memset (tmpline, 0x7f, 64);

  ohana_memcheck_block (data);
# else

  memset (tlabel,  0, 256);
  memset (field,   0, 256);
  memset (format,  0, 256);
  memset (type,    0, 64);
  memset (tmpline, 0, 64);
# endif

  if (label == (char *) NULL) return (FALSE);
  if (label[0] == 0) return (FALSE);

  /* find label in header */
  tlabel[0] = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  for (i = 1; strcasecmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    if (!gfits_scan (header, field, "%s", 1, tlabel)) return FALSE;
  }
  if (strcasecmp (label, tlabel)) return (FALSE);
  N = i - 1;

  /* interpret format */
  snprintf (field, 256, "TSCAL%d", N);
  if (!gfits_scan (header, field, "%lf", 1, &Bscale)) {
    Bscale = 1.0;
  }
  snprintf (field, 256, "TZERO%d", N);
  if (!gfits_scan (header, field, "%lf", 1, &Bzero)) {
    Bzero = 0.0;
  }
  snprintf (field, 256, "TFORM%d", N);
  if (!gfits_scan (header, field, "%s", 1, format)) return FALSE;

  if (!gfits_bintable_format (format, type, &Nval, &Nbytes)) return (FALSE);
  
  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);

  // if no rows have yet been assigned, we need to allocate the full data buffer
  if (Ny == 0) { 
    Ny = Nrow;
    gfits_set_table_rows (header, table, Ny);
  }
  if (Ny != Nrow) return (FALSE);

  // if we call this function with a null pointer, we are only validating the header and allocating the data array
  if (!data) return TRUE;

  /* scan columns to find insert point */
  Nstart = 0;
  for (i = 1; i < N; i++) {
    snprintf (field, 256, "TFORM%d", i);
    if (!gfits_scan (header, field, "%s", 1, format)) return FALSE;
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  // ohana_memcheck (TRUE);

  /* make duplicate of data with correct type
     byte swap and Bzero/Bscale */
  ALLOCATE (array, char, Nbytes*Nval*Nrow);
  if (CHECK_MEMBLOCKS) {
    OhanaMemblock *ref = (OhanaMemblock *) array - 1;
    fprintf (stderr, "ref: 0x%08lx, array: 0x%08lx, size: %zd, (%s@%d : %s), freed: %1d, Nalloc: %d\n", (long int) ref, (long int) array, ref->size, ref->file, ref->line, ref->func, ref->freed, ref->Nalloc);
    if (!ref->nextBlock && !ref->prevBlock) abort();
  }

  Pin = data;
  Pout = array;

  // ohana_memcheck (TRUE);

  if (CHECK_MEMBLOCKS) {
    OhanaMemblock *ref = (OhanaMemblock *) array - 1;
    fprintf (stderr, "ref: 0x%08lx, array: 0x%08lx, size: %zd, (%s@%d : %s), freed: %1d, Nalloc: %d\n", (long int) ref, (long int) array, ref->size, ref->file, ref->line, ref->func, ref->freed, ref->Nalloc);
    if (!ref->nextBlock && !ref->prevBlock) abort();
  }

  // does it makes sense to scale 'char' data?
  if (!strcmp (type, "char")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(char *)Pout = (*(char *)Pin - Bzero) / Bscale;
    }
  }

  if (!strcmp (type, "gfbyte")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(char *)Pout = (*(char *)Pin - Bzero) / Bscale;
    }
  }

  if (!strcmp (type, "short")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(short *)Pout = (*(short *)Pin - Bzero) / Bscale;
# ifdef BYTE_SWAP
      SWAP_BYTE;
# endif
    }  
  }
  
  if (!strcmp (type, "int")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(int *)Pout = (*(int *)Pin - Bzero) / Bscale;
# ifdef BYTE_SWAP
      SWAP_WORD;
# endif
    }
  }

  if (!strcmp (type, "int64_t")) {
    // XXX 64 bit int operations with Bzero & Bscale are inaccurate even with doubles
    if ((Bzero == 0.0) && (Bscale == 1.0)) {
      for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
	*(int64_t *)Pout = *(int64_t *)Pin;
# ifdef BYTE_SWAP
	SWAP_DBLE;
# endif
      }
    } else {
      for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
	*(int64_t *)Pout = (*(int64_t *)Pin - Bzero) / Bscale;
# ifdef BYTE_SWAP
	SWAP_DBLE;
# endif
      }
    }
  }

  if (!strcmp (type, "float")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(float *)Pout = (*(float *)Pin - Bzero) / Bscale;
# ifdef BYTE_SWAP
      SWAP_WORD;
# endif
    }
  }

  if (!strcmp (type, "double")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(double *)Pout = (*(double *)Pin - Bzero) / Bscale;
# ifdef BYTE_SWAP
      SWAP_DBLE;
# endif
    }
  }

  if (!strcmp (type, "var")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(int *)Pout = *(int *)Pin;
# ifdef BYTE_SWAP
      SWAP_WORD;
# endif
    }
  }

  if (!strcmp (type, "var64")) {
    for (i = 0; i < Nval*Nrow; i++, Pin+=Nbytes, Pout+=Nbytes) {
      *(int *)Pout = *(int *)Pin;
# ifdef BYTE_SWAP
      SWAP_DBLE;
# endif
    }
  }

  if (CHECK_MEMBLOCKS) {
    OhanaMemblock *ref = (OhanaMemblock *) array - 1;
    fprintf (stderr, "ref: 0x%08lx, array: 0x%08lx, size: %zd, (%s@%d : %s), freed: %1d, Nalloc: %d\n", (long int) ref, (long int) array, ref->size, ref->file, ref->line, ref->func, ref->freed, ref->Nalloc);
    if (!ref->nextBlock && !ref->prevBlock) abort();
  }

  /* check array space */
  if (Nx*Ny < Nx*(Nrow - 1) + Nstart + Nval*Nbytes) {
    fprintf (stderr, "mismatch in array sizes\n");
    return (FALSE);
  }

  // ohana_memcheck (TRUE);
  /* insert bytes from array into appropriate section of buffer */
  Pout = table[0].buffer + Nstart;
  Pin  = array;
  for (i = 0; i < Nrow; i++, Pout += Nx, Pin += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

# if (OHANA_MEMCHECK)
  myAssert ( tlabel[255] == 0x7f, "oops");
  myAssert (  field[255] == 0x7f, "oops");
  myAssert ( format[255] == 0x7f, "oops");
  myAssert (   type[63]  == 0x7f, "oops");
  myAssert (tmpline[63]  == 0x7f, "oops");
# endif
  // ohana_memcheck (TRUE);

  if (CHECK_MEMBLOCKS) {
    OhanaMemblock *ref = (OhanaMemblock *) array - 1;
    fprintf (stderr, "ref: 0x%08lx, array: 0x%08lx, size: %zd, (%s@%d : %s), freed: %1d, Nalloc: %d\n", (long int) ref, (long int) array, ref->size, ref->file, ref->line, ref->func, ref->freed, ref->Nalloc);
    if (!ref->nextBlock && !ref->prevBlock) abort();
  }

  free (array);

  return (TRUE);
}

/***********************/
// convert the input data array (of the specified intype) to the desired table data type. swap unless nativeOrder is requested
int gfits_set_bintable_column_reformat (Header *header, FTable *table, char *label, char *intype, void *data, off_t Nrow, int element, char nativeOrder) {

  off_t Nx, Ny;
  int i, N, Nfields;
  int Nval, NbytesOut, Nstart, Nv, Nb;
  char tlabel[256], field[256], format[256], outtype[64], tmpline[64];
  char *Pin, *Pout, *array;
  double Bscale, Bzero;

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
  snprintf (field, 256, "TSCAL%d", N);
  if (!gfits_scan (header, field, "%lf", 1, &Bscale)) {
    Bscale = 1.0;
  }
  snprintf (field, 256, "TZERO%d", N);
  if (!gfits_scan (header, field, "%lf", 1, &Bzero)) {
    Bzero = 0.0;
  }
  snprintf (field, 256, "TFORM%d", N);
  gfits_scan (header, field, "%s", 1, format);

  if (!gfits_bintable_format (format, outtype, &Nval, &NbytesOut)) return (FALSE);
  if (element >= Nval) {
    fprintf (stderr, "programming error: element >= Nval for field %s, format %s\n", label, format);
    return FALSE;
  }
  
  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  if (Ny == 0) { 
    Ny = Nrow;
    gfits_set_table_rows (header, table, Ny);
  }
  if (Ny != Nrow) return (FALSE);

  /* scan columns to find insert point */
  Nstart = 0;
  for (i = 1; i < N; i++) {
    snprintf (field, 256, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  // NOTE: we are inserting a single column into the output
  // table.  If the output field is multi-value, the resulting
  // array is inserted into the appropriate bytes in that output field

  /* make duplicate of data with correct type
     byte swap and Bzero/Bscale */
  ALLOCATE (array, char, NbytesOut*Nrow);
  Pin = data;
  Pout = array; // 

  // # define ASSIGN_DATA(ITYPE,INAME,ISIZE,OTYPE,ONAME)
  // if (!strcmp (intype, #ITYPE)) {

  int directCopy = (Bzero == 0.0) && (Bscale == 1.0);

# define SET_VALUES(OUTNAME, OUTTYPE, INNAME, INTYPE, SWAP_OP, NBYTES_IN) \
  if (!strcmp (outtype, OUTNAME) && !strcmp (intype, INNAME)) {		\
    int NbytesIn = NBYTES_IN;						\
    for (i = 0; i < Nrow; i++, Pin += NbytesIn, Pout += NbytesOut) { \
      if (directCopy) {							\
	*(OUTTYPE *)Pout = *(INTYPE *)Pin;				\
      } else {								\
	*(OUTTYPE *)Pout = (*(INTYPE *)Pin - Bzero) / Bscale;		\
      }									\
      if (!nativeOrder) { SWAP_OP; }}}					

  SET_VALUES("char",       char, "char", char, SWAP_NONE, 1);
  SET_VALUES("gfbyte",   gfbyte, "char", char, SWAP_NONE, 1);
  SET_VALUES("short",     short, "char", char, SWAP_BYTE, 1);
  SET_VALUES("int",         int, "char", char, SWAP_WORD, 1);
  SET_VALUES("int64_t", int64_t, "char", char, SWAP_DBLE, 1);
  SET_VALUES("float",     float, "char", char, SWAP_WORD, 1);
  SET_VALUES("double",   double, "char", char, SWAP_DBLE, 1);

  SET_VALUES("char",       char, "gfbyte", gfbyte, SWAP_NONE, 1);
  SET_VALUES("gfbyte",   gfbyte, "gfbyte", gfbyte, SWAP_NONE, 1);
  SET_VALUES("short",     short, "gfbyte", gfbyte, SWAP_BYTE, 1);
  SET_VALUES("int",         int, "gfbyte", gfbyte, SWAP_WORD, 1);
  SET_VALUES("int64_t", int64_t, "gfbyte", gfbyte, SWAP_DBLE, 1);
  SET_VALUES("float",     float, "gfbyte", gfbyte, SWAP_WORD, 1);
  SET_VALUES("double",   double, "gfbyte", gfbyte, SWAP_DBLE, 1);

  SET_VALUES("char",       char, "short", short, SWAP_NONE, 2);
  SET_VALUES("gfbyte",   gfbyte, "short", short, SWAP_NONE, 2);
  SET_VALUES("short",     short, "short", short, SWAP_BYTE, 2);
  SET_VALUES("int",         int, "short", short, SWAP_WORD, 2);
  SET_VALUES("int64_t", int64_t, "short", short, SWAP_DBLE, 2);
  SET_VALUES("float",     float, "short", short, SWAP_WORD, 2);
  SET_VALUES("double",   double, "short", short, SWAP_DBLE, 2);

  SET_VALUES("char",       char, "int", int, SWAP_NONE, 4);
  SET_VALUES("gfbyte",   gfbyte, "int", int, SWAP_NONE, 4);
  SET_VALUES("short",     short, "int", int, SWAP_BYTE, 4);
  SET_VALUES("int",         int, "int", int, SWAP_WORD, 4);
  SET_VALUES("int64_t", int64_t, "int", int, SWAP_DBLE, 4);
  SET_VALUES("float",     float, "int", int, SWAP_WORD, 4);
  SET_VALUES("double",   double, "int", int, SWAP_DBLE, 4);

  SET_VALUES("char",       char, "int64_t", int64_t, SWAP_NONE, 8);
  SET_VALUES("gfbyte",   gfbyte, "int64_t", int64_t, SWAP_NONE, 8);
  SET_VALUES("short",     short, "int64_t", int64_t, SWAP_BYTE, 8);
  SET_VALUES("int",         int, "int64_t", int64_t, SWAP_WORD, 8);
  SET_VALUES("int64_t", int64_t, "int64_t", int64_t, SWAP_DBLE, 8);
  SET_VALUES("float",     float, "int64_t", int64_t, SWAP_WORD, 8);
  SET_VALUES("double",   double, "int64_t", int64_t, SWAP_DBLE, 8);

  SET_VALUES("char",       char, "float", float, SWAP_NONE, 4);
  SET_VALUES("gfbyte",   gfbyte, "float", float, SWAP_NONE, 4);
  SET_VALUES("short",     short, "float", float, SWAP_BYTE, 4);
  SET_VALUES("int",         int, "float", float, SWAP_WORD, 4);
  SET_VALUES("int64_t", int64_t, "float", float, SWAP_DBLE, 4);
  SET_VALUES("float",     float, "float", float, SWAP_WORD, 4);
  SET_VALUES("double",   double, "float", float, SWAP_DBLE, 4);

  SET_VALUES("char",       char, "double", double, SWAP_NONE, 8);
  SET_VALUES("gfbyte",   gfbyte, "double", double, SWAP_NONE, 8);
  SET_VALUES("short",     short, "double", double, SWAP_BYTE, 8);
  SET_VALUES("int",         int, "double", double, SWAP_WORD, 8);
  SET_VALUES("int64_t", int64_t, "double", double, SWAP_DBLE, 8);
  SET_VALUES("float",     float, "double", double, SWAP_WORD, 8);
  SET_VALUES("double",   double, "double", double, SWAP_DBLE, 8);

  /* check array space */
  if (Nx*Ny < Nx*(Nrow - 1) + Nstart + Nval*NbytesOut) {
    fprintf (stderr, "mismatch in array sizes\n");
    return (FALSE);
  }

  /* insert bytes from array into appropriate section of buffer */
  Pout = table[0].buffer + Nstart + element*NbytesOut;
  Pin  = array;
  for (i = 0; i < Nrow; i++, Pout += Nx, Pin += NbytesOut) {
    memcpy (Pout, Pin, NbytesOut);
  }

  free (array);
  return (TRUE);
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
   P - var lenght array descrpt (64 bit)
   
   all can be preceeded by integer N which specified number
   of entries in array
   
*/


/***********************/
int gfits_set_table_column (Header *header, FTable *table, char *label, void *data, off_t Nrow) {

  off_t Nx, Ny;
  int i, N, Nfields, Nval, Nbytes, Nstart, Nv, Nb;
  char tlabel[256], field[256], format[256], cformat[256], type[64], tmp[64];
  char *array, *Pin, *Pout, *line;

  if (label == (char *) NULL) return (FALSE);
  if (label[0] == 0) return (FALSE);

  /* find label in header */
  tlabel[0] = 0;
  gfits_scan (header, "TFIELDS", "%d", 1, &Nfields);
  for (i = 1; strcmp (label, tlabel) && (i < Nfields + 1); i++) {
    snprintf (field, 256, "TTYPE%d", i);
    gfits_scan (header, field, "%s", 1, tlabel);
  }
  if (strcmp (label, tlabel)) return (FALSE);
  N = i - 1;

  /* interpret format */
  snprintf (field, 256, "TFORM%d", N);
  gfits_scan (header, field, "%s", 1, format);

  if (!gfits_table_format (format, type, &Nval, &Nbytes)) return (FALSE);
  strcpy (cformat, format);
  
  /* check existing table dimensions */
  gfits_scan (header, "NAXIS1", OFF_T_FMT, 1,  &Nx);
  gfits_scan (header, "NAXIS2", OFF_T_FMT, 1,  &Ny);
  if (Ny == 0) { 
    Ny = Nrow;
    gfits_set_table_rows (header, table, Ny);
  }
  if (Ny != Nrow) return (FALSE);

  /* scan columns to find insert point */
  Nstart = 0;
  for (i = 1; i < N; i++) {
    snprintf (field, 256, "TFORM%d", i);
    gfits_scan (header, field, "%s", 1, format);
    gfits_table_format (format, tmp, &Nv, &Nb);
    Nstart += Nv*Nb;
  }

  /* print line with appropriate formatting */
  ALLOCATE (array, char, Nbytes*Nval*Nrow);
  ALLOCATE (line, char, Nbytes+1);
  Pin = data;
  Pout = array;
  if (!strcmp (type, "char")) {
    for (i = 0; i < Nbytes*Nval*Nrow; i++, Pin++, Pout++) {
      *Pout = *Pin;
    }
  }
  if (!strcmp (type, "int")) {
    for (i = 0; i < Nrow; i++, Pin+=4, Pout+=Nbytes) {
      snprintf (line, Nbytes + 1, cformat, *(int *)Pin);
      memcpy (Pout, line, Nbytes);
    }
  }
  if (!strcmp (type, "float")) {
    for (i = 0; i < Nrow; i++, Pin+=4, Pout+=Nbytes) {
      snprintf (line, Nbytes + 1, cformat, *(float *)Pin);
      memcpy (Pout, line, Nbytes);
    }
  }

  /* check array space */
  if (Nx*Ny < Nx*(Nrow - 1) + Nstart + Nval*Nbytes) {
    fprintf (stderr, "mismatch in array sizes\n");
    return (FALSE);
  }

  /* insert bytes from array into appropriate section of buffer */
  Pout = table[0].buffer + Nstart;
  Pin  = array;
  for (i = 0; i < Nrow; i++, Pout += Nx, Pin += Nval*Nbytes) {
    memcpy (Pout, Pin, Nval*Nbytes);
  }

  free (line);
  free (array);
  return (TRUE);
}

/*
  valid TABLE column formats:
  FN.N  - floating point
  INN   - integer
  ANN   - NN char string
  
*/

