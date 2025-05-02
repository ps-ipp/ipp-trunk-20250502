# include "pcontrol.h"
# define DEBUG 0

int CheckDoneHost (Host *host) {
  
  int       status;

  ASSERT (host, "host not set");

  status = PclientCommand (host, "reset", PCLIENT_PROMPT, PCONTROL_RESP_CHECK_DONE_HOST);

  /* check on success of pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      return (FALSE);
      /* DONE host does not have an incomplete job */
      // XXX do we need to close the connection?

    case PCLIENT_GOOD:
      if (VerboseMode()) gprint (GP_ERR, "checking done host %s\n", host[0].hostname);  
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
  ABORT ("should not reach here (CheckDoneHost)"); 
}

int CheckDoneHostResponse (Host *host) {

  int status;
  char *p;
  IOBuffer *buffer;

  /* job must have assigned host */
  ASSERT (host, "missing host");
  buffer = &host[0].comms_buffer;

  /** successful command, examine result **/
  p = memstr (buffer[0].buffer, "RESET_RESULT:", buffer[0].Nbuffer);
  if (p == NULL) {
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "missing RESET_RESULT: in response; try again\n");
      PutHost (host, PCONTROL_HOST_DONE, STACK_BOTTOM);
      return (FALSE);
  }

  sscanf (p, "%*s %d", &status);
  switch (status) {
    case -1:
      ABORT ("reset syntax error");
      
    case 0:
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "reset failed\n");
      PutHost (host, PCONTROL_HOST_DONE, STACK_BOTTOM);
      return (FALSE);
      
    case 1:
    case 2:
      if (DEBUG || VerboseMode()) gprint (GP_ERR, "successful reset\n");
      PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
      return (FALSE);

    default:
      ABORT ("should not reach here (CheckDoneHost)");
  }
  ABORT ("should not reach here (CheckDoneHost)");
}
