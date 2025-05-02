# include "data.h"

int queuesubstr (int argc, char **argv) {
  
  int i;
  char *p, *q;
  char *match, *replace;
  Queue *queue;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: queueprint (queue) (match) (replace)\n");
    return (FALSE);
  }

  match = argv[2];
  replace = argv[3];

  assert (match);
  assert (replace);

  queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "ERROR: queue %s not found\n", argv[1]);
    return (FALSE);
  }

  for (i = 0; i < queue[0].Nlines; i++) {

    p = queue[0].lines[i];
    if (p == NULL) continue;
    
    while (strstr (p, match) != NULL) {
      q = strsubs (p, match, replace);
      free (p);
      p = q;
    }
    queue[0].lines[i] = p;
  }

  return (TRUE);
}

