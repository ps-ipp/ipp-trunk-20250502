# include "pcontrol.h"

int kill_pc (int argc, char **argv) {

  Job *job;
  int JobID;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: kill (JobID)\n");
    return (FALSE);
  }
  JobID = atoi (argv[1]);

  /* XXX this function should only fail if a process is hung */
  job = PullJobFromStackByID (PCONTROL_JOB_BUSY, JobID);
  if (job == NULL) {
    gprint (GP_ERR, "job %s not BUSY\n", argv[1]);
    /* make output message more readable by scheduler */
    return (FALSE);
  }

  PutJob (job, PCONTROL_JOB_KILL, STACK_BOTTOM);
  return (TRUE);
}
