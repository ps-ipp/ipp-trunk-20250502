# include <ohana.h>
# include <gfitsio.h>

/*********************** fits free matrix ***********************************/
void gfits_free_matrix (Matrix *matrix) {

  if (matrix[0].buffer == (char *) NULL) return;
  free (matrix[0].buffer);
  matrix[0].buffer = (char *) NULL;
  
}


