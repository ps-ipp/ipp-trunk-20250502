# include "Ximage.h"

// erase all plots, keep the sections
int ErasePlots () {
  
  int i, Nsection;
  Graphic *graphic;
  Section *section;

  graphic = GetGraphic();
  
  // reset the sizes for all sections
  Nsection = GetNumberOfSections ();
  for (i = 0; i < Nsection; i++) {
      section = GetSectionByNumber (i);
      EraseGraph (section->graph);
  }

  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  Refresh ();

  return (TRUE);
}
