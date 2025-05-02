# include "Ximage.h"

/************** GetColor *************/
unsigned long GetColor (Display *display, char *name, Colormap colormap, unsigned long default_color) {

  int status;
  XColor rgbcolor, hardwarecolor;

  status = XLookupColor (display, colormap, name, &rgbcolor, &hardwarecolor);
  if (!status) return (default_color);

  status = XAllocColor (display, colormap, &hardwarecolor);
  if (!status) return (default_color);

  return (hardwarecolor.pixel);
}
