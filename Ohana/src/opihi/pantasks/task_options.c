# include "pantasks.h"

int task_options (int argc, char **argv) {

  int i;
  Task *task;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: options <opt>. ..\n");
    gprint (GP_ERR, "  (define options for this task)\n");
    return (FALSE);
  }

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    task = GetActiveTask ();
    if (task == NULL) {
      gprint (GP_ERR, "ERROR: not defining or running a task\n");
      JobTaskUnlock();
      return (FALSE);
    }
  }

  /* free existing memory used by optv */
  if (task[0].optc != 0) {
    for (i = 0; i < task[0].optc; i++) {
      if (task[0].optv[i] != NULL) free (task[0].optv[i]);
    }
    free (task[0].optv);
  }

  /* create new memory for optv */
  task[0].optc = argc - 1;
  ALLOCATE (task[0].optv, char *, MAX (task[0].optc, 1));
  for (i = 0; i < task[0].optc; i++) {
    task[0].optv[i] = strcreate (argv[i+1]);
  }
  JobTaskUnlock();
  return (TRUE);
}
