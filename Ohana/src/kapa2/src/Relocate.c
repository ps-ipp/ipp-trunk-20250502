# include "Ximage.h"

// XXX Should there be a base command + KiiMessage command?
int Relocate (int sock) {
 
  int x, y, dX, dY;
  Graphic *graphic;

  graphic = GetGraphic();

  KiiScanMessage (sock, "%d %d", &x, &y);

  if (!USE_XWINDOW) return (TRUE);

  dX = DisplayWidth  (graphic->display, graphic->screen);
  dY = DisplayHeight (graphic->display, graphic->screen);

  // let's not lose the window
  x = MAX(0, MIN(x, dX - 10)); 
  y = MAX(0, MIN(y, dY - 10)); 

  XMoveWindow (graphic->display, graphic->window, x, y);

  // if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  // Refresh ();

  return (TRUE);
}
