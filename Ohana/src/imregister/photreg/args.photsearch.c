# include "imregister.h"
# include "photreg.h"

int args (int argc, char **argv) {

  int N, equiv;

  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();

  criteria.Ntimes = 0;
  if (!get_trange_arguments (&argc, argv, &criteria.tstart, &criteria.tstop, &criteria.Ntimes)) {
    fprintf (stderr, "ERROR: syntax error\n");
    exit (1);
  }

  criteria.PhotCodeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if ((criteria.photcode = GetPhotcodeCodebyName (argv[N])) == 0) {
      fprintf (stderr, "ERROR: photcode not found in table\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    criteria.PhotCodeSelect = TRUE;
  }

  criteria.LabelSelect = FALSE;
  if ((N = get_argument (argc, argv, "-label"))) {
    remove_argument (N, &argc, argv);
    criteria.Label = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.LabelSelect = TRUE;
  }

  output.delete = FALSE;
  if ((N = get_argument (argc, argv, "-delete"))) {
    remove_argument (N, &argc, argv);
    output.delete = TRUE;
  }

  output.equiv = FALSE;
  if ((N = get_argument (argc, argv, "-equiv"))) {
    remove_argument (N, &argc, argv);
    output.equiv = TRUE;
  }

  output.offset = FALSE;
  if ((N = get_argument (argc, argv, "-offset"))) {
    remove_argument (N, &argc, argv);
    output.offset = TRUE;
  }

  output.table = (char *) NULL;
  if ((N = get_argument (argc, argv, "-table"))) {
    remove_argument (N, &argc, argv);
    output.table = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  output.bintable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-bintable"))) {
    remove_argument (N, &argc, argv);
    output.bintable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  output.verbose = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    output.verbose = TRUE;
  }

  output.photcodenames = TRUE;
  if ((N = get_argument (argc, argv, "-photcodes"))) {
    remove_argument (N, &argc, argv);
    output.photcodenames = FALSE;
  }

  output.convert = FALSE;
  if ((N = get_argument (argc, argv, "-convert"))) {
    remove_argument (N, &argc, argv);
    output.convert = TRUE;
    if (output.delete || output.modify) {
      fprintf (stderr, "can't change old format table\n");
      exit (1);
    }
  }

  if ((N = get_argument (argc, argv, "-image"))) {
    remove_argument (N, &argc, argv);
    if (argc < N + 3) {
      fprintf (stderr, "missing arguments to -image\n");
      exit (1);
    }
    getImageData (argv[N], argv[N+1], argv[N+2]);
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);
  }

  output.db = strcreate ("phot");
  if ((N = get_argument (argc, argv, "-trans"))) {
    remove_argument (N, &argc, argv);
    output.db = strcreate ("trans");
  }

  if (argc != 1) {
    fprintf (stderr, "USAGE: photsearch [config ops] [-v] [-version]\n");
    fprintf (stderr, "       [-trange start stop/range] [-photcode code]\n");
    fprintf (stderr, "       [-label label] [-delete]\n");
    fprintf (stderr, "       [-table table.fits] [-bintable bintable.fits]\n");
    exit (1);
  }
 
  /* set up photcode information */
  if (criteria.PhotCodeSelect && output.equiv) {
    if (!(equiv = GetPhotcodeEquivCodebyCode (criteria.photcode))) {
      fprintf (stderr, "ERROR: photcode not found in photcode table\n");
      exit (1);
    }
    criteria.photcode = equiv;
  }
  return (TRUE);
}
