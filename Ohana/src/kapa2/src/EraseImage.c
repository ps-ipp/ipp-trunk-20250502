# include "Ximage.h"

int EraseImage () {

  Graphic *graphic;
  Section *section;
  KapaImageWidget *image;

  graphic = GetGraphic();
  section = GetActiveSection();
  image = section->image;
  if (image == NULL) return (TRUE);

  FreeImage (image);
  section->image = NULL;

  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  
  SetSectionSizes (section);
  Refresh ();
  return (TRUE);
}
