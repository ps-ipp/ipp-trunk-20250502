# include "data.h"

int queuepush (int argc, char **argv) {
  
  char *Key;
  int N, Unique, Replace;
  Queue *queue;

  Unique = FALSE;
  if ((N = get_argument (argc, argv, "-uniq"))) {
    remove_argument (N, &argc, argv);
    Unique = TRUE;
  }

  Replace = FALSE;
  if ((N = get_argument (argc, argv, "-replace"))) {
    remove_argument (N, &argc, argv);
    Replace = TRUE;
  }

  Key = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    Key = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: queuepush (queue) (value) [-key N] [-uniq] [-replace]\n");
    return (FALSE);
  }

  /* will create a queue if none exists */
  queue = CreateQueue (argv[1]);

  if (Unique) {
    PushQueueUnique (queue, argv[2], Key);
  }
  if (Replace) {
    PushQueueReplace (queue, argv[2], Key);
  }
  if (!Unique && !Replace) {
    PushQueue (queue, argv[2]);
  }

  if (Key != NULL) free (Key);
  return (TRUE);
}


/* 
 * -key only needed for replace or unique : give an error otherwise
 * -uniq searched for a match and does NOT replace if matched
 * -replace searches for a match and replaces if matched
 * should trigger an error if -uniq and -replace...
 */
