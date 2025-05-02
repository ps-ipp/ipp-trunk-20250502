# include "pantasks.h"

int task_active (int argc, char **argv) {

  Task *task;

  if (argc != 2) goto usage;

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "true")) {
    task[0].active = TRUE;
    JobTaskUnlock();
    return (TRUE);
  }
  if (!strcasecmp (argv[1], "false")) {
    task[0].active = FALSE;
    JobTaskUnlock();
    return (TRUE);
  }

  gprint (GP_ERR, "ERROR: invalid option: %s\n", argv[1]);
  JobTaskUnlock();
  return (FALSE);

usage:
  gprint (GP_ERR, "USAGE: active (true|false)\n");
  return (FALSE);
}
