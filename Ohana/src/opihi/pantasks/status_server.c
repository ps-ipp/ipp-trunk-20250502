# include "pantasks.h"
// this is the server version of the status_sys function (status.c)
// the server verion needs to include locks on the jobs.

int status_scheduler(void);

int status_server (int argc, char **argv) {

  int N;
  int checkController,showJobs, showDetails;

  if (get_argument (argc, argv, "-h")) goto help;
  if (get_argument (argc, argv, "-help")) goto help;
  if (get_argument (argc, argv, "--help")) goto help;

  // special options for task timing stats
  if ((N = get_argument (argc, argv, "-taskstats"))) {
    remove_argument (N, &argc, argv);
    status_scheduler();
    JobTaskLock();
    if (argc == 2) {
      ListTaskStats (argv[N]);
    } else {
      ListTaskStats (NULL);
    }      
    JobTaskUnlock();
    return (TRUE);
  }

  if ((N = get_argument (argc, argv, "-taskstatsreset"))) {
    remove_argument (N, &argc, argv);
    JobTaskLock();
    if (argc == 2) {
      ResetTaskStats (argv[N]);
    } else {
      ResetTaskStats (NULL);
    }      
    JobTaskUnlock();
    return (TRUE);
  }

  checkController = FALSE;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    checkController = TRUE;
  }
  if ((N = get_argument (argc, argv, "-control"))) {
    remove_argument (N, &argc, argv);
    checkController = TRUE;
  }
  if ((N = get_argument (argc, argv, "-controller"))) {
    remove_argument (N, &argc, argv);
    checkController = TRUE;
  }

  showJobs = FALSE;
  if ((N = get_argument (argc, argv, "-jobs"))) {
    remove_argument (N, &argc, argv);
    showJobs = TRUE;
  }

  showDetails = FALSE;
  if ((N = get_argument (argc, argv, "-info"))) {
    remove_argument (N, &argc, argv);
    showDetails = TRUE;
  }
  if ((N = get_argument (argc, argv, "-taskinfo"))) {
    remove_argument (N, &argc, argv);
    showDetails = TRUE;
  }
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    showDetails = TRUE;
  }
  if ((N = get_argument (argc, argv, "-details"))) {
    remove_argument (N, &argc, argv);
    showDetails = TRUE;
  }
  if ((N = get_argument (argc, argv, "-verbose"))) {
    remove_argument (N, &argc, argv);
    showDetails = TRUE;
  }

  status_scheduler();

  JobTaskLock();
  ListTasks (showDetails);
  if (showJobs) {
    ListJobs();
  }
  JobTaskUnlock();

  if (checkController) {
    ControlLock(__func__);
    PrintControllerBusyJobs();
    ControlUnlock(__func__);
  }

  gprint (GP_LOG, "\n");
  return (TRUE);

help:
  gprint (GP_LOG, "USAGE: status [options]\n");
  gprint (GP_LOG, "       (without options: show system status)\n");
  gprint (GP_LOG, "       -jobs : list active jobs\n");
  gprint (GP_LOG, "       -c | -control | -controller      : add status of jobs on controller\n");
  gprint (GP_LOG, "       -v | -verbose | -details | -info : add details for tasks\n");
  gprint (GP_LOG, "       -taskstats                       : show timing statistics for tasks\n");
  gprint (GP_LOG, "       -taskstatsreset                  : reset timing statistics for tasks\n");
  return (FALSE);
}

int status_scheduler(void) {
  
  gprint (GP_LOG, "\n");
  if (CheckTasksGetState()) {
    gprint (GP_LOG, " Scheduler is running\n");
  } else {
    if (CheckJobsGetState()) {
      gprint (GP_LOG, " Scheduler is stopped, harvesting jobs\n");
    } else {
      gprint (GP_LOG, " Scheduler is stopped\n");
    }
  }
  if (CheckControllerStatus()) {
    gprint (GP_LOG, " Controller is running\n");
  } else {
    gprint (GP_LOG, " Controller is stopped\n");
  }
  return (TRUE);
}
