# include "opihi.h"
void gfits_compress_timing ();
  
static int VectorGetMaxStringLength (Vector *vec) {

  int MaxLength = 0;

  if (vec[0].type != OPIHI_STR) return 0;

  for (int i = 0; i < vec[0].Nelements; i++) {
    MaxLength = MAX (MaxLength, strlen(vec[0].elements.Str[i]));
  }
  return MaxLength;
}

// write a set of vectors to a FITS FTable structure (vectors names become fits column names)
static int WriteVectorTable (FTable *ftable, char *extname, Vector **vec, int Nvec, char *format, char nativeOrder) {
  
  int j;

  Header *theader = ftable->header;
  gfits_create_table_header (theader, "BINTABLE", extname);

  // allocate an array of strings to represent the format for each output field
  // formats include single column formats (BIJKDE) and multi-column formats (e.g., 2I)
  // we will have no more than Nvec fields (but we can have fewer)
  int Nfield = 0;
  ALLOCATE_PTR (tformat, char *, Nvec);
  ALLOCATE_PTR (Nelement, int, Nvec);

  if (format) {
    // the bintable format string can defines the byte-width of each field and number of elements (columns per field).
    // valid output columns are currently:
    // B (char), I (16 bit short), J (32 bit int), E (32 bit float), D (64 bit double).
    // the format string is just the sequence of types, eg: LIIJEED.
    // it may have spaces or integer element counts:
    // "2D 4I EEJ"

    // *** validate the format string

    // as I parse each elements, if it is a digit, I need parse that value

    char *ptr = format;
    for (j = 0; j < Nvec; ) {
      while (*ptr && OHANA_WHITESPACE (*ptr)) ptr++;
      if (*ptr == 0) {
	gprint (GP_ERR, "error in binary table format %s (insufficient format chars)\n", format);
	goto escape;
      }

      // is there a leading integer?
      char *endptr;
      Nelement[Nfield] = strtol (ptr, &endptr, 10);
      if (endptr == ptr) {
	Nelement[Nfield] = 1;
      }
      ptr = endptr; // this should now point at the letter that is the format type
      if ((*ptr != 'B') && (*ptr != 'I') && (*ptr != 'J') && (*ptr != 'K') && (*ptr != 'D') && (*ptr != 'E') && (*ptr != 'A')) {
	gprint (GP_ERR, "error in binary table format %s: invalid character %c\n", format, *ptr);
	goto escape;
      }

      int Nchar = snprintf (tformat[Nfield], 0, "%d%c", Nelement[Nfield], *ptr);
      ALLOCATE (tformat[Nfield], char, Nchar + 1);
      int Nout = snprintf (tformat[Nfield], Nchar + 1, "%d%c", Nelement[Nfield], *ptr);
      myAssert (Nout <= Nchar, "oops");

      // tformat[2*j + 0] = *ptr;
      // tformat[2*j + 1] = 0; // a bit sleazy : use a 2xN string to store N 1-byte strings

      // For numeric multi-valued fields, the number of elements from the format defines the number of
      // vectors which go into that field.  For string-type vectors, the format specifies the maximum number of characters
      // that go into the field.

      // for example, a format code of 3E should match a list of three numeric-type vectors while a format code of 15A should
      // match a single string-type vector which will supply up to 15 chars per row.

      if (*ptr != 'A') {
	j += Nelement[Nfield]; // advance past Nelement vectors
      } else {
	if (vec[j][0].type != OPIHI_STR) {
	  gprint (GP_ERR, "error in binary table format %s (mismatch between string format and numeric vector %s)\n", format, vec[j][0].name);
	  goto escape;
	}
	j ++; // advance past a single string-type vector (validate that?)
      }
      if (j > Nvec) {
	gprint (GP_ERR, "error in binary table format %s (too few vectors for listed field)\n", format);
	goto escape;
      }

      ptr ++;
      Nfield ++;
    }
    while (*ptr && OHANA_WHITESPACE (*ptr)) ptr++;
    if (*ptr) {
      gprint (GP_ERR, "error in binary table format %s (extra characters in format)\n", format);
      goto escape;
    }
  } else {
    for (j = 0; j < Nvec; j++) {
      switch (vec[j][0].type) {
	case OPIHI_FLT:
	case OPIHI_INT:
	  // if the format is not defined, just use the native byte-widths
	  ALLOCATE (tformat[j], char, 2);
	  tformat[j][0] = (vec[j][0].type == OPIHI_FLT) ? 'D' : 'K';
	  tformat[j][1] = 0;
	  Nelement[j] = 1;
	  break;
	case OPIHI_STR:
	  // we need to examine the vector to determine the length
	  Nelement[j] = VectorGetMaxStringLength(vec[j]); 
	  int Nchar = snprintf (tformat[j], 0, "%d%c", Nelement[j], 'A');
	  ALLOCATE (tformat[j], char, Nchar + 1);
	  int Nout = snprintf (tformat[j], Nchar + 1, "%d%c", Nelement[j], 'A');
	  myAssert (Nout <= Nchar, "oops");
	  break;
      }
    }
    Nfield = Nvec;
  }

  // define the columns of the table.  XXX NOTE: we cannot have duplicate names in
  // output table (because the data goes to the named column below).  need to enforce
  // this somehow
  int ivec = 0;
  for (j = 0; j < Nfield; j++) {
    // XX need to loop over fields, and skip the additional vectors that are part of a field
    // this call supported multiple columns per named field
    gfits_define_bintable_column (theader, tformat[j], vec[ivec][0].name, NULL, NULL, 1.0, 0.0);
    if (vec[ivec][0].type == OPIHI_STR) {
      ivec ++;
    } else {
      ivec += Nelement[j];
    }
  }

  // need to free the array 
  for (j = 0; j < Nfield; j++) {
    free (tformat[j]);
  }
  free (tformat);

  // generate the output array that carries the data
  gfits_create_table (theader, ftable);

  // I need to add each vector in order, but I need to
  // track which field it corresponds to.

  // add the vectors to the output array
  for (ivec = 0, j = 0; j < Nfield; j++) {
    // the first vector provides the name for the field
    char *fieldname = vec[ivec][0].name;

    if (vec[ivec][0].type == OPIHI_STR) {
      // string-type vectors need to be copied into a contiguous buffer with the right dimensions:
      Vector *thisvec = vec[ivec];

      ALLOCATE_PTR (strbuffer, char, Nelement[j]*thisvec->Nelements);
      for (int i = 0; i < thisvec->Nelements; i++) {
	int nChar = MIN (strlen (thisvec->elements.Str[i]), Nelement[j]);
	// fprintf (stderr, "%d %d %d : %d : %d : %s\n", ivec, j, i, Nelement[j], nChar, thisvec->elements.Str[i]);
	memcpy (&strbuffer[i*Nelement[j]], thisvec->elements.Str[i], nChar);
      }
      gfits_set_bintable_column (theader, ftable, fieldname, strbuffer, thisvec->Nelements);
      free (strbuffer);
      ivec ++;
    } else {
      for (int k = 0; k < Nelement[j]; k++, ivec++) {
	Vector *thisvec = vec[ivec];
	switch (thisvec->type) {
	  case OPIHI_FLT:
	    gfits_set_bintable_column_reformat (theader, ftable, fieldname, "double",  thisvec->elements.Flt, thisvec->Nelements, k, nativeOrder);
	    break;
	  case OPIHI_INT:
	    gfits_set_bintable_column_reformat (theader, ftable, fieldname, "int64_t", thisvec->elements.Int, thisvec->Nelements, k, nativeOrder);
	    break;
	}
      }
    }
  }
  return (TRUE);

 escape:
  if (tformat) free (tformat);
  return (FALSE);
}
  
# define VERBOSE 0
int dump_raw_table (FTable *table, char *message) {
# if (VERBOSE)
  int i;
  fprintf (stderr, "%s data: ", message);
  for (i = 0; i < table->header->Naxis[0]*table->header->Naxis[1]; i++) {
    fprintf (stderr, "0x%02hhx ", table->buffer[i]);
  }
  fprintf (stderr, "\n");
# else
  OHANA_UNUSED_PARAM(table);
  OHANA_UNUSED_PARAM(message);
# endif
  return TRUE;
}

int dump_cmp_table (FTable *table, char *message) {
# if (VERBOSE)
  int i;
  fprintf (stderr, "%s pntr: ", message);
  for (i = 0; i < table->header->Naxis[0]*table->header->Naxis[1]; i++) {
    fprintf (stderr, "0x%02hhx ", table->buffer[i]);
  }
  fprintf (stderr, "\n");

  fprintf (stderr, "%s data: ", message);
  for (i = 0; i < table->header->pcount; i++) {
    fprintf (stderr, "0x%02hhx ", table->buffer[table->heap_start + i]);
  }
  fprintf (stderr, "\n");
# else
  OHANA_UNUSED_PARAM(table);
  OHANA_UNUSED_PARAM(message);
# endif
  return TRUE;
}

// insert a new header line, return end pointer
char *gfits_insert_header_line (Header *header, char *endptr, char *newline) {

  if (0) {
    char temp1[32], temp2[32];
    memset (temp1, 0, 32);
    memset (temp2, 0, 32);

    if (endptr) {
      memcpy (temp1, endptr, 8);
    }
    memcpy (temp2, newline, 8);
    fprintf (stderr, "insert: %s : %s\n", temp1, temp2);
  }

  /* find the END of the header, if not supplied */
  if (!endptr) {
    endptr = gfits_header_field (header, "END", 1);
    if (endptr == NULL) return NULL; 
  }

  /* is there enough space for 1 more line? */
  if (header[0].datasize - (endptr - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
    // no, so expand by a full header block (FT_RECORD_SIZE = 32*80 = 2880)
    header[0].datasize += FT_RECORD_SIZE;
    REALLOCATE (header[0].buffer, char, header[0].datasize);
    // re-find the "END" marker, in case new memory block is used 
    endptr = gfits_header_field (header, "END", 1);
    if (endptr == NULL) return NULL; 
    memset (endptr + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
  }

  /* push END line back 1 */
  memmove ((endptr + FT_LINE_LENGTH), endptr, FT_LINE_LENGTH);
  memset (endptr, ' ', FT_LINE_LENGTH);

  strncpy_nowarn (endptr, newline, 80);
  endptr += FT_LINE_LENGTH;

  return endptr;
}
  
static char *rawkeywords[] = {"SIMPLE", "BITPIX", "NAXIS", "PCOUNT", "GCOUNT", "EXTEND", "EXTNAME", "BSCALE", "BZERO", "TFIELDS", "TFORM", "TTYPE", "TZERO", "TSCAL", "        ", NULL};

// write a set of vectors to a FITS file (vectors names become fits column names)
int WriteVectorTableFITS (char *filename, char *extname, Header *extraheader, Vector **vec, int Nvec, int append, char *compress, char *format, int Ntile) {
  
  Header rawheader;
  Header cmpheader;
  FTable rawtable;
  FTable cmptable;

  FILE *f = NULL;

  /* open file for outuput */
  if (append) {
    f = fopen (filename, "a");
  } else {
    f = fopen (filename, "w");
  }
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for write : %s\n", filename);
    return (FALSE);
  }

  // init so free below does not fail if table is not created
  gfits_init_header (&rawheader);
  gfits_init_header (&cmpheader);
  gfits_init_table (&rawtable);
  gfits_init_table (&cmptable);

  rawtable.header = &rawheader;
  if (!WriteVectorTable (&rawtable, extname, vec, Nvec, format, (compress != NULL))) goto escape;
  // NOTE : for compression, the table is constructed in native byte order 

  FTable *outtable = &rawtable;
  Header *outheader = &rawheader;

  if (compress) {
    cmptable.header = &cmpheader;

    dump_raw_table (&rawtable, "wd raw");
    if (!gfits_compress_table (&rawtable, &cmptable, Ntile, compress)) goto escape;
    gfits_compress_timing ();

    int i, Nfields;
    if (!gfits_scan (&cmpheader, "TFIELDS", "%d", 1, &Nfields)) goto escape;
    for (i = 0; i < Nfields; i++) {
      gfits_byteswap_varlength_column (&cmptable, i+1);
    }
    dump_cmp_table (&cmptable, "wd cmp");

    outtable = &cmptable;
    outheader = &cmpheader;
  }

  if (!append) {
    // generate a blank PHU header
    Header header;
    Matrix matrix;
    gfits_init_header (&header);
    gfits_init_matrix (&matrix);
    header.extend = TRUE;
    gfits_create_header (&header);
    gfits_create_matrix (&header, &matrix);
    gfits_fwrite_header  (f, &header);
    gfits_fwrite_matrix  (f, &matrix);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }

  if (extraheader) {
    // copy keywords which are not the standard or table keywords
    char *buf = extraheader->buffer;
    char *endptr = NULL;
    for (int i = 0; i < extraheader->datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {

      if (0) { 
	char temp1[32];
	memset (temp1, 0, 32);
	memcpy (temp1, buf, 8);
	fprintf (stderr, "buffer: %s\n", temp1);
      }

      for (int j = 0; rawkeywords[j] != NULL; j++) {
	if (!strncmp (buf, rawkeywords[j], strlen(rawkeywords[j]))) goto skip_insert;
      }
      endptr = gfits_insert_header_line (outheader, endptr, buf);
      if (!endptr) {
	gprint (GP_ERR, "failed to update FITS header with extra keywords\n");
	return (FALSE);
      }
    skip_insert:
      continue;
    }
  }

  // write the actual table data
  gfits_fwrite_Theader (f, outheader);
  gfits_fwrite_table  (f, outtable);

  gfits_free_header (&rawheader);
  gfits_free_table (&rawtable);

  if (compress) {
    gfits_free_header (&cmpheader);
    gfits_free_table (&cmptable);
  }

  fclose (f);
  fflush (f);
  return (TRUE);

 escape:
  gfits_free_header (&rawheader);
  gfits_free_table (&rawtable);

  if (compress) {
    gfits_free_header (&cmpheader);
    gfits_free_table (&cmptable);
  }

  fclose (f);
  fflush (f);
  return (FALSE);
}
  
// read the complete set of vectors from the given FITS file & extension
// XXX not quite right : I need to merge multiple vectors together:
// do not associate with an Opihi vector until later in mextract
Vector **ReadVectorTableFITS (char *filename, char *extname, int *nvec) {
  
  Header header;
  FTable ftable;

  int i, j;
  FILE *f = NULL;

  /* open file for input */
  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read\n");
    return NULL;
  }

  ftable.header = &header;

  // read the full table data into a buffer
  if (!gfits_fread_ftable (f, &ftable, extname)) {
    fclose (f);
    return NULL;
  }

  // XXX handle binary and ascii tables (see read_vectors.c)
  // find the columns in the table
  int Nfields;
  gfits_scan (&header, "TFIELDS", "%d", 1, &Nfields);
  int Nrows = header.Naxis[1];

  // how many output vectors do we need?  depends on number of columns per field, but min is Nfields
  int Nvec = 0;
  int NVEC = Nfields;
  Vector **vec = NULL;
  ALLOCATE (vec, Vector *, NVEC);
  
  for (i = 0; i < Nfields; i++) {
    int Nval;
    char type[16], label[16], name[80];

    // determine the column name, data type, and number of sub-fields
    sprintf (label, "TTYPE%d", i + 1);
    int status = gfits_scan (&header, label, "%s", 1, name);
    assert (status);

    status = gfits_get_bintable_column_type_by_N (&header, i + 1, type, &Nval);
    assert (status);

    if (Nvec + Nval >= NVEC) {
      NVEC = Nvec + Nval + 16;
      REALLOCATE (vec, Vector *, NVEC);
    }

    int vecType = OPIHI_INT;
    if (!strcmp (type, "double") || !strcmp (type, "float")) {
      vecType = OPIHI_FLT;
    }

    // generate the needed vectors
    for (j = 0; j < Nval; j++) {
      vec[Nvec + j] = InitVector();
      if (Nval == 1) {
	strcpy (vec[Nvec + j]->name, name);
      } else {
	snprintf (vec[Nvec + j]->name, OPIHI_NAME_SIZE, "%s:%d", name, j);
      }
      ResetVector (vec[Nvec + j], vecType, Nrows);
    }

    // read the actual table data into a column
    void *data;
    status = gfits_get_bintable_column (&header, &ftable, name, &data);
    assert (status);

    if (!VectorAssignData(&vec[Nvec], type, data, Nrows, Nval)) {
      // free unneeded things
      gprint (GP_ERR, "trouble parsing data block type %s\n", type);
      return (NULL);
    }

    free (data);
    Nvec += Nval;
  }

  gfits_free_header (&header);
  gfits_free_table (&ftable);

  fclose (f);

  *nvec = Nvec;
  return vec;

//escape:
//  gfits_free_header (&header);
//  gfits_free_matrix (&matrix);
//  gfits_free_header (&theader);
//  gfits_free_table (&ftable);
//
//  fclose (f);
//  fflush (f);
//  return (FALSE);
}

# define ASSIGN_DATA(TYPE,DTYPE,OPTYPE)	       \
    /* assign the data to the actual vector */ \
    if (!strcmp (type, #TYPE)) { \
      DTYPE *Ptr = data;		    \
      for (k = 0; k < Nrows; k++) { \
	for (j = 0; j < Nval; j++, Ptr++) { \
	  vec[j][0].elements.OPTYPE[k] = *Ptr; \
	  } } return TRUE; }

int VectorAssignData (Vector **vec, char *type, void *data, int Nrows, int Nval) {

  int j, k;

  // assign the data to the actual vector
  ASSIGN_DATA(gfbyte,  gfbyte,  Int);
  ASSIGN_DATA(char,    char,    Int);
  ASSIGN_DATA(short,   short,   Int);
  ASSIGN_DATA(int,     int,     Int);
  ASSIGN_DATA(int64_t, int64_t, Int); // XXX this works if opihi_int is assigned to int64_t
//ASSIGN_DATA(int64_t, int64_t, Flt); // int64_t has a problem: Int is too small, Flt is wrong precision
  ASSIGN_DATA(float,   float,   Flt);
  ASSIGN_DATA(double,  double,  Flt);

  return FALSE;
}

# define ASSIGN_DATA_TRANSPOSE(TYPE,DTYPE,OPTYPE)	\
    /* assign the data to the actual vector */ \
    if (!strcmp (type, #TYPE)) { \
      DTYPE *Ptr = data;		    \
      for (k = 0; k < Nrows; k++) { \
	for (j = 0; j < Nval; j++, Ptr++) { \
	  vec[k][0].elements.OPTYPE[j] = *Ptr; \
	  } } return TRUE; }

int VectorAssignDataTranspose (Vector **vec, char *type, void *data, int Nrows, int Nval) {

  int j, k;

  // assign the data to the actual vector
  ASSIGN_DATA_TRANSPOSE(gfbyte,  gfbyte,  Int);
  ASSIGN_DATA_TRANSPOSE(char,    char,    Int);
  ASSIGN_DATA_TRANSPOSE(short,   short,   Int);
  ASSIGN_DATA_TRANSPOSE(int,     int,     Int);
  ASSIGN_DATA_TRANSPOSE(int64_t, int64_t, Int);
//ASSIGN_DATA_TRANSPOSE(int64_t, int64_t, Flt); // see above comment
  ASSIGN_DATA_TRANSPOSE(float,   float,   Flt);
  ASSIGN_DATA_TRANSPOSE(double,  double,  Flt);

  return FALSE;
}
