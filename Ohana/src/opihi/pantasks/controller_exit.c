# include "pantasks.h"

int controller_exit (int argc, char **argv) {

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: controller exit TRUE\n");
    return (FALSE);
  }

  if (strcasecmp (argv[1], "TRUE")) return (FALSE);

  QuitController ();
  return (TRUE);
}
