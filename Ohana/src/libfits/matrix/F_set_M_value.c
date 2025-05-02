# include <ohana.h>
# include <gfitsio.h>

/****************** fits set matrix value ***********************************/
void gfits_set_matrix_value (Matrix *matrix, off_t x, off_t y, double Value) {

  off_t pixel;
  double value;

  pixel = matrix[0].Naxis[0]*y + x;
  value = (Value - matrix[0].bzero) / matrix[0].bscale;

  if (matrix[0].unsign) {
    switch (matrix[0].bitpix) {
    case 8:
      *((char  *) matrix[0].buffer + pixel) = value;
      break;
    case 16:
      *((short *) matrix[0].buffer + pixel) = value;
      break;
    case 32:
      *((int   *) matrix[0].buffer + pixel) = value;
      break;
    case -32:
      *((float          *) matrix[0].buffer + pixel) = value;
      break;
    case -64:
      *((double         *) matrix[0].buffer + pixel) = value;
      break;
    }
  }
  else {
    switch (matrix[0].bitpix) {
    case 8:
      *((unsigned char  *) matrix[0].buffer + pixel) = value;
      break;
    case 16:
      *((unsigned short *) matrix[0].buffer + pixel) = value;
      break;
    case 32:
      *((unsigned int   *) matrix[0].buffer + pixel) = value;
      break;
    case -32:
      *((float          *) matrix[0].buffer + pixel) = value;
      break;
    case -64:
      *((double         *) matrix[0].buffer + pixel) = value;
      break;
    }
  }

}


