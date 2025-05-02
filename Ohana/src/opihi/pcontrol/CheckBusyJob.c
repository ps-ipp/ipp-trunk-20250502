# include "pcontrol.h"
# define DEBUG 0

int CheckBusyJob (Job *job, Host *host) {

  int status;

  /* we are checking a job which is currently busy.  it has been pulled from the
     JOB_BUSY stack, and is linked to a host in the HOST_BUSY stack.  
     XXX need to check on state of HOST on return */

  ASSERT (job, "job not set");
  ASSERT (host, "host not set");
  ASSERT (host == (Host *) job[0].host, "invalid host");
  ASSERT (job  == (Job *) host[0].job, "invalid job");

  status = PclientCommand (host, "status", PCLIENT_PROMPT, PCONTROL_RESP_CHECK_BUSY_JOB);

  /* check on success of pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      // free the realhost name
      if (job[0].realhost) free (job[0].realhost);
      job[0].realhost = NULL;

      // unlink host & job
      if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
      job[0].host = NULL;
      host[0].job = NULL;
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);
      return (FALSE);

    case PCLIENT_GOOD:
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "message received (CheckBusyJob)");
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
}

int CheckBusyJobResponse (Host *host) {

  int      outstate;
  char    *p;
  char     string[64];
  IOBuffer *buffer;
  Job *job;

  /* job must have assigned host */
  ASSERT (host, "missing host");
  ASSERT (host[0].job, "missing job");
  buffer = &host[0].comms_buffer;
  job = (Job *) host[0].job;
  ASSERT (host == (Host *) job[0].host, "invalid host");

  /** host is up, need to parse message **/
  p = memstr (buffer[0].buffer, "STATUS", buffer[0].Nbuffer);
  if (p == NULL) {
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "missing STATUS in response; try again\n");
      PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_BUSY, STACK_BOTTOM);
      return (TRUE);
  }

  sscanf (p, "%*s %s", string);
  ASSERT (strcmp(string, "NONE"), "no current job\n");

  /** no status change, return to BUSY stack **/
  if (!strcmp(string, "BUSY")) {
    PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
    PutJob (job, PCONTROL_JOB_BUSY, STACK_BOTTOM);
    return (TRUE);
  }

  /* exit status better be either EXIT or CRASH */
  outstate = PCONTROL_JOB_BUSY;
  if (!strcmp(string, "EXIT")) outstate = PCONTROL_JOB_EXIT;
  if (!strcmp(string, "CRASH")) outstate = PCONTROL_JOB_CRASH;
  if (outstate == PCONTROL_JOB_BUSY) {
    if (DEBUG || VerboseMode()) gprint (GP_ERR, "invalid status response (CheckBusyJobResponse), try again\n");
    PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
    PutJob (job, PCONTROL_JOB_BUSY, STACK_BOTTOM);
    return (TRUE);
  }
  ASSERT (outstate != PCONTROL_JOB_DONE, "impossible outstate for job");

  /* parse the exit status and sizes of output buffers */
  p = memstr (buffer[0].buffer, "EXITST", buffer[0].Nbuffer);
  sscanf (p, "%*s %d", &job[0].exit_status);
  p = memstr (buffer[0].buffer, "STDOUT", buffer[0].Nbuffer);
  sscanf (p, "%*s %d", &job[0].stdout_buf.size);
  p = memstr (buffer[0].buffer, "STDERR", buffer[0].Nbuffer);
  sscanf (p, "%*s %d", &job[0].stderr_buf.size);

  // XXX runaway job if output too large?
  if (job[0].stdout_buf.size > 0x10000000) abort();
  if (job[0].stderr_buf.size > 0x10000000) abort();

  // job has exited : move to DONE stack 
  // the host is still BUSY until job output is gathered (int CheckDoneJob)
  // don't unlink job and host yet
  PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
  ASSERT (job[0].host, "host not set for job");
  PutJobSetState (job, PCONTROL_JOB_DONE, STACK_BOTTOM, outstate);
  gettimeofday (&job[0].stop, NULL);
  job[0].dtime = DTIME(job[0].stop, job[0].start);
  return (TRUE);
}
