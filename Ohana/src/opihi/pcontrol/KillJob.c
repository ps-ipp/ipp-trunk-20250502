# include "pcontrol.h"

int KillJob (Job *job, Host *host) {
  
  int status;

  ASSERT (host != NULL, "host missing");
  ASSERT (job != NULL, "job missing");
  ASSERT (host == (Host *) job[0].host, "invalid host");
  ASSERT (job  == (Job *) host[0].job, "invalid job");

  status = PclientCommand (host, "reset", PCLIENT_PROMPT, PCONTROL_RESP_KILL_JOB);

  /* check on success of pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      // unlink host & job
      if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
      job[0].host = NULL;
      host[0].job = NULL;

      // decrement the machine job-host counters
      DelMachineJob (host, job);

      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_CRASH, STACK_BOTTOM);
      return (FALSE);

    case PCLIENT_GOOD:
      if (VerboseMode()) gprint (GP_ERR, "kill job on host %s\n", host[0].hostname);  
      FlushIOBuffer (&host[0].comms_buffer);
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
}

int KillJobResponse (Host *host) {
  
  int status;
  char *p;
  IOBuffer *buffer;
  Job *job;

  ASSERT (host != NULL, "host missing");
  ASSERT (host[0].job, "missing job");
  buffer = &host[0].comms_buffer;
  job = (Job *) host[0].job;

  /** check on response to pclient command **/
  p = memstr (buffer[0].buffer, "STATUS", buffer[0].Nbuffer);
  if (p == NULL) {
      if (VerboseMode()) gprint (GP_ERR, "missing STATUS in response; try again\n");
      PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_KILL, STACK_BOTTOM);
      return (FALSE);
  }
  if (VerboseMode()) gprint (GP_ERR, "client message: %s\n", buffer[0].buffer);

  sscanf (p, "%*s %d", &status);
  gprint (GP_ERR, "client status: %d\n", status);

  switch (status) {
    case -1:
      ABORT ("syntax error to pclient");
    case 0:
      gprint (GP_ERR, "failure to kill child process\n");
      PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_KILL, STACK_BOTTOM);
      return (FALSE);
    case 1:
      gprint (GP_ERR, "killed job %s on %s\n", job[0].argv[0], host[0].hostname);
      // unlink host & job
      job[0].host = NULL;
      host[0].job = NULL;

      // decrement the machine job-host counters
      DelMachineJob (host, job);

      PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_CRASH, STACK_BOTTOM);
      return (TRUE);
    case 2:
      ABORT ("client has no job");
  }
  ABORT ("should not reach here (KillJob)");
}
