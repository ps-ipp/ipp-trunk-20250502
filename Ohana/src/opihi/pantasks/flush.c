# include "pantasks.h"

int flush_jobs (int argc, char **argv) {

  if (argc != 2) goto usage;

  if (!strcasecmp (argv[1], "jobs")) {
    JobTaskLock();
    FlushJobs ();
    JobTaskUnlock();
    return (TRUE);
  }
  
usage:
  gprint (GP_ERR, "USAGE: flush jobs\n");
  return (FALSE);
}

/* find job from jobID, kill and delete */
