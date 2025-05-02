# include "pantasks.h"

int task_stdout (int argc, char **argv) {

  Task *task;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: stdout <name>\n");
    gprint (GP_ERR, "  define dump file for job stdout for this task\n");
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
  if (task[0].stdout_dump != NULL) free (task[0].stdout_dump);
  task[0].stdout_dump = strcreate (argv[1]);
  JobTaskUnlock();
  return (TRUE);
}

int task_stderr (int argc, char **argv) {

  Task *task;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: stderr <name>\n");
    gprint (GP_ERR, "  define dump file for job stderr for this task\n");
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
  if (task[0].stderr_dump != NULL) free (task[0].stderr_dump);
  task[0].stderr_dump = strcreate (argv[1]);
  JobTaskUnlock();
  return (TRUE);
}

/* apparently, local is the default! */
