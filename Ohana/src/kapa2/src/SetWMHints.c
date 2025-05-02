# include "Ximage.h"

/************** SetWMHints  *************/
void SetWMHints (Graphic *graphic, Icon *icon) {

  XWMHints *wmhints;

  wmhints = XAllocWMHints ();
  if (wmhints == NULL) return;

  wmhints[0].initial_state = NormalState;
  wmhints[0].input = True;
  if (icon[0].pixmap != (Pixmap) None) {
    wmhints[0].icon_pixmap = icon[0].pixmap;
    wmhints[0].icon_mask = icon[0].pixmap;
    wmhints[0].flags = StateHint | InputHint | IconPixmapHint | IconMaskHint;
  } else {
    wmhints[0].flags = StateHint | InputHint;
  }
    
  XSetWMHints (graphic->display, graphic->window, wmhints);
  XFree (wmhints);
}
