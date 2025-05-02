# include "data.h"

int list_vectors (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  ListVectors ();
  return (TRUE);
}

int vtype (int argc, char **argv) {

  int N;

  char *Variable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    Variable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: vtype (vector) [-var out]\n");
    return (FALSE);
  }

  Vector *vec = SelectVector (argv[1], OLDVECTOR, FALSE);
  if (!vec) {
    gprint (GP_ERR, "unknown vector %s\n", argv[1]);
    free (Variable);
    return FALSE;
  }

  switch (vec->type) {
    case OPIHI_FLT:
      if (Variable) {
	set_str_variable (Variable, "FLT");
      } else {
	gprint (GP_LOG, "%s : FLT\n", argv[1]);
      }
      break;
    case OPIHI_INT:
      if (Variable) {
	set_str_variable (Variable, "INT");
      } else {
	gprint (GP_LOG, "%s : INT\n", argv[1]);
      }
      break;
    case OPIHI_STR:
      if (Variable) {
	set_str_variable (Variable, "STR");
      } else {
	gprint (GP_LOG, "%s : STR\n", argv[1]);
      }
      break;
  }
  if (Variable) free (Variable);

  return (TRUE);
}

