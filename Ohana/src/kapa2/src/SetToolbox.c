# include "Ximage.h"

// set the position of the image toolbox
void SetToolbox (int sock) {

  int location;
  Section *section;
  Graphic *graphic;

  KiiScanMessage (sock, "%d", &location);
  if ((location < 0) || (location > 4)) {
    fprintf (stderr, "invalid toolbox location %d\n", location);
    return;
  }

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image == NULL) { 
    section->image = InitImageWidget ();
  }
  section->image->location = location;
  SetSectionSizes (section);

  if (!USE_XWINDOW) return;

  Remap (graphic, section->image);
  if (DEBUG) fprintf (stderr, "remapped image\n");
  Refresh ();
  if (DEBUG) fprintf (stderr, "refreshed\n");
  XFlush (graphic->display);

  return;
}
