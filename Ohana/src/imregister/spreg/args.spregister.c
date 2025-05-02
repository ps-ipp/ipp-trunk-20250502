# include "imregister.h"
# include "spreg.h"

int args (int argc, char **argv) {

  int N;

  ConfigInitSpec (&argc, argv); /* load elixir config data */

  NoReg = FALSE;
  if ((N = get_argument (argc, argv, "-noreg"))) {
    remove_argument (N, &argc, argv);
    NoReg = TRUE;
  }

  DUMP = FALSE;
  if ((N = get_argument (argc, argv, "-dump"))) {
    remove_argument (N, &argc, argv);
    DUMP = TRUE;
  }

  output.verbose = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    output.verbose = TRUE;
  }

  NeedType = FALSE;
  if ((N = get_argument (argc, argv, "-needtype"))) {
    remove_argument (N, &argc, argv);
    NeedType = TRUE;
  }

  /* all imregister programs are implicitly modifying the db */
  output.modify = TRUE;
  IMSORT = FALSE;

  if (argc != 2) {
    fprintf (stderr, "ERROR: Usage: spregister (filename) [-noreg]\n");
    exit (1);
  }
  return (TRUE);
}
