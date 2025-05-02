# include "Ximage.h"

void CrossHairs (Graphic *graphic, Picture *image) {

  int x0, x1, x5, x7;
  int y0, y2, y4, y5;
  double zoomscale;

  // is this totally wrong??
  zoomscale = image[0].expand;
  x0 = (zoomscale) * (0.5 * (ZOOM_X + 1) / zoomscale - 2.5) + image[0].x;
  x5 = (zoomscale) * (0.5 * (ZOOM_X + 1) / zoomscale - 0.5) + image[0].x;
  x7 = (zoomscale) * (0.5 * (ZOOM_X + 1) / zoomscale + 0.5) + image[0].x;
  x1 = (zoomscale) * (0.5 * (ZOOM_X + 1) / zoomscale + 2.5) + image[0].x;
						    	 
  y4 = (zoomscale) * (0.5 * (ZOOM_Y + 1) / zoomscale - 2.5) + image[0].y;
  y2 = (zoomscale) * (0.5 * (ZOOM_Y + 1) / zoomscale - 0.5) + image[0].y;
  y0 = (zoomscale) * (0.5 * (ZOOM_Y + 1) / zoomscale + 0.5) + image[0].y;
  y5 = (zoomscale) * (0.5 * (ZOOM_Y + 1) / zoomscale + 2.5) + image[0].y;


  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
  
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x0, y0, x5, y0);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x5, y0, x5, y5);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7, y5, x7, y0);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7, y0, x1, y0);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x0, y2, x5, y2);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x5, y2, x5, y4);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7, y4, x7, y2);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7, y2, x1, y2);

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x0, y0+1, x5-1, y0+1);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x5-1, y0+1, x5-1, y5);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7+1, y5, x7+1, y0+1);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7+1, y0+1, x1, y0+1);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x0, y2-1, x5-1, y2-1);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x5-1, y2-1, x5-1, y4);

  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7+1, y4, x7+1, y2-1);
  XDrawLine (graphic[0].display, graphic[0].window, graphic[0].gc, 
	     x7+1, y2-1, x1, y2-1);

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);

}
