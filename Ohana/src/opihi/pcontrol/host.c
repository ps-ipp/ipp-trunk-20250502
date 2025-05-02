# include "pcontrol.h"

int host (int argc, char **argv) {

  int N, max_threads;
  IDtype HostID;
  Host *host;
  Stack *AllHosts;

  max_threads = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    max_threads = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) goto usage;

  AllHosts = GetHostStack (PCONTROL_HOST_ALLHOSTS);

  if (!strcasecmp (argv[1], "ADD")) {
    HostID = AddHost (argv[2], max_threads);
    gprint (GP_LOG, "HostID: %d\n", (int) HostID);
    return (TRUE);
  }

  if (max_threads) goto usage;

  // this one is safe from in-flight entries: no one else pulls from OFF
  if (!strcasecmp (argv[1], "ON")) {
    host = PullHostFromStackByName (PCONTROL_HOST_OFF, argv[2]);
    if (!host) {
      gprint (GP_LOG, "host %s is not OFF\n", argv[2]);
      return (FALSE);
    }
    host[0].markoff = FALSE;
    DownHost (host);
    return (TRUE);
  }

  // this is a race condition with "CheckDownHosts", but the only 
  // consequence is that both StartHost and reset set the times to 0.0
  if (!strcasecmp (argv[1], "RETRY")) {
    // no need to use a check point [thief: CheckDownHost (DOWN->IDLE)]
    host = PullHostFromStackByName (PCONTROL_HOST_ALLHOSTS, argv[2]);
    if (!host) {
      gprint (GP_LOG, "host %s not found\n", argv[2]);
      return (FALSE);
    }
    if (host[0].stack != PCONTROL_HOST_DOWN) {
      gprint (GP_LOG, "host %s is not DOWN\n", argv[2]);
      return (FALSE);
    }
    /* reset time, place back on ALLHOSTS stack */
    host[0].next_start_try.tv_sec  = 0;
    host[0].next_start_try.tv_usec = 0;
    host[0].last_start_try.tv_sec  = 0;
    host[0].last_start_try.tv_usec = 0;
    PushStack (AllHosts, STACK_BOTTOM, host, host[0].HostID, host[0].hostname);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "CHECK")) {
    host = PullHostFromStackByName (PCONTROL_HOST_ALLHOSTS, argv[2]);
    if (host == NULL) {
      gprint (GP_LOG, "host %s not found\n", argv[2]);
      return (FALSE);
    }
    gprint (GP_LOG, "host %s is %s\n", argv[2], GetHostStackName (host[0].stack));
    PushStack (AllHosts, STACK_BOTTOM, host, host[0].HostID, host[0].hostname);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "OFF")) {
    host = PullHostFromStackByName (PCONTROL_HOST_ALLHOSTS, argv[2]);
    if (host == NULL) {
      gprint (GP_LOG, "host %s not found\n", argv[2]);
      return (FALSE);
    }
    host[0].markoff = TRUE;
    PushStack (AllHosts, STACK_BOTTOM, host, host[0].HostID, host[0].hostname);
    return (TRUE);
  }

  // this one is safe from in-flight entries: no one else pulls from OFF
  if (!strcasecmp (argv[1], "DELETE")) {
    host = PullHostFromStackByName (PCONTROL_HOST_OFF, argv[2]);
    if (!host) {
      gprint (GP_LOG, "host %s is not OFF\n", argv[2]);
      return (FALSE);
    }
    DelHost (host);
    return (TRUE);
  }
  
usage:
  gprint (GP_LOG, "USAGE: host (command) (hostname)\n");
  gprint (GP_LOG, "  valid commands: add, on, retry, check, off, delete\n");
  gprint (GP_LOG, "  -threads Nthreads is optional for 'add'\n");
  return (FALSE);
}
