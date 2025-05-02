# include "data.h"

int set (int argc, char **argv) {
  
  int size;
  char *out;

  /** check basic form for line **/
  if ((argc < 3) || strcmp(argv[2], "=")) {
    gprint (GP_ERR, "%s = (matrix expression)\n", argv[0]);
    return (FALSE);
  }

  out = dvomath (argc - 3, &argv[3], &size, -1);
  if (out == NULL) {
    print_error ();
    return (FALSE);
  }
  
  switch (size) {
    case 0:
      set_str_variable (argv[1], out);
      free (out);
      break;

    case 1:
      if (!MoveNamedVector (argv[1], out)) {
	DeleteNamedVector (out);
	free (out);
	gprint (GP_ERR, "invalid output vector name\n");
	return (FALSE);
      }
      free (out);
      break;
  
    case 2:
      if (!MoveNamedBuffer (argv[1], out)) {
	DeleteNamedBuffer (out);
	free (out);
	gprint (GP_ERR, "invalid output matrix name\n");
	return (FALSE);
      }
      free (out);
      break;
  }
  return (TRUE);
}
