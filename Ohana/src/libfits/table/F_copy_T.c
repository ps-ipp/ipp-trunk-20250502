# include <ohana.h>
# include <gfitsio.h>

/*********************** copy ftable (does not copy the header pointer) ***********************************/
int gfits_copy_ftable (FTable *in, FTable *out) {

  /* find buffer size */
  out[0].validsize  = in[0].validsize;
  out[0].datasize   = in[0].datasize;
  out[0].heap_start = in[0].heap_start;

  ALLOCATE (out[0].buffer, char, out[0].datasize);
  memcpy (out[0].buffer, in[0].buffer, out[0].datasize);
  return (TRUE);
}	

/*********************** copy ftable pointers (does not copy memory) ***********************************/
int gfits_copy_ftable_ptr (FTable *in, FTable *out) {

  if (!in)  return FALSE;
  if (!out) return FALSE;

  /* find buffer size */
  out[0].validsize  = in[0].validsize;
  out[0].datasize   = in[0].datasize;
  out[0].heap_start = in[0].heap_start;
  out[0].header     = in[0].header;
  out[0].buffer     = in[0].buffer;
  return (TRUE);
}	

/*********************** copy ftable (does not copy the header pointer) ***********************************/
int gfits_copy_vtable (VTable *in, VTable *out) {

  off_t i;

  /* find buffer size */
  off_t Nx = in[0].header[0].Naxis[0];
  // off_t Ny = in[0].header[0].Naxis[1];

  // validate these two?
  // table[0].datasize = gfits_data_size (table[0].header);
  // table[0].pad = table[0].datasize - Nx*Ny;

  /* find buffer size */
  out[0].datasize   = in[0].datasize;
  // out[0].heap_start = in[0].heap_start;
  out[0].pad        = in[0].pad;
  out[0].Nrow       = in[0].Nrow;

  off_t Nrows = out[0].Nrow;

  ALLOCATE (out[0].row, off_t, MAX (Nrows, 1));
  ALLOCATE (out[0].buffer, char *, MAX (Nrows, 1));
  for (i = 0; i < Nrows; i++) {
    out[0].row[i] = in[0].row[i];
    ALLOCATE (out[0].buffer[i], char, MAX (Nx, 1));
    memcpy (out[0].buffer[i], in[0].buffer[i], Nx);
  }
  return (TRUE);
}	

