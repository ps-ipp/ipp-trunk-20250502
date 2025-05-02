# include "pantasks.h"

int task_command (int argc, char **argv) {

  int i;
  Task *task;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: command <command> <arg>. ..\n");
    gprint (GP_ERR, "  (define command for this task)\n");
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

  /* free existing memory used by argv */
  if (task[0].argc != 0) {
    for (i = 0; i < task[0].argc; i++) {
      if (task[0].argv[i] != NULL) free (task[0].argv[i]);
    }
    free (task[0].argv);
  }

  /* create new memory for argv */
  task[0].argc = argc - 1;
  ALLOCATE (task[0].argv, char *, MAX (task[0].argc, 1));
  for (i = 0; i < task[0].argc; i++) {
    task[0].argv[i] = strcreate (argv[i+1]);
  }
  JobTaskUnlock();
  return (TRUE);
}
