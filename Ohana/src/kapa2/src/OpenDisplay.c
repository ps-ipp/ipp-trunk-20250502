# include "Ximage.h"

/************** OpenDisplay *************/
Display *OpenDisplay (char *name, int *screen) {

  Display *display;
  
  display = XOpenDisplay (name);
  if (display == NULL) {
    fprintf (stderr, "Error could not open X display to %s\n", XDisplayName (name));
    fprintf (stderr, "Running with background graphics\n");
    // exit (2);
    return NULL;
  }
  *screen = DefaultScreen (display);
  return (display);
}
