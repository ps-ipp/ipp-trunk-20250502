# include <ohana.h>
# include <gfitsio.h>

/******************* fits multiply matrix ***********************************/
int gfits_multiply_matrix (Matrix *M1, Matrix *M2, Matrix *M3) {

  int i,j;
  double value;

  if ((M1[0].Naxis[0] != M2[0].Naxis[0]) ||
      (M1[0].Naxis[1] != M2[0].Naxis[1])) 
    return (FALSE);

  for (i = 0; i < M1[0].Naxis[0]; i++) {
    for (j = 0; j < M1[0].Naxis[1]; j++) {
      value = gfits_get_matrix_value (M1, i, j);
      value *= gfits_get_matrix_value (M2, i, j);
      gfits_set_matrix_value (M3, i, j, value);
    }
  }
  
  return (TRUE);

}


