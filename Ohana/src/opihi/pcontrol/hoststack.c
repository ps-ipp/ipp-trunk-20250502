# include "pcontrol.h"

int hoststack (int argc, char **argv) {

  int i;
  Stack *stack;
  Host *host;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: hoststack (hoststack)\n");
    gprint (GP_ERR, "       (hoststack) : idle, busy, done, down, off\n");
    return (FALSE);
  }

  /* select hoststack */
  stack = GetHostStackByName (argv[1]);
  if (stack == NULL) {
    gprint (GP_ERR, "hoststack not found\n");
    return (FALSE);
  }

  /* print list */
  LockStack (stack);
  gprint (GP_LOG, "Nhosts: %d\n", stack[0].Nobject);
  for (i = 0; i < stack[0].Nobject; i++) {
    host = stack[0].object[i];
    gprint (GP_LOG, "%lld %s\n", host[0].HostID, host[0].hostname);
  }
  UnlockStack (stack);

  return (TRUE);
}

// Safe with PTHREAD_MUTEX_INITIALIZER lock
