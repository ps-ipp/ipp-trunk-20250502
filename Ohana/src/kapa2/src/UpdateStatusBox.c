# include "Ximage.h"

# define MY_LONG_LINE (LONG_LINE_LENGTH + 128)
void UpdateStatusBox (Graphic *graphic, KapaImageWidget *image, double x, double y, double z, int mode) {

  int textpad;
  double ra, dec; 
  char line[MY_LONG_LINE];

  XY_to_RD (&ra, &dec, x, y, &image[0].image[0].coords);

  textpad = graphic[0].font[0].ascent;

  if (mode) {
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc,
		    image[0].text_x, image[0].text_y, image[0].text_dx, image[0].text_dy);  
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
    XDrawRectangle (graphic[0].display, graphic[0].window, graphic[0].gc,
		    image[0].text_x, image[0].text_y, image[0].text_dx, image[0].text_dy);
  
    bzero (line, MY_LONG_LINE);
    snprintf (line, MY_LONG_LINE, "(%d x %d) @ %d   ch: %d                                     ", 
	      image[0].picture.dx, image[0].picture.dy, image[0].picture.expand, image[0].currentChannel+1); 
    XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
		 image[0].text_x + PAD1, image[0].text_y + 4*textpad + 4*PAD1, line, 25);
    
    bzero (line, MY_LONG_LINE);
    snprintf (line, MY_LONG_LINE, "%-25s", image[0].image[0].file); 
    XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
		 image[0].text_x + PAD1, image[0].text_y + 5*textpad + 5*PAD1, line, strlen(line));
    
    bzero (line, MY_LONG_LINE);
    snprintf (line, MY_LONG_LINE, "(%s)                                          ", image[0].image[0].name); 
    XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
		 image[0].text_x + PAD1, image[0].text_y + 6*textpad + 6*PAD1, line, 25);
    
  } else {
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
    // XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].overlay_color[1]);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc,
		    image[0].text_x+1, image[0].text_y+1, image[0].text_dx-1, image[0].text_dyo-2);  
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
  }
  bzero (line, MY_LONG_LINE);

  if (image[0].HexValue) {
      snprintf (line, MY_LONG_LINE, "%04x", (int) z);
  } else {
      snprintf (line, MY_LONG_LINE, "%22.3f", z);
  }
  XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
	       image[0].text_x + PAD1, image[0].text_y + textpad + PAD1, line, strlen(line));
  
  bzero (line, MY_LONG_LINE);
  snprintf (line, MY_LONG_LINE, "%10.2f %10.2f", x, y);
  XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
	       image[0].text_x + PAD1, image[0].text_y + 2*textpad + 2*PAD1, line, strlen(line));
  
  bzero (line, MY_LONG_LINE);
  if (image[0].DecimalDegrees) {
    snprintf (line, MY_LONG_LINE, "%10.6f %10.6f", ra, dec); 
  } else {
    hh_hms (line, ra, dec, ':', MY_LONG_LINE);
  }

  XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
	       image[0].text_x + PAD1, image[0].text_y + 3*textpad + 3*PAD1, line, strlen(line));
}
