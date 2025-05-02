# include "Ximage.h"

void Reorient (Graphic *graphic, KapaImageWidget *image, double X, double Y, int mode) {

  Picture *picture;

  picture = &image[0].picture;

  if (picture[0].expand == 0) {
      picture[0].expand = 1;
      image[0].zoom.expand = 5;
  }

  if ((picture[0].Xc == X) && (picture[0].Yc == Y) && (mode == 0)) {
    Refresh ();
    XFlush (graphic[0].display);
    return;
  }

  switch (mode) {
  case 0:
    if ((picture[0].Xc != X) || (picture[0].Yc != Y)) {
      picture[0].Xc = X;
      picture[0].Yc = Y;
    }
    break;
  case -1: 
    picture[0].expand--;
    if ((picture[0].expand == 0) || (picture[0].expand == -1)) picture[0].expand = -2;
    picture[0].Xc = X;
    picture[0].Yc = Y;
    break;
  case +1:
    picture[0].expand++;
    if ((picture[0].expand == 0) || (picture[0].expand == -1)) picture[0].expand = 1;
    picture[0].Xc = X;
    picture[0].Yc = Y;
    break;
  }
  image[0].zoom.expand = MIN(image[0].zoom.dx / 5.5, MAX(5, 2*picture[0].expand));

  Remap (graphic, image);
  Refresh ();
 
  XFlush (graphic[0].display);
}
