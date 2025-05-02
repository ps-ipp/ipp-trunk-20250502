# include "data.h"

int queuedelete (int argc, char **argv) {
  
  Queue *queue;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: queuedelete (name)\n");
    return (FALSE);
  }

  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  DeleteQueue (queue);
  return (TRUE);
}

