# include <ohana.h>
# include <gfitsio.h>

/*********************** fits insert array ***********************************/
void gfits_insert_matrix (Matrix *matrix, Matrix *array, off_t x, off_t y) {

  /* there is no check here to match BITPIX, BZERO, or BSCALE */

  int i;
  off_t start_m, Nbytes_m, Nbytes;

  if ((x + array[0].Naxis[0] > matrix[0].Naxis[0]) ||
      (y + array[0].Naxis[1] > matrix[0].Naxis[1])) {
    fprintf (stderr, "can't insert array here: ("OFF_T_FMT","OFF_T_FMT") - ("OFF_T_FMT","OFF_T_FMT") vs ("OFF_T_FMT","OFF_T_FMT")\n",
	      x, 
	      y, 
	      x + array[0].Naxis[0], 
	      y + array[0].Naxis[1],
	      matrix[0].Naxis[0], 
	      matrix[0].Naxis[1]);
    return;
  }

  Nbytes = array[0].Naxis[0]*abs(array[0].bitpix) / 8;
  Nbytes_m = matrix[0].Naxis[0]*abs(matrix[0].bitpix) / 8;
  start_m = y*Nbytes_m + x*abs(matrix[0].bitpix) / 8;

  for (i = 0; i < array[0].Naxis[1]; i++) {
    memmove (&matrix[0].buffer[start_m + i*Nbytes_m], 
	     &array[0].buffer[i*Nbytes], Nbytes);
  }
  
}

