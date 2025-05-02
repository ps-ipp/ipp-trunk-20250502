# include <ohana.h>
# include <gfitsio.h>

/*********************** fits insert array ***********************************/
void gfits_add_matrix (Matrix *matrix, Matrix *array, off_t x, off_t y) 
{

  /* there is no check here to match BITPIX, BZERO, or BSCALE */

  int i, j;
  double V;

  if ((x + array[0].Naxis[0] > matrix[0].Naxis[0]) ||
      (y + array[0].Naxis[1] > matrix[0].Naxis[1])) {
    fprintf (stderr, "can't add array here: ("OFF_T_FMT","OFF_T_FMT") - ("OFF_T_FMT","OFF_T_FMT") vs ("OFF_T_FMT","OFF_T_FMT")\n",
	      x, 
	      y, 
	      x + array[0].Naxis[0], 
	      y + array[0].Naxis[1],
	      matrix[0].Naxis[0], 
	      matrix[0].Naxis[1]);
    return;
  }

  for (i = 0; i < array[0].Naxis[0]; i++) {
    for (j = 0; j < array[0].Naxis[1]; j++) {
      V = gfits_get_matrix_value (array, i, j);
      V += gfits_get_matrix_value (matrix, x + i, y + j);
      gfits_set_matrix_value (matrix, x + i, y + j, V);
    }
  }
}

