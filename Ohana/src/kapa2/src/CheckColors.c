# include "Ximage.h"

/************** CheckColors *************/
void CheckColors (Graphic *graphic, int *argc, char **argv) {

  char *temp_name;
  int N;

  graphic->fore = BlackPixel (graphic->display, graphic->screen);
  temp_name = XGetDefault (graphic->display, argv[0], "Foreground");
  if ((N = get_argument (*argc, argv, "-fg"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is -fg color\n");
      exit (2);
    }
    remove_argument (N, argc, argv);
    temp_name = argv[N];
    remove_argument (N, argc, argv);
  } 
  if (temp_name != NULL) {
    graphic->fore = GetColor (graphic->display, temp_name, graphic->colormap, graphic->fore);
  }

  graphic->back = WhitePixel (graphic->display, graphic->screen);
  temp_name = XGetDefault (graphic->display, argv[0], "Background");
  if ((N = get_argument (*argc, argv, "-bg"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is -bg color\n");
      exit (2);
    }
    remove_argument (N, argc, argv);
    temp_name = argv[N];
    remove_argument (N, argc, argv);
  } 
  if (temp_name != NULL) {
    graphic->back = GetColor (graphic->display, temp_name, graphic->colormap, graphic->back);
  }
  return;
}

  /* here we define the values for foreground and background
     if -fg, or -bg exist, or if Foreground or Background are set in .Xdefaults, 
     use those.  foreground defaults to black, background defaults to white. */

