# include "dvoutils.h"

int dvoutils_args (int *argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-verbose"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  DVOUTILS_OP = DVOUTILS_NONE;
  if ((N = get_argument (*argc, argv, "-uniq-images"))) {
    DVOUTILS_OP = DVOUTILS_UNIQ_IMAGES;
    remove_argument (N, argc, argv);
    IMAGES_LIST = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-check-images"))) {
    DVOUTILS_OP = DVOUTILS_CHECK_IMAGES;
    remove_argument (N, argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    EXTERN_ID_LIST = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  if (*argc != 1) {
    fprintf (stderr, "USAGE: dvoutils -uniq-images (filename) [-v,-verbose]\n\n");
    fprintf (stderr, "USAGE: dvoutils -check-images (catdir) (externIDs) [-v,-verbose]\n\n");
    fprintf (stderr, "  -v : VERBOSE\n");
    exit (2);
  }

  if (DVOUTILS_OP == DVOUTILS_NONE) {
    fprintf (stderr, "ERROR: no valid mode selected\n");
    exit (2);
  }

  return TRUE;
}
