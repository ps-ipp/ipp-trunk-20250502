# include "pcontrol.h"
# define RETRY_BASE 10.0

int StartHost (Host *host) {

  int pid;
  int stdio[3];
  char command[64], shell[64];
  struct timeval now;
  float delta;

  /* perhaps change the name of these config variables... */
  if (VarConfig ("COMMAND", "%s", command) == NULL) strcpy (command, "ssh");
  if (VarConfig ("SHELL", "%s", shell)     == NULL) strcpy (shell, "pclient");

  if (VerboseMode()) gprint (GP_ERR, "starting remote connection to %s...", host[0].hostname);

  int errorInfo;
  pid = rconnect (command, host[0].hostname, shell, stdio, &errorInfo, TRUE);
  if (!pid) {     
    /** failure to start: extend retry period **/
    if (VerboseMode()) gprint (GP_ERR, "failure to start %s (error %d)\n", host[0].hostname, errorInfo);
    gettimeofday (&now, (void *) NULL);
    if (ZTIME(host[0].next_start_try) || ZTIME(host[0].last_start_try)) {
      /* reset retry period if either is zero */
      delta = RETRY_BASE;
    } else {
      delta = MAX(1.0, 2*DTIME (host[0].next_start_try, host[0].last_start_try));
    }
    host[0].next_start_try.tv_sec  = now.tv_sec  + delta;
    host[0].next_start_try.tv_usec = now.tv_usec;
    host[0].last_start_try.tv_sec  = now.tv_sec;
    host[0].last_start_try.tv_usec = now.tv_usec;
    PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
    return (FALSE);
  }
  host[0].next_start_try.tv_sec  = 0;
  host[0].next_start_try.tv_usec = 0;
  host[0].last_start_try.tv_sec  = 0;
  host[0].last_start_try.tv_usec = 0;

  // set the connection time
  gettimeofday (&host[0].connect_time, (void *) NULL);

  host[0].stdin_fd  = stdio[0];
  host[0].stdout_fd = stdio[1];
  host[0].stderr_fd = stdio[2];
  host[0].pid       = pid;
  PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
  return (TRUE);
}
