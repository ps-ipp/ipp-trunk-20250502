# include "Ximage.h"

/************** MakeCursor *************/
void MakeCursor (Graphic *graphic, unsigned int cursor) {

  graphic->cursor = XCreateFontCursor (graphic->display, (unsigned) cursor);

  if (graphic->cursor != (Cursor) None) XDefineCursor (graphic->display, graphic->window, graphic->cursor);
}
