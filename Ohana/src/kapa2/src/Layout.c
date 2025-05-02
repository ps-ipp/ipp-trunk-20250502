# include "Ximage.h"
static char default_colormap[] = "grayscale";

void InitLayout (int argc, char **argv) {

  int N;
  char *namedSocket = NULL;
  
  namedSocket = NULL;
  if ((N = get_argument (argc, argv, "-socket"))) { 
    remove_argument (N, &argc, argv);
    namedSocket = argv[N];
    remove_argument (N, &argc, argv);
  }

  // if we specify a named socket, wait until we are contacted
  // otherwise, open an INET sock and check occasionally for requests
  InitPipe (namedSocket);

  ACTIVE_CURSOR = FALSE;

  /* get the display colors */
  if (USE_XWINDOW) {
    MakeColormap (argc, argv);
  } else {
    char *colormap;
    colormap = default_colormap;
    if ((N = get_argument (argc, argv, "-cm"))) {
      remove_argument (N, &argc, argv);
      colormap = argv[N];
    }
    SetColormap (colormap);
  }

  /* move this out of here... */
  if (argc != 1) {
    fprintf (stderr, "USAGE: kapa\n");
    exit (1);
  }

  InitRotFonts ();

  /* create basic section, empty of image or graph */
  // Section *section is returned by not used
  AddSection ("default", 0.0, 0.0, 1.0, 1.0, -1);
}

void FreeLayout (void) {
  FreeSections ();
  FreeRotFonts ();
}
