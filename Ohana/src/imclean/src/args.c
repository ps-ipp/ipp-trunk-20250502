# include "imclean.h"
# define NARGS 2  /* minimum is: addstar (filename) */

void help () {

  fprintf (stderr, "USAGE: imclean (file.fits) (file.obj) (file.cmp)\n");
  exit (2);

}

void args (int argc, char **argv) {
  
  int i, N;

  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  FITS_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "-fits"))) {
    FITS_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }
  HEADER_COORDS = TRUE;
  if ((N = get_argument (argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    RA = atof (argv[N]);
    remove_argument (N, &argc, argv);
    DEC = atof (argv[N]);
    remove_argument (N, &argc, argv);
    HEADER_COORDS = FALSE;
  }

  NEWPHOTCODE = FALSE;
  if ((N = get_argument (argc, argv, "-p"))) {
    NEWPHOTCODE = TRUE;
    remove_argument (N, &argc, argv);
    PHOTCODE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PROVIDE_ASTROM = FALSE;
  if ((N = get_argument (argc, argv, "-astrom"))) {
    remove_argument (N, &argc, argv);
    strcpy (AstromFile, argv[N]);
    remove_argument (N, &argc, argv);
    PROVIDE_ASTROM = TRUE;
  }

  MODE = DOPHOT;
  if ((N = get_argument (argc, argv, "-chad"))) {
    MODE = CHAD;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-sex"))) {
    MODE = SEXTRACT;
    remove_argument (N, &argc, argv);
  }

  ALLOCATE (KEYWORD, char *, 64);
  ALLOCATE (KEYVALU, char *, 64);
  ALLOCATE (KEYFMT, char *, 64);
  FIX_KEYWORD = 0;
  while ((N = get_argument (argc, argv, "-key"))) {
    i = FIX_KEYWORD;
    remove_argument (N, &argc, argv);
    KEYWORD[i] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    KEYFMT[i] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    KEYVALU[i] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    FIX_KEYWORD ++;
    if (FIX_KEYWORD == 64) break;
  }


  if (argc != 4) help ();

}

