# include "Ximage.h"

void DrawButton (Graphic *graphic, Button *button) {
  
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
  XFillRectangle (graphic[0].display, 
		  graphic[0].window,
		  graphic[0].gc,
		  button[0].x,  button[0].y,
		  button[0].dx, button[0].dy);
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
  XDrawRectangle (graphic[0].display, 
		  graphic[0].window,
		  graphic[0].gc,
		  button[0].x,  button[0].y,
		  button[0].dx, button[0].dy);

  DrawBitmap (graphic, 
	      button[0].x + (button[0].dx - button[0].width) / 2 + 1, 
	      button[0].y + (button[0].dy - button[0].height) / 2 + 1, 
	      button[0].width, button[0].height, 
	      button[0].bitmap, 1);

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
}

# if (0)
  if (button[0].text) {
    dX = XTextWidth (graphic[0].font, button[0].bitmap, strlen(button[0].bitmap));
    y = button[0].y + (button[0].dy + graphic[0].font[0].ascent)/2;
    XDrawString (graphic[0].display, 
		 graphic[0].window, 
		 graphic[0].gc, 
		 button[0].x + (button[0].dx - dX) / 2, y,
		 button[0].bitmap, strlen(button[0].bitmap));
  } else {
    DrawBitmap (graphic, 
		button[0].x + (button[0].dx - button[0].width) / 2 + 1, 
		button[0].y + (button[0].dy - button[0].height) / 2 + 1, 
		button[0].width, button[0].height, 
		button[0].bitmap, 1);
  }
# endif
