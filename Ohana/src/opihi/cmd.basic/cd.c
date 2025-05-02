# include "basic.h"

int cd (int argc, char **argv) {

  int N, VERBOSE, status;
  char *cwd;

  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: cd <path>\n");
    return (FALSE);
  }

  status = chdir (argv[1]);
  if (!status) {
    if ((cwd = getcwd (NULL, 1024)) == NULL) {
      gprint (GP_ERR, "error getting cwd\n");
      return (FALSE);
    }
    if (VERBOSE) gprint (GP_LOG, "cwd: %s\n", cwd);
    real_free (cwd);
    return (TRUE);
  }

  gprint (GP_ERR, "error changing to %s\n", argv[1]);
  return (FALSE);

}

int pwd (int argc, char **argv) {

  int N;
  char *cwd, *var;

  var = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    var = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: pwd [-var variable]\n");
    return (FALSE);
  }
  
  if ((cwd = getcwd(NULL, 1024)) == NULL) {
    gprint (GP_ERR, "error getting cwd\n");
    if (var != NULL) free (var);
    return (FALSE);
  }
  if (var == NULL) {
      gprint (GP_LOG, "cwd: %s\n", cwd);
  } else {
      set_str_variable (var, cwd);
      free (var);
  }
  real_free (cwd);
  return (TRUE);
  
}
