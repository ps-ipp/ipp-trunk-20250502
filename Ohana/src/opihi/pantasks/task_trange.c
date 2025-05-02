# include "pantasks.h"

int task_trange (int argc, char **argv) {

  int N;
  Task *task;
  TimeRange range;

  /* reset the tranges for the current task */
  if ((N = get_argument (argc, argv, "-reset"))) {
    remove_argument (N, &argc, argv);
    if (argc != 1) goto usage;

    JobTaskLock();
    task = GetNewTask ();
    if (task == NULL) {
      gprint (GP_ERR, "ERROR: not defining or running a task\n");
      JobTaskUnlock();
      return (FALSE);
    }

    task[0].Nranges = 0;
    REALLOCATE (task[0].ranges, TimeRange, 1);
    JobTaskUnlock();
    return (TRUE);
  } 

  range.Nmax = 0;
  range.Nrun = 0;
  if ((N = get_argument (argc, argv, "-nmax"))) {
    remove_argument (N, &argc, argv);
    range.Nmax = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* keep = true means the time range defines a valid time range
     keep = false means the time range defines an invalid range */
  range.include = TRUE;
  if ((N = get_argument (argc, argv, "-exclude"))) {
    remove_argument (N, &argc, argv);
    range.include = FALSE;
  }

  if (argc != 3) goto usage;

  /* test for Mon[@HH:MM:SS] - both must match */
  if (day_to_sec (argv[1], &range.start)) {
    if (!day_to_sec (argv[2], &range.stop)) {
      gprint (GP_ERR, "invalid day/time %s\n", argv[2]);
      goto usage;
    }
    range.type = RANGE_WEEK;
    goto valid;
  }

  /* allow a syntax which means something like "every hour at MM:SS"
     this could by something like *:MM:SS ? */

  /* test for HH:MM:SS */
  if (hms_to_sec (argv[1], &range.start)) {
    if (!hms_to_sec (argv[2], &range.stop)) {
      gprint (GP_ERR, "invalid time %s\n", argv[2]);
      goto usage;
    }
    range.type = RANGE_DAY;
    goto valid;
  }

  /* this test does not fail sufficiently robustly for invalid inputs */
  /* test for YYYY/MM/DD - both must match */
  if (ohana_str_to_time (argv[1], &range.start)) {
    if (!ohana_str_to_time (argv[2], &range.stop)) {
      gprint (GP_ERR, "invalid date/time %s\n", argv[2]);
      goto usage;
    }
    range.type = RANGE_ABS;
    goto valid;
  }
  goto usage;

valid:
  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  N = task[0].Nranges;
  task[0].Nranges ++;
  REALLOCATE (task[0].ranges, TimeRange, task[0].Nranges);
  
  task[0].ranges[N] = range;
  JobTaskUnlock();
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: trange start end [-nmax N]\n");
  gprint (GP_ERR, "USAGE: trange -reset\n");
  return (FALSE);
}
