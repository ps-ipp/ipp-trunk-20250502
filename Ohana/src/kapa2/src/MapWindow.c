# include "Ximage.h"

/************** MapWindow *************/
void MapWindow (Graphic *graphic) {

  XMapRaised (graphic->display, graphic->window);
  XMapSubwindows (graphic->display, graphic->window);
  FlushDisplay ();
}
