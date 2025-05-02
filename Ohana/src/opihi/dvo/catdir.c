# include "dvoshell.h"

int catdir_define (int argc, char **argv) {
  
  char *current;
  int status, N, VERBOSE;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: catdir (name)\n");
    gprint (GP_ERR, "       (name) may be a path or 'default'\n");
    current = GetCATDIR ();
    if (current == NULL) {
      gprint (GP_ERR, "catdir not defined\n");
    } else {
      gprint (GP_ERR, "current: %s\n", current);
    }
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "default")) {
    status = SetCATDIR (NULL, VERBOSE);
  } else {
    status = SetCATDIR (argv[1], VERBOSE);
  }

  if (!status) {
    gprint (GP_ERR, "invalid / undefined CATDIR\n");
    return (FALSE);
  }

  return (TRUE);
}
