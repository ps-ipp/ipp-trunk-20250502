# include "Ximage.h"

// XXX merge this code with Resize
int Reconfig (XEvent *event) {

  int i, Nsection, NX, NY;
  Graphic *graphic;
  Section *section;

  graphic = GetGraphic();

  // XXX keep this min limit (or modify for !USE_XWINDOW)?
  NX = MAX(event[0].xconfigure.width, MIN_WIDTH); 
  NY = MAX(event[0].xconfigure.height, MIN_HEIGHT); 

  // if the new size is the same as the old size, do nothing.
  if ((graphic->dx == NX) && (graphic->dy == NY)) {
    if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
    Refresh ();
    return (TRUE);
  }

  // set the new window size
  graphic->dx = NX; 
  graphic->dy = NY; 

  // reset the sizes for all sections
  Nsection = GetNumberOfSections ();
  for (i = 0; i < Nsection; i++) {
      section = GetSectionByNumber (i);
      SetSectionSizes (section);
  }

  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  Refresh ();

  return (TRUE);
}
