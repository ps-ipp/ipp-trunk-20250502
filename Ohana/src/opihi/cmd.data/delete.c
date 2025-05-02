# include "data.h"

int delete (int argc, char **argv) {
  
  int i, N, Quiet;

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: delete <obiect> [<object> ..]\n");
    return (FALSE);
  }

  for (i = 1; i < argc; i++) {
    if (DeleteNamedBuffer (argv[i])) continue; 
    if (DeleteNamedVector (argv[i])) continue;
    if (DeleteNamedScalar (argv[i])) continue; 
    if (!Quiet) gprint (GP_ERR, "can't find object %s\n", argv[i]);
  }

  return (TRUE);
}

