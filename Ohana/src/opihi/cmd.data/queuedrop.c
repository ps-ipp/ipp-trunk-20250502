# include "data.h"

int queuedrop (int argc, char **argv) {
  
  int N;
  char *Key = NULL;
  char *line = NULL;
  Queue *queue = NULL;
  char *Value = NULL;

  Key = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    Key = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Value = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((argc != 2) || (Key == NULL)) {
    gprint (GP_ERR, "USAGE: queuedrop (queue) [-key N value]\n");
    return (FALSE);
  }

  /* will create a queue if none exists */
  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  /* drop all matching entries, if any exist */
  while ((line = PopQueueMatch (queue, Key, Value)) != NULL) {
    free (line);
  }
  return (TRUE);
}

