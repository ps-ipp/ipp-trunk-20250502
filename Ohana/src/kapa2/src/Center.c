# include "Ximage.h"

int Center (int sock) {

  int zoom;
  double X, Y;
  Graphic *graphic;
  Section *section;
  KapaImageWidget *image;

  KiiScanMessage (sock, "%lf %lf %d", &X,  &Y, &zoom);

  graphic = GetGraphic();
  section = GetActiveSection();
  image = section->image;
  if (image == NULL) return (TRUE);

  // enforce integer center here?
  // NAN value means retain existing center
  if (isfinite(X)) image[0].picture.Xc = X;
  if (isfinite(Y)) image[0].picture.Yc = Y;
  if ((zoom != 0) && (zoom != -1)) {
    image[0].picture.expand = zoom;
    image[0].zoom.expand = MIN(image[0].zoom.dx / 5.5, MAX(5, 2*zoom));
  }

  if (USE_XWINDOW) {
    Remap (graphic, image);
    Refresh ();
    XFlush (graphic[0].display);
  }

  return (TRUE);
}

int Parity (int sock) {

  int X, Y;
  Graphic *graphic;
  Section *section;
  KapaImageWidget *image;

  KiiScanMessage (sock, "%d %d", &X,  &Y);

  graphic = GetGraphic();
  section = GetActiveSection();
  image = section->image;
  if (image == NULL) return (TRUE);

  image[0].picture.flipx = X;
  image[0].picture.flipy = Y;

  image[0].zoom.flipx 	 = X;
  image[0].zoom.flipy 	 = Y;

  image[0].wide.flipx 	 = X;
  image[0].wide.flipy 	 = Y;

  if (USE_XWINDOW) {
    Remap (graphic, image);
    Refresh ();
    XFlush (graphic[0].display);
  }

  return (TRUE);
}

