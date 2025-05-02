# include "pcontrol.h"

int PrintJobStack (int Nstack, char *command, char *hostname, int state, float age);
int PrintHostStack (int Nstack);

int status (int argc, char **argv) {

  int N;

  if (get_argument (argc, argv, "-h")) goto usage;
  if (get_argument (argc, argv, "-help")) goto usage;
  if (get_argument (argc, argv, "--help")) goto usage;

  /* I would like to add the following options:
   * strsub on argv[0]
   * strsub on hostname, realhost
   * list hostname and realhost
   * filter by state
   * filter by dtime
  */

  // -cmd (cmd)
  // -host (hostname)
  // -state (busy, pending, done, kill, exit, crash, resp, hung
  // -age (seconds?) (minutes?)
  // -nohost

  char *COMMAND = NULL;
  if ((N = get_argument (argc, argv, "-cmd"))) {
    remove_argument (N, &argc, argv);
    COMMAND = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char *HOSTNAME = NULL;
  if ((N = get_argument (argc, argv, "-host"))) {
    remove_argument (N, &argc, argv);
    HOSTNAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int STATE = PCONTROL_JOB_ALLJOBS;
  if ((N = get_argument (argc, argv, "-state"))) {
    remove_argument (N, &argc, argv);
    STATE = GetJobStackIDbyName (argv[N]);
    remove_argument (N, &argc, argv);
    if (STATE == PCONTROL_JOB_NONE) goto usage;
  }

  float AGE = 0.0;
  if ((N = get_argument (argc, argv, "-age"))) {
    remove_argument (N, &argc, argv);
    AGE = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int SHOWHOST = TRUE;
  if ((N = get_argument (argc, argv, "-nohost"))) {
    remove_argument (N, &argc, argv);
    SHOWHOST = FALSE;
  }
  if ((N = get_argument (argc, argv, "+jobs"))) {
    remove_argument (N, &argc, argv);
    SHOWHOST = FALSE;
  }

  int SHOWJOBS = TRUE;
  if ((N = get_argument (argc, argv, "-nojobs"))) {
    remove_argument (N, &argc, argv);
    SHOWJOBS = FALSE;
  }
  if ((N = get_argument (argc, argv, "+host"))) {
    remove_argument (N, &argc, argv);
    SHOWJOBS = FALSE;
  }

  if (SHOWJOBS) {
    PrintJobStack (PCONTROL_JOB_ALLJOBS, COMMAND, HOSTNAME, STATE, AGE);
  }
  if (SHOWHOST) {
    PrintHostStack (PCONTROL_HOST_ALLHOSTS);
  }

  return (TRUE);

usage:
  gprint (GP_LOG, "USAGE: status [-cmd command] [-host hostname] [-state state] [-age seconds] [+jobs,-nohost] [+host,-nojobs]\n");
  return FALSE;
}

int PrintJobStack (int Nstack, char *command, char *hostname, int state, float age) {

  int i, j, Nobject;
  Stack *stack;
  Job *job;
  struct timeval now;
  float dtime;

  stack = GetJobStack (Nstack);
  ASSERT (stack != NULL, "programming error");

  LockStack (stack);
  Nobject = stack[0].Nobject;
  gprint (GP_LOG, "job stack %s:  %d objects\n", GetJobStackName(Nstack), Nobject);

  for (i = 0; i < Nobject; i++) {
    job = stack[0].object[i];
    ASSERT (job != NULL, "programming error");

    char *thishost = (job[0].realhost == NULL) ? job[0].hostname : job[0].realhost;

    switch (job[0].state) {
	// for active jobs or pending jobs, print time since start (or create in the case of pending)
      case PCONTROL_JOB_PENDING:
      case PCONTROL_JOB_BUSY:
      case PCONTROL_JOB_RESP:
      case PCONTROL_JOB_HUNG:
	gettimeofday (&now, (void *) NULL);
	dtime = DTIME (now, job[0].start);
	break;

	// for active jobs or pending jobs, print time since start (or create in the case of pending)
      case PCONTROL_JOB_DONE:
      case PCONTROL_JOB_KILL:
      case PCONTROL_JOB_EXIT:
      case PCONTROL_JOB_CRASH:
      default:
	dtime = DTIME (job[0].stop, job[0].start);
	break;
    }

    // check on the filters
    if (command) {
      if (!strstr (job[0].argv[0], command)) continue;
    }
    if (hostname) {
      if (!strstr (thishost, hostname)) continue;
    }
    if (age > 0.0) {
      if (dtime < age) continue;
    }
    if (state != PCONTROL_JOB_ALLJOBS) {
      // allow PCONTROL_JOB_RESP == BUSY
      int validState = FALSE;
      validState |= (state == PCONTROL_JOB_RESP) && (job[0].state == PCONTROL_JOB_BUSY);
      validState |= (state == PCONTROL_JOB_BUSY) && (job[0].state == PCONTROL_JOB_RESP);
      validState |= (state == job[0].state);
      if (!validState) continue;
    }

    gprint (GP_LOG, "%3d %9s ", i, thishost);
    gprint (GP_LOG, "%7s  ", GetJobStackName (job[0].state));
    gprint (GP_LOG, "%8.2f ", dtime);

    PrintID (GP_LOG, job[0].JobID);
    gprint (GP_LOG, " %2d ", job[0].nicelevel);
    for (j = 0; j < job[0].argc; j++) {
      gprint (GP_LOG, "%s ", job[0].argv[j]);
    }
    gprint (GP_LOG, "\n");
  }
  UnlockStack (stack);

  return (TRUE);
}

int PrintHostStack (int Nstack) {

  int i, Nobject;
  Stack *stack;
  Host *host;

  stack = GetHostStack (Nstack);

  LockStack (stack);
  Nobject = stack[0].Nobject;
  gprint (GP_LOG, "host stack %s:  %d objects\n", GetHostStackName(Nstack), Nobject);

  for (i = 0; i < Nobject; i++) {
    host = stack[0].object[i];
    gprint (GP_LOG, "%d  %s  ", i, host[0].hostname);
    gprint (GP_LOG, "%5s  ", GetHostStackName (host[0].stack));
    PrintID (GP_LOG, host[0].HostID);
    gprint (GP_LOG, "\n");
  }
  UnlockStack (stack);

  return (TRUE);
}

// Safe with PTHREAD_MUTEX_INITIALIZER lock
