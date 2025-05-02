# include "basic.h"

int dirname_opihi (int argc, char **argv) {

  int N;
  char *dirName, *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: dirname (path) [-var name]\n");
    return (FALSE);
  }

  dirName = pathname (argv[1]);

  if (varName == NULL) {
    gprint (GP_LOG, "%s\n", dirName);
  } else {
    set_str_variable (varName, dirName);
    free (varName);
  }    

  free (dirName);
  return (TRUE);
}

// XXX need to add mode option
// XXX need to respect umask (need umask command?)
