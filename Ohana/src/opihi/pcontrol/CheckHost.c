# include "pcontrol.h"

// if the host has a job, we skip it (down or crash state will be caught elsewhere)
// in fact, just touch the IDLE hosts, not the BUSY hosts?
int CheckHost (Host *host) {
  
  int status;

  ASSERT (host, "host not set");

  if (host[0].job != NULL) return (TRUE);

  /* if this host has been marked to be turned off, do that and return */
  if (host[0].markoff) {
    host[0].markoff = FALSE;
    StopHost (host, PCONTROL_HOST_OFF);
    return (TRUE);
  }

  // the argument to echo (OK) is the expected response below in CheckHostResponse
  status = PclientCommand (host, "echo OK", PCLIENT_PROMPT, PCONTROL_RESP_CHECK_HOST);

  switch (status) {
    case PCLIENT_DOWN:
      if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      return (FALSE);
      
    case PCLIENT_GOOD:
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
  ABORT ("should not reach here (CheckHost)"); 
}

int CheckHostResponse (Host *host) {
  
  /* we only check IDLE hosts without jobs */
  ASSERT (host, "missing host");

  // XXX check on the value of the response? (OK)

  PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
  return (TRUE);
}
