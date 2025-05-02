# include "pantasks.h"

int delete_job (int argc, char **argv) {

  Job *job;
  IDtype JobID;

  if (argc != 3) goto usage;

  if (!strcasecmp (argv[1], "job")) {
    JobID = atoi (argv[2]);

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

  if (!strcasecmp (argv[1], "task")) {
    gprint (GP_ERR, "delete task not implemented yet\n");
    return (FALSE);
  }

usage:
  gprint (GP_ERR, "USAGE: delete job (JobID)\n");
  gprint (GP_ERR, "USAGE: delete task (TaskName)\n");
  return (FALSE);
}

/* find job from jobID, kill and delete */
