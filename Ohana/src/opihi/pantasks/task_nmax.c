# include "pantasks.h"

int task_nmax (int argc, char **argv) {

  Task *task;

  if (argc != 2) goto usage;

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  task[0].Nmax = atoi (argv[1]);
  JobTaskUnlock();
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: nmax N\n");
  return (FALSE);
}

int task_npending (int argc, char **argv) {

  Task *task;

  if (argc != 2) goto usage;

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  task[0].NpendingMax = atoi (argv[1]);
  JobTaskUnlock();
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: npending N\n");
  return (FALSE);
}
