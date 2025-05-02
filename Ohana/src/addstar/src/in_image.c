# include "addstar.h"

int in_image (double r, double d, Image *image) {

  double X, Y;

  RD_to_XY (&X, &Y, r, d, &image[0].coords);
  if (X < 0) return (FALSE);
  if (Y < 0) return (FALSE);
  if (X >= image[0].NX) return (FALSE);
  if (Y >= image[0].NY) return (FALSE);
  return (TRUE);
}

