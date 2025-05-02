# include "Ximage.h"

void PaintOverlay (Graphic *graphic, KapaImageWidget *image, int N) {

  int i;
  int dX, dY, dx, dy;
  int Xmin, Ymin, Xmax, Ymax;
  double t, expand, X, Y, pX, pY;
 
  XSetForeground (graphic[0].display, graphic[0].gc, image[0].overlay[N].color);
  XSetLineAttributes (graphic->display, graphic->gc, 1, LineSolid, CapNotLast, JoinMiter);
  
  expand = 1.0;
  if (image[0].picture.expand > 0) {
    expand = image[0].picture.expand;
  }
  if (image[0].picture.expand < 0) {
    expand = 1.0 / fabs((double)image[0].picture.expand);
  }

  Xmin = image[0].picture.x;
  Ymin = image[0].picture.y;
  Xmax = image[0].picture.x + image[0].picture.dx;
  Ymax = image[0].picture.y + image[0].picture.dy;

  for (i = 0; i < image[0].overlay[N].Nobjects; i++) {
    Image_to_Screen (&X, &Y, image[0].overlay[N].objects[i].x, image[0].overlay[N].objects[i].y, &image[0].picture);
    dX = image[0].overlay[N].objects[i].dx * expand;
    dY = image[0].overlay[N].objects[i].dy * expand;
    if (image[0].picture.flipx) dX *= -1;
    if (image[0].picture.flipy) dY *= -1;

    if (X + dX < Xmin) continue;
    if (X - dX > Xmax) continue;
    if (Y + dY < Ymin) continue; 
    if (Y - dY > Ymax) continue;

    pX = (image[0].picture.flipx) ? -1.0 : +1.0;
    pY = (image[0].picture.flipy) ? -1.0 : +1.0;

    /* for a LINE, (x, y) is the start, (dx, dy) is the distance to end
       for a CIRCLE (x, y) is the center, (dx, dy) is the radius 
       for a BOX (x, y) is the center, (dx, dy) is the width */

    switch (image[0].overlay[N].objects[i].type) {
      case KII_OVERLAY_LINE:
	// the angle makes no sense for a line...
	XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y, (X+dX), (Y+dY));
	break;
      case KII_OVERLAY_TEXT:
	// XXX currently we ignore the rectangle angle
	XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, X, Y, image[0].overlay[N].objects[i].text, strlen(image[0].overlay[N].objects[i].text));
	break;
      case KII_OVERLAY_BOX:
	dx = MAX (abs(dX),2) / 2;
	dy = MAX (abs(dY),2) / 2;
	// XXX currently we ignore the rectangle angle
	XDrawRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, (X - dx), (Y - dy), 2*dx, 2*dy);
	break;
      case KII_OVERLAY_CIRCLE:
	dx = MAX (abs(dX),2);
	dy = MAX (abs(dY),2);
	if (image[0].overlay[N].objects[i].angle == 0.0) {
	  XDrawArc (graphic[0].display, graphic[0].window, graphic[0].gc, (X - dx), (Y - dy), 2*dx, 2*dy, 0, 23040);
	} else {
	  // very stupid rotated ellipse drawing:
	  double x0, y0, x1, y1;
	  double angle = image[0].overlay[N].objects[i].angle * RAD_DEG;
	  double cs = cos(angle);
	  double sn = sin(angle);
	  x0 = X + pX*dx*cs;
	  y0 = Y + pY*dx*sn;
	  // XXX dt should be based on the size of the ellipse...
	  // 0.10 -> 60 segments on the ellipse
	  # define DT 0.1
	  for (t = DT; t < 2*M_PI + DT; t+=DT) {
	    x1 = X + pX*dx*cos(t)*cs - pX*dy*sin(t)*sn;
	    y1 = Y + pY*dx*cos(t)*sn + pY*dy*sin(t)*cs;
	    XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, x0, y0, x1, y1);
	    x0 = x1;
	    y0 = y1;
	  }
	}
	break;
      default:
	fprintf (stderr, "skipping unknown object\n");
	break;
    }
  }
  
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
  XSetLineAttributes (graphic->display, graphic->gc, 1, LineSolid, CapNotLast, JoinMiter);
  return;
}
