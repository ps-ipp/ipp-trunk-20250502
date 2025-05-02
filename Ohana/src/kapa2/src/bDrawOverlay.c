# include "Ximage.h"
# define INFRONT 4

static char name[4][16] = {"red", "green", "blue", "yellow"};

void bDrawOverlay (bDrawBuffer *buffer, KapaImageWidget *image, int N) {

  int i;
  int dx, dy;
  int Xmin, Ymin, Xmax, Ymax;
  double expand, X, Y, dX, dY, pX, pY;
  bDrawColor color;
 
  /* translate color to bDrawColors : image[0].overlay[N].color */
  color = KapaColorByName (name[N]);
  bDrawSetStyle (buffer, color, 0, 0, 1.0);
  
  expand = 1.0;
  if (image[0].picture.expand > 0) {
    expand = image[0].picture.expand;
  }
  if (image[0].picture.expand < 0) {
    expand = 1.0 / fabs((double)image[0].picture.expand);
  }

  // Xmin = 0;
  // Ymin = 0;
  // Xmax = image[0].picture.dx;
  // Ymax = image[0].picture.dy;

  Xmin = image[0].picture.x;
  Ymin = image[0].picture.y;
  Xmax = image[0].picture.x + image[0].picture.dx; // maybe this should be just dx?
  Ymax = image[0].picture.y + image[0].picture.dy;

  if (N == INFRONT) {
    fprintf (stderr, "INFRONT deprecated\n");
    return;
  } 

  for (i = 0; i < image[0].overlay[N].Nobjects; i++) {
    // XXX the 0.5,0.5 offset is apparently needed here.
    // work on rationalizing these functions in the context of their different plotting types
    Image_to_Screen (&X, &Y, image[0].overlay[N].objects[i].x - 0.5, image[0].overlay[N].objects[i].y - 0.5, &image[0].picture);
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
	bDrawLine (buffer, X, Y, (X+dX), (Y+dY));
	break;
      case KII_OVERLAY_TEXT:
	bDrawRotText (buffer, X, Y, image[0].overlay[N].objects[i].text, 8, 0.0);
	break;
      case KII_OVERLAY_BOX:
	dx = MAX (abs(dX),2) / 2;
	dy = MAX (abs(dY),2) / 2;
	bDrawRectOpen (buffer, (X-dx), (Y-dy), (X+dx), (Y+dy));
	// bDrawRectOpen (buffer, (X-dx), (Y-dy), (X), (Y));
	break;
      case KII_OVERLAY_CIRCLE:
	dx = MAX (abs(dX),2);
	dy = MAX (abs(dY),2);
	if (image[0].overlay[N].objects[i].angle == 0.0) {
	  bDrawArc (buffer, X, Y, dx, dy, 0, 360);
	} else {
	  // moderately-stupid rotated ellipse drawing:
	  // ANGLE is distance ccw from the x-axis to the major axis
	  double x0, y0, x1, y1, t;
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
	    bDrawLine (buffer, x0, y0, x1, y1);
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
  
  /* translate color to bDrawColors : image[0].overlay[N].color */
  bDrawSetStyle (buffer, color, 0, 0, 1.0);
}

/* this routine is independent of the number of overlays */

