# include "Ximage.h"

void help(void);

int args (int *argc, char **argv) {

  int N;

  if ((N = get_argument (*argc, argv, "-h"))) help();
  if ((N = get_argument (*argc, argv, "-help"))) help();
  if ((N = get_argument (*argc, argv, "--h"))) help();
  if ((N = get_argument (*argc, argv, "--help"))) help();

  MAP_WINDOW = TRUE;
  if ((N = get_argument (*argc, argv, "-nomap"))) {
    remove_argument(N, argc, argv);
    MAP_WINDOW = FALSE;
  }

  NAME_WINDOW = NULL;
  if ((N = get_argument (*argc, argv, "-name"))) {
    remove_argument(N, argc, argv);
    NAME_WINDOW = strcreate (argv[N]);
    remove_argument(N, argc, argv);
  }

  if ((N = get_argument (*argc, argv, "-memdump"))) {
    remove_argument(N, argc, argv);
    MemoryDumpSetOnExit (TRUE);
  }

  USE_XWINDOW = TRUE;
  if ((N = get_argument (*argc, argv, "-noX"))) {
    remove_argument(N, argc, argv);
    USE_XWINDOW = FALSE;
  }

  if ((N = get_argument (*argc, argv, "-debug"))) {
    remove_argument(N, argc, argv);
    DEBUG = TRUE;
  } else {
    DEBUG = FALSE;
  }

  FOREGROUND = FALSE;
  if ((N = get_argument (*argc, argv, "-fg"))) {
    remove_argument(N, argc, argv);
    FOREGROUND = TRUE;
  }

  NPIXELS_DYNAMIC = 128;
  NPIXELS_STATIC = 128;
  if ((N = get_argument (*argc, argv, "-ncolors"))) {
    remove_argument(N, argc, argv);
    NPIXELS_DYNAMIC = atoi (argv[N]);
    NPIXELS_STATIC = NPIXELS_DYNAMIC;
    remove_argument(N, argc, argv);
  } 

  NAN_RED   = 0;
  NAN_GREEN = 0xffff;
  NAN_BLUE  = 0;
  if ((N = get_argument (*argc, argv, "-nan"))) {
    if (N > *argc - 4) {
      fprintf (stderr, "USAGE: kapa -nan (red) (green) (blue)\n");
      exit (2);
    }
    remove_argument(N, argc, argv);
    NAN_RED = strtol(argv[N], NULL, 0);
    remove_argument(N, argc, argv);
    NAN_GREEN = strtol(argv[N], NULL, 0);
    remove_argument(N, argc, argv);
    NAN_BLUE = strtol(argv[N], NULL, 0);
    remove_argument(N, argc, argv);
  }

  return (TRUE);
}

void help (void) {

  fprintf (stderr, "USAGE: kapa [options]\n");
  fprintf (stderr, "OPTIONS: \n");
  fprintf (stderr, "-h, -help, --h, --help : this listing\n");
  fprintf (stderr, "-nomap : use X but do not generate a window\n");
  fprintf (stderr, "-name : name for the X window bar\n");
  fprintf (stderr, "-noX : do not use an X window\n");
  fprintf (stderr, "-debug : report debug info\n");
  fprintf (stderr, "-fg : define the foreground color (X color style)\n");
  fprintf (stderr, "-ncolors : set the number of colors (pixels) available (128 default)\n");
  fprintf (stderr, "-nan (red) (green) (blue) : set NaN pixels to this color (hex values)\n");
  exit (2);
}
