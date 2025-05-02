# include "data.h"

int queuelist (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);
  
  if (argc != 1) {
    gprint (GP_ERR, "USAGE: queuelist\n");
    return (FALSE);
  }

  ListQueues ();
  return (TRUE);
}
