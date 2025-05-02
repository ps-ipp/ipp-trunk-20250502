# include <ohana.h>
# include <gfitsio.h>

/********************* fits divide matrix ***********************************/
int gfits_divide_matrix (Matrix *M1, Matrix *M2, Matrix *M3) {

  int i,j;
  double value, val2;

  if ((M1[0].Naxis[0] != M2[0].Naxis[0]) ||
      (M1[0].Naxis[1] != M2[0].Naxis[1])) 
    return (FALSE);

  for (i = 0; i < M1[0].Naxis[0]; i++) {
    for (j = 0; j < M1[0].Naxis[1]; j++) {
      value = gfits_get_matrix_value (M1, i, j);
      val2  = gfits_get_matrix_value (M2, i, j);
      if (val2 != 0) 
	value = value / val2;
      else
	value = 0.0;
      gfits_set_matrix_value (M3, i, j, value);
    }
  }
  
  return (TRUE);

}


