# include "pantasks.h"

int controller_output (int argc, char **argv) {

  if ((argc != 1) || ((argc == 2) && (strcmp(argv[1], "flush")))) {
    gprint (GP_ERR, "USAGE: controller output\n");
    return (FALSE);
  }

  CheckControllerOutput ();
  PrintControllerOutput ();

  if (argc == 2) {
    FlushControllerOutput ();
  }

  return (TRUE);
}
