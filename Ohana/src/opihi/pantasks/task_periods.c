# include "pantasks.h"

int task_periods (int argc, char **argv) {

  int N, Poll, Exec, Timeout;
  float PollValue, ExecValue, TimeoutValue;
  Task *task;

  PollValue = 0;
  Poll = FALSE;
  if ((N = get_argument (argc, argv, "-poll"))) {
    Poll = TRUE;
    remove_argument (N, &argc, argv);
    PollValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  ExecValue = 0;
  Exec = FALSE;
  if ((N = get_argument (argc, argv, "-exec"))) {
    Exec = TRUE;
    remove_argument (N, &argc, argv);
    ExecValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  TimeoutValue = 0;
  Timeout = FALSE;
  if ((N = get_argument (argc, argv, "-timeout"))) {
    Timeout = TRUE;
    remove_argument (N, &argc, argv);
    TimeoutValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: periods [-poll value] [-exec value] [-timeout value]\n");
    gprint (GP_ERR, "  define time periods for this task\n");
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

  if (Poll) task[0].poll_period = PollValue;
  if (Exec) task[0].exec_period = ExecValue;
  if (Timeout) task[0].timeout_period = TimeoutValue;

  JobTaskUnlock();
  return (TRUE);
}
