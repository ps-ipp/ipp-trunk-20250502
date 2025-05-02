# include "Ximage.h"

void StatusBox (Graphic *graphic, KapaImageWidget *image) {

  double  x, y, z;

  z = -1;

  if (image[0].MovePointer) {
    x = 0.5*image[0].image[0].matrix.Naxis[0];
    y = 0.5*image[0].image[0].matrix.Naxis[1];
    // z = -1;
    image[0].zoom.Xc = x;
    image[0].zoom.Yc = y;
    // image[0].z = z;
  } else {
    x = image[0].zoom.Xc;
    y = image[0].zoom.Yc;
    // z = image[0].z;
  }
  
  UpdateStatusBox (graphic, image, x, y, z, 1);
}
