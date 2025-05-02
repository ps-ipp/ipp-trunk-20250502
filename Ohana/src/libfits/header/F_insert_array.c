# include <ohana.h>
# include <gfitsio.h>

/*********************** fits insert array ***********************************/
void gfits_insert_matrix (matrix, array, x, y) 
Matrix *matrix, *array; 
int x, y;
{

  /* there is no check here to match BITPIX, BZERO, or BSCALE */

  int i, start_m, Nbytes_m, Nbytes;

  Nbytes = array[0].Naxis[0]*abs(array[0].bitpix) / 8;
  Nbytes_m = matrix[0].Naxis[0]*abs(matrix[0].bitpix) / 8;
  start_m = y*Nbytes_m + x*abs(matrix[0].bitpix) / 8;

  for (i = 0; i < array[0].Naxis[1]; i++) {
    bcopy (&array[0].buffer[i*Nbytes], 
	   &matrix[0].buffer[start_m + i*Nbytes_m], Nbytes);
  }
  
}

