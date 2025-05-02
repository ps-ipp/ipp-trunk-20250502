# include "data.h"

int queuesave (int argc, char **argv) {
  
  Queue *queue;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: queuesave (name) (file)\n");
    return (FALSE);
  }

  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  SaveQueue (queue, argv[2]);
  return (TRUE);
}

