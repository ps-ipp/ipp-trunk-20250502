# include "Ximage.h"

/************** CreateWindow *************/
void CreateWindow (Graphic *graphic, Window parent, int border, long events) {

  XSetWindowAttributes attributes;
  unsigned long attribute_mask;
  Visual *visual = CopyFromParent;

  HAVE_BACKING = (DoesBackingStore (ScreenOfDisplay(graphic->display, graphic->screen)) == Always);
  HAVE_BACKING = FALSE;

  if (HAVE_BACKING) {
    attributes.backing_store = Always;
    attribute_mask = CWBackingStore | CWBackPixel | CWBorderPixel | CWEventMask;
  } else {
    attribute_mask = CWBackPixel | CWBorderPixel | CWEventMask;
  }

  attributes.background_pixel = graphic->back;
  attributes.border_pixel     = graphic->fore;
  attributes.event_mask       = events;

  graphic->window = XCreateWindow (graphic->display, parent, 
				     graphic->x, graphic->y, 
				     graphic->dx, graphic->dy, 
				     border, CopyFromParent,
				     InputOutput, visual, 
				     attribute_mask, &attributes);

  if (graphic->window == (Window) None)
    QuitX (graphic->display, "error: could not open window");

  XSelectInput (graphic->display, graphic->window, events);
}
