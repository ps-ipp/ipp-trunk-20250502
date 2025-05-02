# include "data.h"

int queuepop (int argc, char **argv) {
  
  int N;
  char *Key;
  char *var;
  char *line;
  char *Value;
  Queue *queue;

  var = (char *) NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    var = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Key = NULL;
  Value = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    Key = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Value = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: queuepop (queue) [-var variable] [-key N value]\n");
    return (FALSE);
  }

  /* will create a queue if none exists */
  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  if (Key == NULL) {
    line = PopQueue (queue);
  } else {
    line = PopQueueMatch (queue, Key, Value);
  }

  if (var == NULL) {
    if (line == NULL) {
      gprint (GP_ERR, "queue %s is empty or match not found\n", argv[1]);
      return (FALSE);
    } else {
      gprint (GP_LOG, "%s\n", line);
      free (line);
      return (TRUE);
    }
  }

  if (line == NULL) {
    set_str_variable (var, "NULL");
  } else {
    set_str_variable (var, line);
    free (line);
  }

  free (var);
  if (Key != NULL) free (Key);
  if (Value != NULL) free (Value);

  return (TRUE);
}

