# include "Ximage.h"

void CreateZoom (Graphic *graphic, KapaImageWidget *image) {

  switch (graphic[0].Nbits) {
  case 8:
    Remap8  (graphic, image, &image->zoom, &image->image->matrix);
    break;
  case 16:
    Remap16 (graphic, image, &image->zoom, &image->image->matrix);
    break;
  case 24:
    Remap24 (graphic, image, &image->zoom, &image->image->matrix);
    break;
  case 32:
    Remap32 (graphic, image, &image->zoom, &image->image->matrix);
    break;
  }    

}
