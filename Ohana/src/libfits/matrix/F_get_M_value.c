# include <ohana.h>
# include <gfitsio.h>

/***************** fits get matrix value ***********************************/
double gfits_get_matrix_value (Matrix *matrix, off_t x, off_t y) {

  double value;
  int pixel;

  if ((x < 0) || (x >= matrix[0].Naxis[0]) || (y < 0) || (y >= matrix[0].Naxis[1]))
    return (0.0);
  pixel = matrix[0].Naxis[0]*y + x;
  value = NAN;

  if (matrix[0].unsign) {
    switch (matrix[0].bitpix) {
    case 8:
      value = *((unsigned char  *) matrix[0].buffer + pixel);
      break;
    case 16:
      value = *((unsigned short *) matrix[0].buffer + pixel);
      break;
    case 32:
      value = *((unsigned int   *) matrix[0].buffer + pixel);
      break;
    case -32:
      value = *((float          *) matrix[0].buffer + pixel);
      break;
    case -64:
      value = *((double         *) matrix[0].buffer + pixel);
      break;
    }
  }
  else {
    switch (matrix[0].bitpix) {
    case 8:
      value = *((char  *) matrix[0].buffer + pixel);
      break;
    case 16:
      value = *((short *) matrix[0].buffer + pixel);
      break;
    case 32:
      value = *((int   *) matrix[0].buffer + pixel);
      break;
    case -32:
      value = *((float          *) matrix[0].buffer + pixel);
      break;
    case -64:
      value = *((double         *) matrix[0].buffer + pixel);
      break;
    }
  }

  value = matrix[0].bscale*value + matrix[0].bzero;
  return (value);

}


