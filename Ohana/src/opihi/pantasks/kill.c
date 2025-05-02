# include "pantasks.h"

int kill_job (int argc, char **argv) {

  Job *job;
  IDtype JobID;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: kill (JobID)\n");
    return (FALSE);
  }
  JobID = atoll (argv[1]);

  JobTaskLock();
  job = FindJob (JobID);
  if (job == NULL) {
    gprint (GP_LOG, "job not found\n");
    JobTaskUnlock();
    return (TRUE);
  }

  if (job[0].mode == JOB_LOCAL) {
    if (!KillLocalJob (job)) {
      job[0].state = JOB_HUNG;
      if (VerboseMode()) gprint (GP_LOG, "child process %d is hung, cannot kill\n", job[0].pid);
      JobTaskUnlock();
      return (FALSE);
    }
  } else {
    if (!KillControllerJob (job)) {
      job[0].state = JOB_HUNG;
      if (VerboseMode()) gprint (GP_LOG, "child process %d is hung, cannot kill\n", job[0].pid);
      JobTaskUnlock();
      return (FALSE);
    }
  }    
  DeleteJob (job);
  gprint (GP_LOG, "job removed\n");
  JobTaskUnlock();
  return (TRUE);
}
