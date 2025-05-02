# include "pcontrol.h"

Stack *HostPool_AllHosts;  // virtual pool for user status queries

Stack *HostPool_Idle; // these hosts are waiting for something to do
Stack *HostPool_Busy; // these hosts are working
Stack *HostPool_Resp; // these hosts are trying to respond
Stack *HostPool_Done; // these hosts have finished a job
Stack *HostPool_Down; // these hosts are not responding
Stack *HostPool_Off;  // these hosts are off

void InitHostStacks () {
  HostPool_AllHosts = InitStack ();

  HostPool_Idle = InitStack ();
  HostPool_Busy = InitStack ();
  HostPool_Resp = InitStack ();
  HostPool_Done = InitStack ();
  HostPool_Down = InitStack ();
  HostPool_Off  = InitStack ();
}

void FreeHostStack (Stack *stack) {
  Host *host;
  while ((host = PullStackByLocation (stack, stack[0].Nobject - 1)) != NULL) {
    DelHost (host);
  }
  FreeStack (stack);
}

void FreeHostStacks () {
  FreeHostStack (HostPool_Idle);
  FreeHostStack (HostPool_Busy);
  FreeHostStack (HostPool_Resp);
  FreeHostStack (HostPool_Done);
  FreeHostStack (HostPool_Down);
  FreeHostStack (HostPool_Off );

  // AllHosts is a virtual stack : all hosts are references
  FreeStack (HostPool_AllHosts);
}

char *GetHostStackName (int StackID) {
  switch (StackID) {
    case PCONTROL_HOST_ALLHOSTS: return ("ALLHOSTS");
    case PCONTROL_HOST_IDLE: return ("IDLE");
    case PCONTROL_HOST_DOWN: return ("DOWN");
    case PCONTROL_HOST_RESP: return ("RESP");
    case PCONTROL_HOST_DONE: return ("DONE");
    case PCONTROL_HOST_BUSY: return ("BUSY");
    case PCONTROL_HOST_OFF:  return ("OFF");
  }
  gprint (GP_ERR, "error: unknown host stack : programming error\n");
  pcontrol_exit (51);
  return (NULL);
}

Stack *GetHostStack (int StackID) {
  switch (StackID) {
    case PCONTROL_HOST_ALLHOSTS: return (HostPool_AllHosts);
    case PCONTROL_HOST_IDLE: return (HostPool_Idle);
    case PCONTROL_HOST_DOWN: return (HostPool_Down);
    case PCONTROL_HOST_RESP: return (HostPool_Resp);
    case PCONTROL_HOST_DONE: return (HostPool_Done);
    case PCONTROL_HOST_BUSY: return (HostPool_Busy);
    case PCONTROL_HOST_OFF:  return (HostPool_Off);
  }
  gprint (GP_ERR, "error: unknown host stack : programming error\n");
  pcontrol_exit (52);
  return (NULL);
}

Stack *GetHostStackByName (char *name) {
  if (!strcasecmp (name, "all")) return (HostPool_AllHosts);
  if (!strcasecmp (name, "idle")) return (HostPool_Idle);
  if (!strcasecmp (name, "down")) return (HostPool_Down);
  if (!strcasecmp (name, "resp")) return (HostPool_Resp);
  if (!strcasecmp (name, "done")) return (HostPool_Done);
  if (!strcasecmp (name, "busy")) return (HostPool_Busy);
  if (!strcasecmp (name, "off"))  return (HostPool_Off);
  return (NULL);
}

/* add host to position in stack */
int PutHost (Host *host, int StackID, int where) {

  int stat;
  Stack *stack;

  // fprintf (stderr, "move host %s to %s\n", host[0].hostname, GetHostStackName(StackID));

  stack = GetHostStack (StackID);
  if (stack == NULL) return (FALSE);

  host[0].stack = StackID;
  stat = PushStack (stack, where, host, host[0].HostID, host[0].hostname);
  // XXX need to handle the error conditions, or we drop the host & leak memory
  return (stat);
}
  
/* find the host by ID in the defined host stacks */
Host *PullHostByID (IDtype HostID, int *StackID) {

  Host *host;

  *StackID = PCONTROL_HOST_IDLE;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_DOWN;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_RESP;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_DONE;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_BUSY;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_OFF;
  host = PullHostFromStackByID (*StackID, HostID);
  if (host != NULL) return (host);

  *StackID = -1;
  return (NULL);
}

/* find the host by ID in the defined host stacks */
Host *PullHostByName (char *name, int *StackID) {

  Host *host;

  *StackID = PCONTROL_HOST_IDLE;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_DOWN;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_RESP;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_DONE;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_BUSY;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = PCONTROL_HOST_OFF;
  host = PullHostFromStackByName (*StackID, name);
  if (host != NULL) return (host);

  *StackID = -1;
  return (NULL);
}

Host *PullHostFromStackByID (int StackID, IDtype ID) {

  Host *host;
  Stack *stack;

  stack = GetHostStack (StackID);
  if (stack == NULL) return (NULL);

  host = PullStackByID (stack, ID);
  return (host);
}

Host *PullHostFromStackByName (int StackID, char *name) {

  Host *host;
  Stack *stack;

  stack = GetHostStack (StackID);
  if (stack == NULL) return (NULL);

  host = PullStackByName (stack, name);
  return (host);
}

IDtype AddHost (char *hostname, int max_threads) {

  Host *host;

  ALLOCATE (host, Host, 1);

  host[0].hostname    = strcreate (hostname);
  host[0].max_threads = max_threads;
  host[0].stdin_fd    = 0;
  host[0].stdout_fd   = 0;
  host[0].stderr_fd   = 0;
  host[0].HostID      = NextHostID();

  host[0].last_start_try.tv_sec  = 0;
  host[0].last_start_try.tv_usec = 0;
  host[0].next_start_try.tv_sec  = 0;
  host[0].next_start_try.tv_usec = 0;

  InitIOBuffer (&host[0].comms_buffer, 0x100);
  host[0].response_state = PCONTROL_RESP_NONE;
  host[0].response = NULL;

  host[0].markoff  = FALSE;
  host[0].job      = NULL;

  AddMachineHost (host);

  PutHost (host, PCONTROL_HOST_ALLHOSTS, STACK_BOTTOM);
  PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
  return (host[0].HostID);
}

void DelHost (Host *host) {

  Host *copy;

  copy = PullStackByID (HostPool_AllHosts, host[0].HostID);
  ASSERT (copy == host, "programming error: ALLHOSTS entry does not match");

  DelMachineHost (host);

  FreeIOBuffer (&host[0].comms_buffer);
  FREE (host[0].hostname);
  FREE (host);
}
