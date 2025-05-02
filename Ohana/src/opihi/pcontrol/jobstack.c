# include "pcontrol.h"

int jobstack (int argc, char **argv) {

  int i;
  Stack *stack;
  Job *job;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: jobstack (jobstack)\n");
    gprint (GP_ERR, "       (jobstack) : pending, busy, exit, crash, hung, done\n");
    return (FALSE);
  }

  /* select jobstack */
  stack = GetJobStackByName (argv[1]);
  if (stack == NULL) {
    gprint (GP_ERR, "jobstack not found\n");
    return (FALSE);
  }

  /* print list */
  LockStack (stack);
  gprint (GP_LOG, "BEGIN BLOCK Njobs: %d\n", stack[0].Nobject);
  for (i = 0; i < stack[0].Nobject; i++) {
    job = stack[0].object[i];
    /* PrintID (GP_LOG, job[0].JobID); */
    gprint (GP_LOG, "%lld ", job[0].JobID);
    if (job[0].realhost) {
	gprint (GP_LOG, "%s   %s\n", job[0].argv[0], job[0].realhost);
    } else {
	gprint (GP_LOG, "%s  (%s)\n", job[0].argv[0], job[0].hostname);
    }
  }
  UnlockStack (stack);

  return (TRUE);
}

// Safe with PTHREAD_MUTEX_INITIALIZER lock
