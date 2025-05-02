# include "basic.h"

int local (int argc, char **argv) {

  int i, N, STATIC;
  char *p;

  /* create a variable named MacroDepth.argv[1] */

  STATIC = FALSE;
  if ((N = get_argument (argc, argv, "-static"))) {
    remove_argument (N, &argc, argv);
    STATIC = TRUE;
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: local (variable)\n");
    return (FALSE);
  }
  
  for (i = 1; i < argc; i++) {
    if (STATIC) {
      p = get_local_variable_ptr (argv[i]);
      if (p == NULL) {
	set_local_variable (argv[i], "NULL");
      }
    } else {
      set_local_variable (argv[i], "NULL");
    }      
  }
  return (TRUE);

}
