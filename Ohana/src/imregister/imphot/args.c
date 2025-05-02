# include "imregister.h" 
# include "imphot.h"

/* criteria struct is global */
int args (int argc, char **argv) {

  int N;

  ConfigInit (&argc, argv); /* load elixir config data */

  /* interpret command-line arguments */
  if (!get_trange_arguments (&argc, argv, &criteria.tstart, &criteria.tstop, &criteria.Ntimes)) {
    fprintf (stderr, "ERROR: syntax error\n");
    exit (1);
  }

  /* select by image photcode */
  criteria.PhotcodeSelect = FALSE;
  criteria.photcode = 0;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if (!(criteria.photcode = GetPhotcodeCodebyName (argv[N]))) {
      fprintf (stderr, "ERROR: photcode not found in photcode table\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    criteria.PhotcodeSelect = TRUE;
  }

  /* string in image name */
  criteria.NameSelect = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    criteria.Name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.NameSelect = TRUE;
  }

  /* string in image name */
  criteria.CodeSelect = FALSE;
  criteria.Code = 0;
  if ((N = get_argument (argc, argv, "-code"))) {
    criteria.CodeSelect = TRUE;
    remove_argument (N, &argc, argv);
    criteria.Code = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  options.table = (char *) NULL;
  if ((N = get_argument (argc, argv, "-fits"))) {
    remove_argument (N, &argc, argv);
    options.table = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  options.bintable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-binfits"))) {
    remove_argument (N, &argc, argv);
    options.bintable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* desired action */
  options.modify = FALSE;
  options.ModifyValue = options.ModifyEntry = NULL;
  if ((N = get_argument (argc, argv, "-flag"))) {
    remove_argument (N, &argc, argv);
    options.ModifyEntry = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    options.ModifyValue = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    options.modify = TRUE;
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  FORCE_READ = FALSE;
  if ((N = get_argument (argc, argv, "-force"))) {
    remove_argument (N, &argc, argv);
    FORCE_READ = TRUE;
  }

  if (argc != 1) {
    fprintf (stderr, "USAGE: imphotsearch [config ops] [-trange start stop/delta] [-photcode code] [-fits out.fits]\n");
    exit (1);
  }
  return (TRUE);
}
