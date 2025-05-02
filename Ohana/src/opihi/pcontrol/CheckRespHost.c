# include "pcontrol.h"
# define DEBUG 0

// this function operates on hosts waiting for a response. we simply check if the message is
// complete, and if so, send it to the correct parsing function
int CheckRespHost (Host *host) {
  
  int status;
  Job  *job;

  ASSERT (host, "host not set");
  job = (Job *) host[0].job;

  status = PclientResponse (host, host[0].response, &host[0].comms_buffer);

  /* check on output from pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);

      // not all hosts here have a job; if it does, return it to PENDING
      if (job) {
	// unlink host & job
	if (DEBUG || VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
	job[0].host = NULL;
	host[0].job = NULL;
	PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);
      }

      // clear the response data
      host[0].response_state = PCONTROL_RESP_NONE;
      host[0].response = NULL;

      // if want the host to be shutdown, accept the result
      if (host[0].response_state == PCONTROL_RESP_DOWN_HOST) {
	if (DEBUG) fprintf (stderr, "PCONTROL_RESP_DOWN_HOST\n");
	DownHostResponse (host);
	return TRUE;
      }	
      if (host[0].response_state == PCONTROL_RESP_STOP_HOST) {
	if (DEBUG) fprintf (stderr, "PCONTROL_RESP_STOP_HOST\n");
	StopHostResponse (host);
	return TRUE;
      }

      // host has unexpectedly shutdown; harvest the defunct process
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      return (FALSE);

    case PCLIENT_HUNG:
      // not done yet; try again later
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      if (job) {
	PutJob (job, PCONTROL_JOB_RESP, STACK_BOTTOM);
      }
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "host %s is not responding\n", host[0].hostname);
      return (FALSE);

    case PCLIENT_GOOD:
      break;

    default:
      ABORT ("unknown status for pclient command");  
  }

  switch (host[0].response_state) {
    case PCONTROL_RESP_START_JOB:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_START_JOB\n");
      status = StartJobResponse (host);
      break;

    case PCONTROL_RESP_CHECK_HOST:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_CHECK_HOST\n");
      status = CheckHostResponse (host);
      break;

    case PCONTROL_RESP_CHECK_DONE_HOST:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_CHECK_DONE_HOST\n");
      status = CheckDoneHostResponse (host);
      break;

    case PCONTROL_RESP_CHECK_BUSY_JOB:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_BUSY_JOB\n");
      status = CheckBusyJobResponse (host);
      break;

    case PCONTROL_RESP_KILL_JOB:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_KILL_JOB\n");
      status = KillJobResponse (host);
      break;

    case PCONTROL_RESP_DOWN_HOST:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_DOWN_HOST\n");
      status = DownHostResponse (host);
      break;

    case PCONTROL_RESP_STOP_HOST:
      if (DEBUG) fprintf (stderr, "PCONTROL_RESP_STOP_HOST\n");
      status = StopHostResponse (host);
      break;

    default:
      ABORT ("undefined response state");
  }

  // we have detected a valid response, clear the response data
  host[0].response_state = PCONTROL_RESP_NONE;
  host[0].response = NULL;
  return (status);
}      

