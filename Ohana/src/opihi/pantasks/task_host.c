# include "pantasks.h"

int task_host (int argc, char **argv) {

  int N, RequiredHost;
  Task *task;

  RequiredHost = FALSE;
  if ((N = get_argument (argc, argv, "-required"))) {
    remove_argument (N, &argc, argv);
    RequiredHost = TRUE;
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: host <name> [-required]\n");
    gprint (GP_ERR, "  define host machine for this task\n");
    gprint (GP_ERR, "  -required flags indicates controller must use this host\n");
    gprint (GP_ERR, "  value of 'local' for host indicates process not using controller\n");
    gprint (GP_ERR, "  value of 'anyhost' for host indicates controller may assign at will\n");
    return (FALSE);
  }

  task = GetNewTask ();
  JobTaskLock();
  if (task == NULL) {
    task = GetActiveTask ();
    if (task == NULL) {
      gprint (GP_ERR, "ERROR: not defining or running a task\n");
      JobTaskUnlock();
      return (FALSE);
    }
  }
  task[0].host_required = RequiredHost;

  if (task[0].host != NULL) free (task[0].host);
  task[0].host = NULL;

  if (!strcasecmp (argv[1], "LOCAL")) {
    JobTaskUnlock();
    return (TRUE);
  }

  task[0].host = strcreate (argv[1]);
  JobTaskUnlock();
  return (TRUE);
}

/* apparently, local is the default! */
