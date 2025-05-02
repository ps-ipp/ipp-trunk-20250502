# include "Ximage.h"

/************** TopWindow *************/
void TopWindow (Graphic *graphic, Icon *icon) {

  Window rootwindow;

  rootwindow = RootWindow (graphic->display, graphic->screen);

  CreateWindow (graphic, rootwindow, BORDER_WIDTH, EVENT_MASK);
  MakeGC (graphic);

  icon[0].pixmap = XCreateBitmapFromData (graphic->display, graphic->window, (char *) icon[0].bits, icon[0].width, icon[0].height);

  MakeCursor (graphic, DEFAULT_CURSOR);
  XFreeCursor (graphic->display, graphic->cursor);
}
