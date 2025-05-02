# include "pcontrol.h"

// we attempt to harvest the 'down' hosts in HarvestHost.  However, sometimes the
// child is busy and does not exit in the timeout period.  we need to keep a list and
// try again occasionally to free up the needed resources
static int NUNHARVESTED = 0;
static int Nunharvested = 0;
static int *unharvested = NULL;

void DownHost (Host *host) {
  CLOSE (host[0].stdin_fd);
  CLOSE (host[0].stdout_fd);
  CLOSE (host[0].stderr_fd);
  host[0].job = NULL;
  PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
}

void OffHost (Host *host) {
  CLOSE (host[0].stdin_fd);
  CLOSE (host[0].stdout_fd);
  CLOSE (host[0].stderr_fd);
  host[0].job = NULL;
  PutHost (host, PCONTROL_HOST_OFF, STACK_BOTTOM);
}

/* for use by shutdown: force machines which are up to go down
   wait for a little while for the client thread to take care 
   of them
*/
   
int DownHosts () {

  int i, Nobject, Nwait;
  Stack *stack;
  Host  *host;

  SetCheckPoint (); // ensure we can find the specified host
  stack = GetHostStack (PCONTROL_HOST_IDLE);
  ASSERT (stack != NULL, "stack missing");
  Nobject = stack[0].Nobject;
  for (i = 0; i < Nobject; i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) continue;
    host[0].markoff = TRUE;
    PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
  }

  stack = GetHostStack (PCONTROL_HOST_BUSY);
  ASSERT (stack != NULL, "stack missing");
  Nobject = stack[0].Nobject;
  for (i = 0; i < Nobject; i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) continue;
    host[0].markoff = TRUE;
    PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
  }
  ClearCheckPoint ();

  Nwait = 0;
  stack = GetHostStack (PCONTROL_HOST_IDLE);
  ASSERT (stack != NULL, "stack missing");

  gprint (GP_ERR, "waiting for clients to exit");
  while ((Nwait < 15) && stack[0].Nobject) {
    gprint (GP_ERR, ".");
    usleep (100000); // wait for clients to exit
    Nwait++;
  }
  gprint (GP_ERR, "\n");
  if (stack[0].Nobject) {
    gprint (GP_ERR, "trouble shutting down all pclient instances: %d still alive\n", stack[0].Nobject);
  } else {
    gprint (GP_ERR, "done\n");
  }
  return (TRUE);
}

int StopHost (Host *host, int mode) {

  int       status;

  switch (mode) {
    case PCONTROL_HOST_DOWN:
      status = PclientCommand (host, "exit", "Goodbye", PCONTROL_RESP_DOWN_HOST);
      break;
    case PCONTROL_HOST_OFF:
      status = PclientCommand (host, "exit", "Goodbye", PCONTROL_RESP_STOP_HOST);
      break;
    default:
      ABORT ("programming error: invalid StopHost mode");
  }

  /* check on success of pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      return (TRUE);

    case PCLIENT_GOOD:
      if (VerboseMode()) gprint (GP_ERR, "stop host %s\n", host[0].hostname);  
      FlushIOBuffer (&host[0].comms_buffer);
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
  ABORT ("should not reach here");  
}

int StopHostResponse (Host *host) {

  OffHost (host);
  HarvestHost (host[0].pid);
  return (TRUE);
}

int DownHostResponse (Host *host) {

  DownHost (host);
  HarvestHost (host[0].pid);
  return (TRUE);
}

/* the host is thought to be down; check for child exit status */
int HarvestHost (int pid) {
  
  int i, result, waitstatus;

  if (VerboseMode()) gprint (GP_ERR, "harvesting within thread\n");
  if (VerboseMode()) gprint (GP_ERR, "child process %d is down, wait for exit status\n", pid);
  
  // Loop a few times waiting for child to exit
  for (i = 0; i < 50; i++) {
    result = waitpid (pid, &waitstatus, WNOHANG);
    if ((result == -1) && (errno == ECHILD)) {
      usleep (10000); // wait for child to exit
      continue;
    } else {
      break;
    }
  }
  switch (result) {
    case -1:  /* error with waitpid */
      switch (errno) {
	case ECHILD:
	  gprint (GP_ERR, "HarvestHost: unknown PID (%d), not a child proc\n", pid);
	  gprint (GP_ERR, "did process already exit?  programming error?\n");
	  break;
	case EINTR:
	case EINVAL:
	default:
	  perror ("unexpected error");
	  ABORT ("(HarvestHost)");
      }
      break;
      
    case 0:
      gprint (GP_ERR, "HarvestHost: child with connection to remote host failed to exit: may be hung\n");
      AddZombie(pid);
      break;

    default:
      if (result != pid) {
	gprint (GP_ERR, "waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, pid);
	pcontrol_exit (58);
      }
      
      if (WIFEXITED(waitstatus)) {
	if (VerboseMode()) gprint (GP_ERR, "child exited with status %d\n", WEXITSTATUS(waitstatus));
      }
      if (WIFSIGNALED(waitstatus)) {
	if (VerboseMode()) gprint (GP_ERR, "child crashed with status %d\n", WTERMSIG(waitstatus));
      }
      if (WIFSTOPPED(waitstatus)) {
        gprint (GP_ERR, "waitpid returns 'stopped': programming error\n");
	pcontrol_exit (59);
      }
  }
  return (TRUE);
}

int AddZombie(int pid) {

  if (unharvested == NULL) {
    NUNHARVESTED = 128;
    ALLOCATE (unharvested, int, NUNHARVESTED);
    memset (unharvested, 0, NUNHARVESTED*sizeof(int));
  }
  unharvested[Nunharvested] = pid;

  Nunharvested ++;
  if (Nunharvested >= NUNHARVESTED) {
    NUNHARVESTED += 128;
    REALLOCATE (unharvested, int, NUNHARVESTED);
    memset (&unharvested[Nunharvested], 0, (NUNHARVESTED - Nunharvested)*sizeof(int));
  }
  return TRUE;
}

int DelZombies() {

  int i, j;

  if (!unharvested) return FALSE;
  if (!Nunharvested) return FALSE;
  if (!NUNHARVESTED) return FALSE;

  int *newlist = NULL;

  ALLOCATE (newlist, int, NUNHARVESTED);
  memset (newlist, 0, NUNHARVESTED*sizeof(int));

  j = 0;
  for (i = 0; i < NUNHARVESTED; i++) {
    if (!unharvested[i]) continue;
    newlist[j] = unharvested[i];
    j++;
  }
  free (unharvested);
  unharvested = newlist;
  Nunharvested = j;
  return TRUE;
}

int CheckZombies() {

  int pid, i, result, waitstatus;

  if (!unharvested) return FALSE;
  if (!Nunharvested) return FALSE;
  if (!NUNHARVESTED) return FALSE;

  for (i = 0; i < Nunharvested; i++) {
    if (!unharvested[i]) continue;
    pid = unharvested[i];
    result = waitpid (pid, &waitstatus, WNOHANG);
    switch (result) {
      case -1:  /* error with waitpid */
	switch (errno) {
	  case ECHILD:
	    gprint (GP_ERR, "CheckZombies: unknown PID (%d), not a child proc\n", pid);
	    gprint (GP_ERR, "did process already exit?  programming error?\n");
	    break;
	  case EINTR:
	  case EINVAL:
	  default:
	    perror ("unexpected error");
	    ABORT ("CheckZombies impossible condition");
	}
	break;
      
      case 0:
	if (VerboseMode()) gprint (GP_ERR, "CheckZombies: still waiting on %d\n", pid);
	break;

      default:
	if (result != pid) {
	  gprint (GP_ERR, "waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, pid);
	  ABORT ("CheckZombies impossible condition");
	}
	
	if (WIFEXITED(waitstatus)) {
	  if (VerboseMode()) gprint (GP_ERR, "child exited with status %d\n", WEXITSTATUS(waitstatus));
	}
	if (WIFSIGNALED(waitstatus)) {
	  if (VerboseMode()) gprint (GP_ERR, "child crashed with status %d\n", WTERMSIG(waitstatus));
	}
	if (WIFSTOPPED(waitstatus)) {
	  ABORT ("waitpid returns 'stopped': programming error\n");
	}
	unharvested[i] = 0;
	break;
    }
  }
  DelZombies();
  return (TRUE);
}
