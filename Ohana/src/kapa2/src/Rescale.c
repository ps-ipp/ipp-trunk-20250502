# include "Ximage.h"

void Rescale (Graphic *graphic, KapaImageWidget *image, Matrix *matrix) {

  int i, j, DX, DY, value;
  float *iData;
  unsigned short *oData;
  float slope;
  float start;
  unsigned short MaxValue;

  // define the color transform parameters
  MaxValue = graphic[0].Npixels - 1;
  if (image[0].image[0].range != 0.0) {
    slope = graphic[0].Npixels / image[0].image[0].range;
    start = graphic[0].Npixels * image[0].image[0].zero / image[0].image[0].range;
  } else {
    slope = 1.0;
    start = image[0].image[0].zero;
  }

  DX = matrix[0].Naxis[0];
  DY = matrix[0].Naxis[1];

  oData = graphic[0].pixmap;
  iData = (float *) matrix[0].buffer;

  // convert pixel data values to pixel index values (0 - Npixel)
  for (i = 0; i < DX; i++) {
    for (j = 0; j < DY; j++, iData++, oData++) {
      value = slope * iData - start;
      if (value < 0) value = 0;
      if (value > MaxValue) value = MaxValue;
      *oData = value;
    }
  }
}

/** in order to call this function, graphic[0].pixmap must be allocated to match the
 * current matrix (current channel) 
 */
