# include "Ximage.h"

int UpdatePointer (Graphic *graphic, XMotionEvent *event) {

  int textpad;
  double  x, y, z;
  float *data;
  char line[100];
  Section *section;
  KapaImageWidget *image;

  // XXX select the window element which contains the event
  section = GetActiveSection();
  image   = section->image;
  if (image == NULL) return (TRUE);
  if (!image[0].location) return (TRUE);

  if (image[0].MovePointer && InPicture ((XButtonEvent *)event, &image[0].picture)) {

    data = (float *) image[0].image[0].matrix.buffer;
    Screen_to_Image (&x, &y, event[0].x + 0.5, event[0].y + 0.5, &image[0].picture);

    z = -1;
    if (x < 0) goto skip;
    if (x >= image[0].image[0].matrix.Naxis[0]) goto skip;
    if (y < 0) goto skip;
    if (y >= image[0].image[0].matrix.Naxis[1]) goto skip;
    z = data[(int)(y)*image[0].image[0].matrix.Naxis[0] + (int)(x)];

  skip:
    image[0].zoom.Xc = x;
    image[0].zoom.Yc = y;
    
    UpdateStatusBox (graphic, image, x, y, z, 0);
    XFlush (graphic[0].display);
      
    CreateZoom (graphic, image);  
    if (image[0].zoom.pix) {
	XPutImage (graphic[0].display, graphic[0].window, graphic[0].gc,
		   image[0].zoom.pix, 0, 0, 
		   image[0].zoom.x, image[0].zoom.y, 
		   image[0].zoom.dx, image[0].zoom.dy);
    }
    CrossHairs (graphic, &image[0].zoom);
    XFlush (graphic[0].display);
  }
  
  if (InPicture ((XButtonEvent *)event, &image[0].cmapbar)) {
    z = image[0].image[0].zero  + image[0].image[0].range * (event[0].x - image[0].cmapbar.x) / image[0].cmapbar.dx;
    textpad = graphic[0].font[0].ascent;
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc,
                  image[0].text_x + 1, image[0].text_y + 1, ZOOM_X - 2, textpad + PAD1 + 1);
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
    bzero (line, 100);
    sprintf (line, "%22.3f", z);
    XDrawString (graphic[0].display, graphic[0].window, graphic[0].gc, 
                 image[0].text_x + PAD1, image[0].text_y + textpad + PAD1, 
		 line, strlen(line));
    
  }

  return (TRUE);
}
