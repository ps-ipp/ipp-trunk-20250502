# include "Ximage.h"

/************** QuitX *************/
void QuitX (Display *display, char *error_message) {

  fprintf (stderr, "Error: %s\n", error_message);
  XCloseDisplay (display);
  exit (3);
}
