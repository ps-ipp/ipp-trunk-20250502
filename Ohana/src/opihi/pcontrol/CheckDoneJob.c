# include "pcontrol.h"
# define DEBUG 0

int CheckDoneJob (Job *job, Host *host) {
  
  int status1, status2;

  ASSERT (job, "job not set");
  ASSERT (host, "host not set");

  ASSERT (host == (Host *) job[0].host, "invalid host");
  ASSERT (job == (Job *) host[0].job, "invalid job");

  // we have four possible states here:
  // 1) stdout & stderr not yet requested
  // 2) stdout requested, not yet completed
  // 3) stdout completed, stderr not yet requested
  // 4) stdout completed, stderr requested, stderr not yet completed

  // we can always call this for stdout (if it is done, this is a NOP)
  status1 = GetJobOutput ("stdout", host, &job[0].stdout_buf);

  // we cannot try stderr until stdout is completed
  status2 = PCLIENT_HUNG;
  if (job[0].stdout_buf.completed) {
      status2 = GetJobOutput ("stderr", host, &job[0].stderr_buf);
  }

  if ((status1 == PCLIENT_DOWN) || (status2 == PCLIENT_DOWN)) {

    // decrement the machine job-host counters
    DelMachineJob (host, job);

    // unlink host & job
    if (DEBUG || VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
    job[0].host = NULL;
    host[0].job = NULL;
	
    PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);

    // clear the response data
    host[0].response_state = PCONTROL_RESP_NONE;
    host[0].response = NULL;

    // host has shutdown; harvest the defunct process
    HarvestHost (host[0].pid);
    PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
    return (FALSE);
  }

  // try again if we are still waiting
  if ((status1 == PCLIENT_HUNG) || (status2 == PCLIENT_HUNG)) {
    PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
    ASSERT (job[0].host, "host not set for job");
    PutJobSetState (job, PCONTROL_JOB_DONE, STACK_BOTTOM, job[0].state); // keep the incoming state
    return (FALSE);
  }

  // decrement the machine job-host counters
  DelMachineJob (host, job);

  /* job's state is either EXIT or CRASH (verify?) */
  // unlink host & job
  job[0].host = NULL;
  host[0].job = NULL;
  PutHost (host, PCONTROL_HOST_DONE, STACK_BOTTOM);

  ASSERT ((job[0].state == PCONTROL_JOB_EXIT) || (job[0].state == PCONTROL_JOB_CRASH), "unexpected job state");

  PutJob (job, job[0].state, STACK_BOTTOM);

  return (TRUE);
}
