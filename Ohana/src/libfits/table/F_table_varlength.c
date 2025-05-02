# include <ohana.h>
# include <gfitsio.h>

// fills the column definition structure for a specific column based on a given table.
int gfits_varlength_column_define (FTable *ftable, VarLengthColumn *def, int column) {

  int i;
  int Nv, Nb;
  char *p1, *p2, *p3;
  char field[256];
  char format[256];
  char tmpline[256];

  // grab the value of TFORMn for this column
  snprintf (field, 80, "TFORM%d", column);
  if (!gfits_scan (ftable->header, field, "%s", 1, format)) return (FALSE);

  def->column = column;

  // find and remove the max field length element
  p1 = strchr (format, '(');
  p2 = strchr (format, ')');

  if (!p1 || !p2) return (FALSE); // not a valid varlength column -- missing (e_max)
  if (p2 - p1 < 2) return (FALSE); // not a valid varlength column -- contains ()

  def->maxlen = strtol (p1 + 1, &p3, 10);

  if (p3 != p2) return (FALSE); // not a valid varlength column -- (e_max) contains extra chars
  *p1 = 0; // make the format string end here for the rest of the function

  // first char may optionally be 0, 1
  p1 = format;
  if ((*p1 == '0') || (*p1 == '1')) p1 ++;

  // now p1 must be 'P';
  if (*p1 == 0) return (FALSE);
  if ((*p1 != 'P') && (*p1 != 'Q')) return (FALSE);
  def->mode = *p1;

  // next value is the actual varlength column format
  p1 ++;
  if (*p1 == 0) return (FALSE);
  if (*p1 == 'P') { return (FALSE); }

  def->nbytes = 0;
  if (*p1 == 'X') { def->nbytes = 1;  def->format = *p1; }
  if (*p1 == 'L') { def->nbytes = 1;  def->format = *p1; }
  if (*p1 == 'A') { def->nbytes = 1;  def->format = *p1; }
  if (*p1 == 'B') { def->nbytes = 1;  def->format = *p1; }
  if (*p1 == 'I') { def->nbytes = 2;  def->format = *p1; }
  if (*p1 == 'J') { def->nbytes = 4;  def->format = *p1; }
  if (*p1 == 'K') { def->nbytes = 8;  def->format = *p1; }
  if (*p1 == 'E') { def->nbytes = 4;  def->format = *p1; }
  if (*p1 == 'D') { def->nbytes = 8;  def->format = *p1; }
  if (*p1 == 'C') { def->nbytes = 8;  def->format = *p1; }
  if (*p1 == 'M') { def->nbytes = 16; def->format = *p1; }
  if (!def->nbytes) { return (FALSE); }
  
  /* scan columns to find column offset for metadata column*/
  def->offset = 0;
  for (i = 1; i < column; i++) {
    snprintf (field, 256, "TFORM%d", i);
    gfits_scan (ftable->header, field, "%s", 1, format);
    gfits_bintable_format (format, tmpline, &Nv, &Nb);
    def->offset += Nv*Nb;
  }

  // heap_start must be long long so file may be very large
  // confirm that ftable->heap_start is correctly set?

  return TRUE;
}

# define SWAP_WORD \
  tmp = pchar[0]; pchar[0] = pchar[3]; pchar[3] = tmp; \
  tmp = pchar[1]; pchar[1] = pchar[2]; pchar[2] = tmp;

# define SWAP_DBLE \
  tmp = pchar[0]; pchar[0] = pchar[7]; pchar[7] = tmp; \
  tmp = pchar[1]; pchar[1] = pchar[6]; pchar[6] = tmp; \
  tmp = pchar[2]; pchar[2] = pchar[5]; pchar[5] = tmp; \
  tmp = pchar[3]; pchar[3] = pchar[4]; pchar[4] = tmp;

int gfits_byteswap_varlength_column (FTable *ftable, int column) {

# ifdef BYTE_SWAP  
  VarLengthColumn zdef;
  if (!gfits_varlength_column_define (ftable, &zdef, column)) return FALSE;

  char *bufstart = &ftable->buffer[zdef.offset];

  int i;
  char *pchar, tmp;

  off_t Nx = ftable->header->Naxis[0];

  for (i = 0; i < ftable->header->Naxis[1]; i++) {
    if (zdef.mode == 'P') {
      pchar = &bufstart[i*Nx];
      SWAP_WORD;
      pchar = &bufstart[i*Nx + 4];
      SWAP_WORD;
    }      
    if (zdef.mode == 'Q') {
      char *pchar, tmp;
      pchar = &bufstart[i*Nx];
      SWAP_DBLE;
      pchar = &bufstart[i*Nx + 8];
      SWAP_DBLE;
    }
  }
# endif
  return TRUE;
}

// return the data and length for row 'row' of a variable length column
// is this capable of handling large files (value holding length and offset is an int)?
void *gfits_varlength_column_pointer (FTable *ftable, VarLengthColumn *column, off_t row, off_t *length) {

  if ((column->mode != 'P') && (column->mode != 'Q')) abort();

  // find the values for the specified row
  // the values in the main table for this row and varlength column:
  off_t Nx = ftable->header->Naxis[0];
  off_t offset = 0;

  if (column->mode == 'P') {
    int *ptr;
    ptr = (int *) &ftable->buffer[row*Nx + column->offset];
    *length = ptr[0];
    offset = ptr[1];
  } 
  if (column->mode == 'Q') {
    off_t *ptr;
    ptr = (off_t *) &ftable->buffer[row*Nx + column->offset];
    *length = ptr[0];
    offset = ptr[1];
  }

  // fprintf (stderr, "length: %d, offset: %d\n", (int) *length, (int) offset);

  void *result = (void *) (ftable->buffer + ftable->heap_start + offset);
  return result;
}

// to generate a compressed image or a compressed table, we need to be able to add data
// for varlength columns to the heap.  To start, assume we have defined the cartesian
// portion of the table correctly, and are only extending the heap.

int gfits_varlength_column_add_data (FTable *table, char *data, off_t Ndata, int row, VarLengthColumn *column) {

  // find the current starting point for new data (end of current buffer main data + current heap
  off_t heap_offset = gfits_data_min_size (table->header);

  // extend the buffer size to add the new data
  off_t Nbytes = gfits_data_pad_size (heap_offset + Ndata);
  REALLOCATE (table->buffer, char, Nbytes);

  char *heap_ptr = table->buffer + heap_offset;

  // add the new data to the table buffer
  memcpy (heap_ptr, data, Ndata);

  // zero out the extra bytes
  off_t Nextra = Nbytes - (heap_offset + Ndata);
  memset (&heap_ptr[Ndata], 0, Nextra);

  // *** now we add the data description to the table ***

  if ((column->mode != 'P') && (column->mode != 'Q')) abort();

  // find the cell for the specified row & column
  // the values in the main table for this row and varlength column:
  int Nx = table->header->Naxis[0];

  // set the cell values
  if (column->mode == 'P') {
    int *ptr;
    ptr = (int *) &table->buffer[row*Nx + column->offset];
    ptr[0] = Ndata;
    ptr[1] = heap_offset - table->heap_start;
  } 
  if (column->mode == 'Q') {
    off_t *ptr;
    ptr = (off_t *) &table->buffer[row*Nx + column->offset];
    ptr[0] = Ndata;
    ptr[1] = heap_offset - table->heap_start;
    // fprintf (stderr, "row: %d, Ndata: %d, maxlen: %d, Ndata: %d, ptr1: %d\n", (int) row, (int) Ndata, (int) column->maxlen, (int) Ndata, (int) (heap_offset - table->heap_start));
  }

  // update maxlen
  // fprintf (stderr, "row: %d, Ndata: %d, maxlen: %d\n", (int) row, (int) Ndata, (int) column->maxlen);
  column->maxlen = MAX (column->maxlen, Ndata);

  off_t pcount;
  gfits_scan (table->header, "PCOUNT", OFF_T_FMT, 1, &pcount);

  pcount += Ndata;
  gfits_modify (table->header, "PCOUNT", OFF_T_FMT, 1, pcount);
  table->header->pcount = pcount;
  table->datasize = gfits_data_size (table->header);
  myAssert (table->datasize == Nbytes, "inconsistent header and data");

  return TRUE;
}

// update the header TFORM value for this entry
int gfits_varlength_column_finish (FTable *ftable, VarLengthColumn *def) {

  char field[81];
  char format[81];

  // grab the value of TFORMn for this column
  snprintf (field, 80, "TFORM%d", def->column);
  if (!gfits_scan (ftable->header, field, "%s", 1, format)) return FALSE;

  // find and update the max field length element
  char *p1 = strchr (format, '(');
  if (!p1) return (FALSE); // not a valid varlength column -- missing (e_max)
  
  p1 ++;
  *p1 = 0;

  snprintf (p1, 80 - (p1 - format), "%d)", def->maxlen);
  if (!gfits_modify (ftable->header, field, "%s", 1, format)) return FALSE;

  return TRUE;
}
