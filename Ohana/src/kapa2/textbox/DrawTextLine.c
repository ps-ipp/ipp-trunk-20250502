# include "Ximage.h"
# define XBorder 5
# define YBorder 2

DrawTextLine (graphic, textline)
Graphic  graphic[];
TextLine textline[];
{

  int y;

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);
  if (textline[0].outline) {
    XDrawRectangle (graphic[0].display, 
		    graphic[0].window,
		    graphic[0].gc,
		    textline[0].x,  textline[0].y,
		    textline[0].dx, textline[0].dy);
  }

  y = textline[0].y + (textline[0].dy + graphic[0].font[0].ascent) / 2;
  XDrawString (graphic[0].display, 
	       graphic[0].window, 
	       graphic[0].gc, 
	       textline[0].x + XBorder, y,
	       textline[0].label, 
	       strlen(textline[0].label));
  RedrawString (graphic, textline);

  if (textline[0].cursor != -1)
    DrawCursor (graphic, textline);

}

