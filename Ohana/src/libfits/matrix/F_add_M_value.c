# include <ohana.h>
# include <gfitsio.h>

/***************** fits add matrix value ***********************************/
void gfits_add_matrix_value (Matrix *matrix, off_t x, off_t y, double Value) 
{

  double value;

  value = gfits_get_matrix_value (matrix, x, y);
  value += Value;
  gfits_set_matrix_value (matrix, x, y, value);

}


