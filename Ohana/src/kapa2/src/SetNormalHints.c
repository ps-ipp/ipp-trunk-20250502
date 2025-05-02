# include "Ximage.h"

/************** SetNormalHints  *************/
void SetNormalHints (Graphic *graphic) {

  XSizeHints *sizehints;

  sizehints = XAllocSizeHints ();
  if (sizehints == NULL) return;

  sizehints[0].x = graphic->x;
  sizehints[0].y = graphic->x;
  sizehints[0].width = graphic->dx;
  sizehints[0].height = graphic->dy;
  sizehints[0].min_width = MIN_WIDTH;
  sizehints[0].min_height = MIN_HEIGHT;    

  // XXX : can we drop the position flag 
  // sizehints[0].flags = USPosition | USSize | PMinSize;
  sizehints[0].flags = USSize | PMinSize;

  sizehints[0].base_width = graphic->dx;
  sizehints[0].base_height = graphic->dy;
  sizehints[0].flags |= PBaseSize;
    
  XSetWMNormalHints (graphic->display, graphic->window, sizehints);
  XFree (sizehints);
}
