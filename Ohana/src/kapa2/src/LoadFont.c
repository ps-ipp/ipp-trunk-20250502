# include "Ximage.h"

/************** LoadFont *************/
void LoadFont (Graphic *graphic, int *argc, char **argv, char *default_name) {

  int N;
  char *name;

  name = XGetDefault (graphic->display, argv[0], "Font");
  if (name == NULL) name = default_name;

  /* check for command-line options */
  if ((N = get_argument (*argc, argv, "-font"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is -font fontname\n");
      exit (2);
    }
    remove_argument(N, argc, argv);
    name = argv[N];
    remove_argument(N, argc, argv);
  }   
  if ((N = get_argument (*argc, argv, "-fn"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is -fn fontname\n");
      exit (2);
    }
    remove_argument(N, argc, argv);
    name = argv[N];
    remove_argument(N, argc, argv);
  } 

  graphic->font = XLoadQueryFont (graphic->display, name);
  if ((graphic->font == NULL) && (name != default_name)) {
    fprintf (stderr, "Could not load requested font %s, trying %s\n", name, default_name);
    graphic->font = XLoadQueryFont (graphic->display, default_name);
  }
  if (graphic->font == NULL) {
    fprintf (stderr, "Could not load font %s, using %s\n", default_name, "fixed");
    graphic->font = XLoadQueryFont (graphic->display, "fixed");
  }
  if (graphic->font == NULL) {
    QuitX (graphic->display, "Error: could not load font");
  }

  XSetFont (graphic->display, graphic->gc, graphic->font[0].fid);
}
