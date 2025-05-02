# include "pcontrol.h"

int delete (int argc, char **argv) {

  Job *job;
  int JobID;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: delete (JobID)\n");
    return (FALSE);
  }
  JobID = GetID (argv[1]);
  if (!JobID) {
    gprint (GP_ERR, "invalid job id %s\n", argv[1]);
    return (FALSE);
  }
      
  /* use a string interp to convert JobIDs to ints ? */

  job = PullJobFromStackByID (PCONTROL_JOB_PENDING, JobID);
  if (job != NULL) goto found;

  job = PullJobFromStackByID (PCONTROL_JOB_CRASH, JobID);
  if (job != NULL) goto found;

  job = PullJobFromStackByID (PCONTROL_JOB_EXIT, JobID);
  if (job != NULL) goto found;

  gprint (GP_ERR, "job %s not PENDING, CRASH, EXIT\n", argv[1]);
  return (FALSE);
  
found:
  {
    int j;
    gprint (GP_LOG, "deleting job  %s  %d  ", job[0].hostname, job[0].argc);
    for (j = 0; j < job[0].argc; j++) {
      gprint (GP_LOG, "%s ", job[0].argv[j]);
    }
    PrintID (GP_LOG, job[0].JobID);
    gprint (GP_LOG, "\n");
  }  
  DelJob (job);

  return (TRUE);
}

/**** at the moment, this function requires the job to be in the correct state
      to be deleted.  This should be changed to kill, then delete job if it is 
      not in the correct state 
****/
