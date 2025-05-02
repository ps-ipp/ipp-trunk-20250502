# include "Ximage.h"

int EraseSections () {
  
  Graphic *graphic;

  graphic = GetGraphic();
  
  FreeSections ();
  AddSection ("default", 0.0, 0.0, 1.0, 1.0, -1);

  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  Refresh ();

  return (TRUE);
}
