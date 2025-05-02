# include "pantasks.h"

int task_nice (int argc, char **argv) {

  char *endptr = NULL;
  int nicelevel;
  Task *task;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: nice (nicelevel)\n");
    return (FALSE);
  }

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  nicelevel = strtol (argv[1], &endptr, 10);
  if (*endptr) goto fail;
  if (nicelevel < 0) goto fail;
  if (nicelevel > 20) goto fail;

  task[0].nicelevel = nicelevel;
  JobTaskUnlock();
  return (TRUE);

fail:
    gprint (GP_ERR, "ERROR: nice (nicelevel) -- nicelevel must be an integer 0 to 20\n");
    return (FALSE);
}
