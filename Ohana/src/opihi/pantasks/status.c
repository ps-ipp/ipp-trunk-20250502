# include "pantasks.h"

int status_sys (int argc, char **argv) {

  int N;

  if (get_argument (argc, argv, "-h")) goto help;
  if (get_argument (argc, argv, "-help")) goto help;
  if (get_argument (argc, argv, "--help")) goto help;

  if ((N = get_argument (argc, argv, "-tasks"))) {
    remove_argument (N, &argc, argv);
    ListTasks (FALSE);
    return (TRUE);
  }

  if ((N = get_argument (argc, argv, "-taskinfo")) ||
      (N = get_argument (argc, argv, "-info")) ||
      (N = get_argument (argc, argv, "-v")) ||
      (N = get_argument (argc, argv, "-verbose")) ||
      (N = get_argument (argc, argv, "-details"))) {
    remove_argument (N, &argc, argv);
    ListTasks (TRUE);
    return (TRUE);
  }

  if ((N = get_argument (argc, argv, "-taskstats"))) {
    remove_argument (N, &argc, argv);
    if (argc == 2) {
      ListTaskStats (argv[N]);
    } else {
      ListTaskStats (NULL);
    }      
    return (TRUE);
  }

  if ((N = get_argument (argc, argv, "-taskstatsreset"))) {
    remove_argument (N, &argc, argv);
    if (argc == 2) {
      ResetTaskStats (argv[N]);
    } else {
      ResetTaskStats (NULL);
    }      
    return (TRUE);
  }

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
  ListTasks (FALSE);
  ListJobs ();
  return (TRUE);

help:
  gprint (GP_LOG, "USAGE: status [options]\n");
  gprint (GP_LOG, "       (without options: over system status)\n");
  gprint (GP_LOG, "       -tasks     : list defined tasks\n");
  gprint (GP_LOG, "       -taskinfo  : details for tasks\n");
  gprint (GP_LOG, "       -taskstats : processing statistics for tasks\n");
  gprint (GP_LOG, "       -taskstatsreset : reset processing statistics for tasks\n");
  return (FALSE);
}
