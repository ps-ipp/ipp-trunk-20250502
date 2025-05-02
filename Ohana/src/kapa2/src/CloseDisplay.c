# include "Ximage.h"

/************** CloseDisplay *************/
void CloseDisplay () {

  Graphic *graphic;

  if (!USE_XWINDOW) return;

  graphic = GetGraphic();
  XFreeFont (graphic->display, graphic->font); 
  XFreeGC (graphic->display, graphic->gc);
  XDestroySubwindows (graphic->display, graphic->window);
  XDestroyWindow (graphic->display, graphic->window);
  XFlush (graphic->display);
  XCloseDisplay (graphic->display);
}
