# include "Ximage.h"

// erase just the current plot
int EraseCurrentPlot () {
  
  Graphic *graphic;
  Section *section;

  graphic = GetGraphic();

  section = GetActiveSection();
  if (section->graph == NULL) return (TRUE);

  EraseGraph (section->graph);
  
  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);

  SetSectionSizes (section);
  Refresh ();

  return (TRUE);
}
