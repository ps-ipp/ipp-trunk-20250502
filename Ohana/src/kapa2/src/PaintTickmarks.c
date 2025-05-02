# include "Ximage.h"

void PaintTickmarks (Graphic *graphic, KapaImageWidget *image) {

  int i;
  int X, Y, dX, dY;
  int Xmin, Ymin, Xmax, Ymax, Xrange, Yrange;
 
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);

  Xmin = image[0].picture.x;
  Ymin = image[0].picture.y;
  Xmax = image[0].picture.x + image[0].picture.dx;
  Ymax = image[0].picture.y + image[0].picture.dy;
  Xrange = image[0].picture.dx;
  Yrange = image[0].picture.dy;

  for (i = 0; i < image[0].tickmarks.Nobjects; i++) {
    X  = image[0].tickmarks.objects[i].x * Xrange + Xmin;
    Y  = image[0].tickmarks.objects[i].y * Yrange + Ymin;
    dX = image[0].tickmarks.objects[i].dx * Xrange;
    dY = image[0].tickmarks.objects[i].dy * Yrange;

    switch (image[0].overlay[0].objects[i].type) {
      case KII_OVERLAY_LINE:
	XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y, (X+dX), (Y+dY));
	break;
      case KII_OVERLAY_TEXT:
	if (image[0].tickmarks.objects[i].dy == 0) {
	    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
	    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y-11, 6*strlen(image[0].tickmarks.objects[i].text), 11); 
	    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
	    XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y, image[0].tickmarks.objects[i].text, strlen(image[0].tickmarks.objects[i].text));
	}
	if (image[0].tickmarks.objects[i].dy == 90) {
	    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
	    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y-6*strlen(image[0].tickmarks.objects[i].text), 11, 6*strlen(image[0].tickmarks.objects[i].text)); 
	    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
	    /* XDrawRotString (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y, image[0].tickmarks.objects[i].text, strlen(image[0].tickmarks.objects[i].text)); */
	}
	break;
      case KII_OVERLAY_BOX:
	XDrawRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, (int)(X - 0.5*dX), (int)(Y - 0.5*dY), abs(dX), abs(dY));
	break;
      case KII_OVERLAY_CIRCLE:
	XDrawArc (graphic[0].display, graphic[0].window, graphic[0].gc, X - dX, Y - dY, abs(2*dX), abs(2*dY), 0, 23040);
	break;
      default:
	fprintf (stderr, "skipping unknown object\n");
	break;
    }
  }
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
}
