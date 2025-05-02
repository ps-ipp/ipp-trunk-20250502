# include "data.h"

int queuesize (int argc, char **argv) {
  
  int N;
  char *var;
  Queue *queue;

  var = (char *) NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    var = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: queuesize (name) [-var variable]\n");
    return (FALSE);
  }

  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  if (var == (char *) NULL) {
    gprint (GP_ERR, "Nlines: %d\n", queue[0].Nlines);
    return (TRUE);
  }

  set_int_variable (var, queue[0].Nlines);
  free (var);
  return (TRUE);
}

