# include "Ximage.h"

static char name[4][16] = {"red", "green", "blue", "yellow"};

void PSOverlay (KapaImageWidget *image, int N, FILE *f, int extra) {

  int i;
  double X, Y, dX, dY;
  int Xmin, Ymin, Xmax, Ymax;
  double expand, pX, pY;
  bDrawColor color;
 
  /* translate color to bDrawColors : image[0].overlay[N].color */
  color = KapaColorByName (name[N]);
  fprintf (f, "%s setrgbcolor\n", KapaColorRGBString(color));
  
  expand = 1.0;
  if (image[0].picture.expand > 0) {
    expand = image[0].picture.expand;
  }
  if (image[0].picture.expand < 0) {
    expand = 1.0 / fabs((double)image[0].picture.expand);
  }

  Xmin = 0;
  Ymin = 0;
  Xmax = image[0].picture.dx;
  Ymax = image[0].picture.dy;

  for (i = 0; i < image[0].overlay[N].Nobjects; i++) {

    Image_to_Screen (&X, &Y, image[0].overlay[N].objects[i].x, image[0].overlay[N].objects[i].y, &image[0].picture);
    dX = image[0].overlay[N].objects[i].dx * expand;
    dY = image[0].overlay[N].objects[i].dy * expand;
    if (image[0].picture.flipx) dX *= -1;
    if (image[0].picture.flipy) dY *= -1;

    // PS coord system is flipped relative to screen
    Y = Ymax - Y;

    pX = (image[0].picture.flipx) ? -1.0 : +1.0;
    pY = (image[0].picture.flipy) ? -1.0 : +1.0;

    switch (image[0].overlay[N].objects[i].type) {
      case KII_OVERLAY_LINE:
	if (((X < Xmin) && (X + dX < Xmin)) || ((X > Xmax) && (X + dX > Xmax)) ||
	    ((Y < Ymin) && (Y + dY < Ymin)) || ((Y > Ymax) && (Y + dY > Ymax))) {
	  break;
	}
	fprintf (f, " %6.1f %6.1f %6.1f %6.1f L\n", X + extra, Y + extra, (X+dX + extra), (Y-dY + extra));
	break;
      case KII_OVERLAY_TEXT:
	if (((X < Xmin) && (X + dX < Xmin)) || ((X > Xmax) && (X + dX > Xmax)) ||
	    ((Y < Ymin) && (Y + dY < Ymin)) || ((Y > Ymax) && (Y + dY > Ymax))) {
	  break;
	}
	fprintf (f, "(%s) %6.1f %6.1f T\n", image[0].overlay[N].objects[i].text, X + extra, Y + extra); 
	break;
      case KII_OVERLAY_BOX:
	if (((X - 0.5*dX < Xmin) && (X + 0.5*dX < Xmin)) || ((X - 0.5*dX > Xmax) && (X + 0.5*dX > Xmax)) ||
	    ((Y - 0.5*dY < Ymin) && (Y + 0.5*dY < Ymin)) || ((Y - 0.5*dY > Ymax) && (Y + 0.5*dY > Ymax))) {
	  break;
	}
	fprintf (f, " %6.1f %6.1f %6.1f %6.1f B\n", (dX + 2*extra), (dY + 2*extra), (X - 0.5*dX - extra), (Y - 0.5*dY - extra));
	break;
      case KII_OVERLAY_CIRCLE:
	if (((X - dX < Xmin) && (X + dX < Xmin)) || ((X - dX > Xmax) && (X + dX > Xmax)) ||
	    ((Y - dY < Ymin) && (Y + dY < Ymin)) || ((Y - dY > Ymax) && (Y + dY > Ymax))) {
	  break;
	}
	// is this a true circle, or is it an ellipse?
	if (fabs(dX - dY) < 0.01) {
	  fprintf (f, " %6.1f %6.1f %6.1f C\n", X, Y, fabs(dX + extra));
	} else {
	  // moderately-stupid rotated ellipse drawing:
	  // ANGLE is distance ccw from the x-axis to the major axis
	  double x0, y0, x1, y1, t;
	  double angle = image[0].overlay[N].objects[i].angle * RAD_DEG;
	  double cs = cos(angle);
	  double sn = sin(angle);
	  x0 = X + pX*dX*cs;
	  y0 = Y - pY*dX*sn;
	  // XXX dt should be based on the size of the ellipse...
	  // 0.10 -> 60 segments on the ellipse
	  # define DT 0.1
	  for (t = DT; t < 2*M_PI + DT; t+=DT) {
	    x1 = X + pX*dX*cos(t)*cs + pX*dY*sin(t)*sn;
	    y1 = Y - pY*dX*cos(t)*sn + pY*dY*sin(t)*cs;
	    fprintf (f, " %6.1f %6.1f %6.1f %6.1f L\n", x0 + extra, y0 + extra, x1 + extra, y1 + extra);
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
}
