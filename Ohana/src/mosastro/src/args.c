# include "mosastro.h"

void print_help () {

  fprintf (stderr, "mosastro -- mosaic astrometry\n");
  fprintf (stderr, "\n"); 
  exit (0);

}

void args (int *argc, char **argv) {
  
  int N;

  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    print_help ();
  }

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  DUMP = NULL;
  if ((N = get_argument (*argc, argv, "-dump"))) {
    remove_argument (N, argc, argv);
    DUMP = strcreate(argv[N]);
    remove_argument (N, argc, argv);
  }

  SAVE_RESID = FALSE;
  if ((N = get_argument (*argc, argv, "-save-residuals"))) {
    remove_argument (N, argc, argv);
    SAVE_RESID = TRUE;
  }

  CHIPS = (char *) NULL;
  if ((N = get_argument (*argc, argv, "-chips"))) {
    remove_argument (N, argc, argv);
    CHIPS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  FIELD = (char *) NULL;
  if ((N = get_argument (*argc, argv, "-field"))) {
    remove_argument (N, argc, argv);
    FIELD = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  /** currently unused **/
  field.Norder = 0;
  if ((N = get_argument (*argc, argv, "-order"))) {
    remove_argument (N, argc, argv);
    field.Norder = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }

  /** currently unused **/
  ChipOrder = 1;
  if ((N = get_argument (*argc, argv, "-chiporder"))) {
    remove_argument (N, argc, argv);
    ChipOrder = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
}
