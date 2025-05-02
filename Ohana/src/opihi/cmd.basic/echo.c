# include "basic.h"

int echo (int argc, char **argv) {
  
  int i, N, RETURN_CHAR;

  RETURN_CHAR = TRUE;
  if ((N = get_argument (argc, argv, "-no-return"))) {
    remove_argument (N, &argc, argv);
    RETURN_CHAR = FALSE;
  }

  for (i = 1; i < argc - 1; i++) {
    gprint (GP_LOG, "%s ", argv[i]);
  }
  if (argc >= 2) {
    if (RETURN_CHAR) {
      gprint (GP_LOG, "%s\n", argv[argc - 1]);
    } else {
      gprint (GP_LOG, "%s", argv[argc - 1]);
    }
  }
  return (TRUE);
}
