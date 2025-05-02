# include "Ximage.h"

// XXX Should there be a base command + KiiMessage command?
int Resize (int sock) {
 
  int i, Nsection;
  unsigned int NX, NY;
  Graphic *graphic;
  Section *section;

  graphic = GetGraphic();

  // must scan the message before possible return
  KiiScanMessage (sock, "%u %u", &NX, &NY);

  // XXX keep this min limit (or modify for !USE_XWINDOW)?
  NX = MAX(NX, MIN_WIDTH); 
  NY = MAX(NY, MIN_HEIGHT); 

  // if the new size is the same as the old size, do nothing.
  if ((graphic->dx == NX) && (graphic->dy == NY)) return (TRUE);

  // set the new window size
  graphic->dx = NX; 
  graphic->dy = NY; 

  if (USE_XWINDOW) XResizeWindow (graphic->display, graphic->window, NX, NY);

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

// resise the window so the image in the currently active window fills its section
int ResizeByImage (int sock) {
  OHANA_UNUSED_PARAM(sock);
 
  int i, Nsection;
  unsigned int NX, NY;
  double dx, dy;
  int dXm, dXp, dYm, dYp;
  double x0, y0, x1, y1, expand;
  Section *section;
  Graphic *graphic;
  KapaImageWidget *image;

  graphic = GetGraphic();

  section = GetActiveSection();

  image = section->image;
  if (!image) {
    fprintf (stderr, "no image to define size\n");
    return (TRUE);
  }

  GetGraphBoundary (section, &x0, &y0, &x1, &y1, &dXm, &dXp, &dYm, &dYp);

  expand = 1.0;
  if (image[0].picture.expand > 0) {
    expand = image[0].picture.expand;
  }
  if (image[0].picture.expand < 0) {
    expand = 1.0 / fabs((double)image[0].picture.expand);
  }

  // pixel dimensions of the imaging region + boundary
  dx = image[0].image[0].matrix.Naxis[0]*expand + x1;
  dy = image[0].image[0].matrix.Naxis[1]*expand + y1;

  NX = dx / section[0].dx;
  NY = dy / section[0].dy;

  NX = MAX(NX, MIN_WIDTH); 
  NY = MAX(NY, MIN_HEIGHT); 

  // if the new size is the same as the old size, do nothing.
  if ((graphic->dx == NX) && (graphic->dy == NY)) return (TRUE);

  // set the new window size
  graphic->dx = NX; 
  graphic->dy = NY; 

  if (USE_XWINDOW) XResizeWindow (graphic->display, graphic->window, NX, NY);

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
