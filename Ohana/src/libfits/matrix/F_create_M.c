# include <ohana.h>
# include <gfitsio.h>

/*********************** fits create matrix *******************************/
int gfits_create_matrix (Header *header, Matrix *matrix) {

  int i;
  off_t Nbytes;

  matrix[0].bitpix = header[0].bitpix;
  matrix[0].unsign = header[0].unsign;
  matrix[0].bscale = header[0].bscale;
  matrix[0].bzero  = header[0].bzero;
  matrix[0].Naxes  = header[0].Naxes;
  for (i = 0; i < FT_MAX_NAXES; i++)
    matrix[0].Naxis[i] = header[0].Naxis[i];

  Nbytes = gfits_data_size (header);
  ALLOCATE (matrix[0].buffer, char, MAX (Nbytes, 1));
  memset (matrix[0].buffer, 0, Nbytes);
  
  matrix[0].datasize = Nbytes;
  return (TRUE);

}

/*********************** fits create matrix *******************************/
int gfits_init_matrix (Matrix *matrix) {

  int i;

  matrix[0].bitpix = 8;
  matrix[0].unsign = FALSE;
  matrix[0].bscale = 1.0;
  matrix[0].bzero  = 0.0;
  matrix[0].Naxes  = 0;
  for (i = 0; i < FT_MAX_NAXES; i++) matrix[0].Naxis[i] = 0;

  matrix[0].buffer = NULL;
  matrix[0].datasize = 0;
  return (TRUE);

}

/*********************** fits create matrix *******************************/
Matrix *gfits_alloc_matrix (void) {

  Matrix *matrix = NULL;
  ALLOCATE (matrix, Matrix, 1);
  gfits_init_matrix (matrix);
  return matrix;
}

/*********************** return number of valid pixels *******************************/
off_t gfits_npix_matrix (Matrix *matrix) {

  if (!matrix->Naxes) return 0;

  if (!matrix->Naxis[0]) return 0;

  int i;
  int Npix = 1;
  for (i = 0; i < matrix->Naxes; i++) {
    if (matrix->Naxis[i] == 0) break;
    Npix *= matrix->Naxis[i];
  }
  return Npix;
}
