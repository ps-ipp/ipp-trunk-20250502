# include "data.h"

int queueinit (int argc, char **argv) {

  Queue *queue;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: queueinit (name)\n");
    return (FALSE);
  }

  queue = CreateQueue (argv[1]);
  InitQueue (queue);
  return (TRUE);
}
