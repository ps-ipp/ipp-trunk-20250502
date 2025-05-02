# include "data.h"

int opihi_type (int argc, char **argv) {
  
  int N;

  int SCALAR = FALSE;
  if ((N = get_argument (argc, argv, "-scalar"))) {
    SCALAR = TRUE;
    remove_argument (N, &argc, argv);
  }

  char *varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) goto usage;

  char result[16]; memset (result, 0, 16);

  if (SCALAR) {
    if (IsScalar (argv[1])) {
      strcpy (result, "found");
    } else {
      strcpy (result, "none");
    }
    goto got_result;
  }

  Vector *vec = NULL;
  Buffer *buf = NULL;
  if ((vec = SelectVector (argv[1], OLDVECTOR, FALSE)) != NULL) {
    if (vec->type == OPIHI_FLT) { strcpy (result, "float");  goto got_result; }
    if (vec->type == OPIHI_INT) { strcpy (result, "int");    goto got_result; }
    if (vec->type == OPIHI_STR) { strcpy (result, "string"); goto got_result; }
    myAbort ("impossible");
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, FALSE)) != NULL) { strcpy (result, "matrix"); goto got_result; }
  strcpy (result, "none");

got_result:
  if (varName) {
    set_str_variable (varName, result);
  } else {
    gprint (GP_LOG, "%s\n", result);
  }
  return TRUE;

usage:
  gprint (GP_ERR, "SYNTAX: type (vector/buffer) [-var value] [-scalar]\n");
  gprint (GP_ERR, "  returns 'none', 'float', 'int' for vector types, 'matrix' for buffer / image / matrix\n");
  gprint (GP_ERR, "  returns 'none', 'found' for scalar types if -scalar is selected\n");
  return (FALSE);
}
