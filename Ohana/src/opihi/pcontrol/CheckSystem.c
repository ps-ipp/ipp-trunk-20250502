# include "pcontrol.h"
# define DEBUG 0

static struct timeval lastlive = {0, 0};
static RunLevels RunLevel = PCONTROL_RUN_NONE;

static int CheckSystemThreadRuns = TRUE;

void QuitCheckSystemThread (void) {
  CheckSystemThreadRuns = FALSE;  
}


RunLevels SetRunLevel (RunLevels level) {
  RunLevels oldlevel;
  oldlevel = RunLevel;
  RunLevel = level;
  return oldlevel;
}

RunLevels GetRunLevel () {
  return RunLevel;
}

int CheckSystem () {

  struct timeval now;
  float dtime;

  /* we want to give each block a maximum allowed time */
  CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_NEEDHOST); /* submit a new job */
  CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_WANTHOST); /* submit a new job */
  CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_ANYHOST);  /* submit a new job */
  CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_OLDWANT);  /* submit a new job */

  CheckBusyJobs(0.020);  /* get job status */
  CheckDoneJobs(0.020);  /* harvest job stdout/stderr */
  CheckKillJobs(0.020);  /* harvest job stdout/stderr */

  CheckDoneHosts(0.020); /* reset the host */
  CheckDownHosts(0.100); /* launch the host */

  /* always allow at least one test */
  /* most tests require about 2ms per host.  
     CheckDoneJobs must depend on the size of the output buffer */

  gettimeofday (&now, (void *) NULL);
  dtime = DTIME (now, lastlive);
  if (dtime > 1.0) {
    CheckLiveHosts(0.040);
    lastlive = now;
  } 

  if (DEBUG) { 
    Stack *stack;
    int Nidle, Ndown, Nbusy;
    stack = GetHostStack (PCONTROL_HOST_IDLE);
    Nidle = stack[0].Nobject;
    stack = GetHostStack (PCONTROL_HOST_DOWN);
    Ndown = stack[0].Nobject;
    stack = GetHostStack (PCONTROL_HOST_BUSY);
    Nbusy = stack[0].Nobject;
    gprint (GP_ERR, "busy, idle, down: %2d %2d %2d\n", Nbusy, Nidle, Ndown);
  }

  return (TRUE);
}

void *CheckSystem_Threaded (void *data) {
  OHANA_UNUSED_PARAM(data);

  int Njobchecks, Nhostchecks, Ndonejobs;

  gprintInit ();

  while (CheckSystemThreadRuns) {
    // stop here if the user-thread requests (no objects in flight) 
    TestCheckPoint ();

    // don't run the system checks if RunLevel is FALSE
    // XXX stop should not suspend all checks: we should continue
    // to harvest completed jobs and migrate idle machines to down
    if (RunLevel == PCONTROL_RUN_NONE) {
      usleep (100000); // idle if we are running nothing
      continue;
    }

    /* always allow at least one test */
    /* most tests require about 2ms per host.  
       CheckDoneJobs must depend on the size of the output buffer */
    /* the max delay times are fairly arbitrary and do not impact
       the user interface.
     */

    Njobchecks = 0;
    Nhostchecks = 0;
    Ndonejobs = 0;

    if ((RunLevel == PCONTROL_RUN_ALL) || (RunLevel == PCONTROL_RUN_REAP)) {
      Njobchecks  += CheckBusyJobs(0.020);  /* get job status (PCLIENT) */
      TestCheckPoint ();
      Ndonejobs    = CheckDoneJobs(0.020);  /* harvest job stdout/stderr (!PCLIENT) */
      Njobchecks  += Ndonejobs;
      TestCheckPoint ();
      Njobchecks  += CheckKillJobs(0.020);  /* harvest job stdout/stderr (PCLIENT) */
      TestCheckPoint ();
    }

    if (RunLevel != PCONTROL_RUN_NONE) {
      Nhostchecks += CheckRespHosts(0.020); /* check for incoming messages */
      TestCheckPoint ();
      Nhostchecks += CheckDoneHosts(0.020); /* reset the host */
      TestCheckPoint ();
      Nhostchecks += CheckDownHosts(0.100); /* launch the host */
      TestCheckPoint ();
      CheckZombies(); /* launch the host */
      TestCheckPoint ();
    }

    if (RunLevel == PCONTROL_RUN_ALL) {
      // we want to give each block a maximum allowed time
      Nhostchecks += CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_NEEDHOST); /* submit a new job (PCLIENT) */
      Nhostchecks += CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_WANTHOST); /* submit a new job (PCLIENT) */
      Nhostchecks += CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_ANYHOST); /* submit a new job (PCLIENT) */
      Nhostchecks += CheckIdleHosts(0.015, PCONTROL_JOB_STAGE_OLDWANT); /* submit a new job (PCLIENT) */
      TestCheckPoint ();
    }

    // there is nothing on the stacks.  test the hosts and wait a bit
    if (!Njobchecks && !Nhostchecks && (RunLevel != PCONTROL_RUN_NONE)) {
      CheckLiveHosts(0.040);
      // fprintf (stderr, "sleep a bit\n");
      usleep (100000); // idle if no jobs are waiting
    } else {
      // if we only have busy jobs, pause a moment before trying again
      if (!Ndonejobs) {
	// fprintf (stderr, "sleep a bit\n");
	usleep (100000);
      }
    }

    if (DEBUG) { 
      Stack *stack;
      int Nidle, Ndown, Nbusy;
      stack = GetHostStack (PCONTROL_HOST_IDLE);
      Nidle = stack[0].Nobject;
      stack = GetHostStack (PCONTROL_HOST_DOWN);
      Ndown = stack[0].Nobject;
      stack = GetHostStack (PCONTROL_HOST_BUSY);
      Nbusy = stack[0].Nobject;
      gprint (GP_ERR, "Njobchecks: %d, busy, idle, down: %2d %2d %2d\n", Njobchecks, Nbusy, Nidle, Ndown);
    }
  }
  return (NULL);
}

int CheckBusyJobs (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *hoststack;
  Stack *jobstack;
  Job   *job;
  Host  *host;
  float dtime;

  /* Loop through objects on the stack, no more than once.  Note that it is not important if the
     stack size is modified by other threads or is changed by any of the actions performed during
     this loop: the Nobject value is only used to get a rough number for the number of iterations.
   */

  hoststack = GetHostStack (PCONTROL_HOST_BUSY);
  jobstack  = GetJobStack (PCONTROL_JOB_BUSY);
  Nobject   = jobstack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    // pull both job and host from their stacks
    LockStack (hoststack);
    job = PullStackByLocation (jobstack, STACK_TOP);
    if (job == NULL) {
      UnlockStack (hoststack);
      break;
    }
    host = (Host *) job[0].host;
    ASSERT (host != NULL, "host is NULL");
    RemoveStackByID (hoststack, host[0].HostID);
    UnlockStack (hoststack);

    CheckBusyJob (job, host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG && (Nobject > 0)) gprint (GP_ERR, "checked %d of %d jobs\n", i, Nobject);
  return (i);
}

int CheckDoneJobs (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *hoststack;
  Stack *jobstack;
  Job   *job;
  Host  *host;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  hoststack = GetHostStack (PCONTROL_HOST_BUSY);
  jobstack  = GetJobStack (PCONTROL_JOB_DONE);
  Nobject   = jobstack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    LockStack (hoststack);
    job = PullStackByLocation (jobstack, STACK_TOP);
    if (job == NULL) {
      UnlockStack (hoststack);
      break;
    }
    host = (Host *) job[0].host;
    ASSERT (host, "host is NULL");

    RemoveStackByID (hoststack, host[0].HostID);
    UnlockStack (hoststack);

    CheckDoneJob (job, host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG && (Nobject > 0)) gprint (GP_ERR, "checked %d of %d jobs\n", i, Nobject);
  return (i);
}

int CheckKillJobs (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *hoststack;
  Stack *jobstack;
  Job   *job;
  Host  *host;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  hoststack = GetHostStack (PCONTROL_HOST_BUSY);
  jobstack = GetJobStack (PCONTROL_JOB_KILL);
  Nobject = jobstack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    LockStack (hoststack);
    job = PullStackByLocation (jobstack, STACK_TOP);
    if (job == NULL) {
      UnlockStack (hoststack);
      break;
    }
    host = (Host *) job[0].host;
    ASSERT (host, "host is NULL");

    RemoveStackByID (hoststack, host[0].HostID);
    UnlockStack (hoststack);

    KillJob (job, host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG && (Nobject > 0)) gprint (GP_ERR, "checked %d of %d jobs\n", i, Nobject);
  return (i);
}

int CheckRespHosts (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *hoststack;
  Stack *jobstack;
  Host *host;
  Job *job;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  hoststack = GetHostStack (PCONTROL_HOST_RESP);
  jobstack = GetJobStack (PCONTROL_JOB_RESP);
  Nobject = hoststack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    LockStack (jobstack);
    host = PullStackByLocation (hoststack, STACK_TOP);
    if (host == NULL) {
	UnlockStack (jobstack);
	break;
    }

    // if the host has a job, we need to pull the job from its stack
    job = (Job *) host[0].job;
    if (job != NULL) {
	RemoveStackByID (jobstack, job[0].JobID);
    }
    UnlockStack (jobstack);

    CheckRespHost (host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG) gprint (GP_ERR, "checked %d hosts\n", i);
  return (i);
}

int CheckDoneHosts (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *stack;
  Host  *host;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  stack = GetHostStack (PCONTROL_HOST_DONE);
  Nobject = stack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) break;
    CheckDoneHost (host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG) gprint (GP_ERR, "checked %d hosts\n", i);
  return (i);
}

int CheckDownHosts (float MaxDelay) {

  int i, Nobject;
  Stack *stack;
  Host  *host;
  struct timeval start, stop;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  stack = GetHostStack (PCONTROL_HOST_DOWN);
  Nobject = stack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) break;
    if (host[0].markoff) {
      // DOWN -> OFF
      host[0].markoff = FALSE;
      OffHost (host);
      return (TRUE);
    }
    dtime = DTIME (host[0].next_start_try, start);
    if (dtime > 0) {
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
    } else {
      // DOWN -> IDLE (maybe)
      // this is a race condition with "host retry", but the only 
      // consequence is that both StartHost and reset set the times to 0.0
      StartHost (host);
    }
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG) gprint (GP_ERR, "checked %d hosts\n", i);
  return (i);
}

// if we have any IDLE hosts, check if there are jobs to be launched 
// for each pass, we only check one type of job: stage = NEED, WANT, ANY, OLDWANT
int CheckIdleHosts (float MaxDelay, int Stage) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *stack;
  Host  *host;
  float dtime;

  /* check if there are any pending jobs */
  stack = GetJobStack (PCONTROL_JOB_PENDING);

  /* if there are no pending jobs and we are not in STAGE_NEEDHOST, skip test */
  if (!stack[0].Nobject && (Stage != PCONTROL_JOB_STAGE_NEEDHOST)) return (0);

  /* if there are no pending jobs, check for hosts that need to be reset */
  if (!stack[0].Nobject) {
    /* cycle through IDLE hosts */
    stack = GetHostStack (PCONTROL_HOST_IDLE);
    Nobject = stack[0].Nobject;
    for (i = 0; i < Nobject; i++) {
      host = PullStackByLocation (stack, STACK_TOP);
      if (host == NULL) break;
      if (CheckResetHost (host)) {
	return (1);
      }
      PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
    }
    return (0);
  }

  /* Loop through objects on the stack, no more than once. see note above */
  stack = GetHostStack (PCONTROL_HOST_IDLE);
  Nobject = stack[0].Nobject;

  /* always allow at least one test */
  gettimeofday (&start, (void *) NULL);
  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) break;
    CheckIdleHost (host, Stage);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }

  if (DEBUG) gprint (GP_ERR, "checked %d hosts\n", i);
  return (i);
}

/* this is just a heartbeat check (only IDLE hosts) */
int CheckLiveHosts (float MaxDelay) {

  struct timeval start, stop;
  int i, Nobject;
  Stack *stack;
  Host  *host;
  float dtime;

  /* Loop through objects on the stack, no more than once. see note above */
  stack = GetHostStack (PCONTROL_HOST_IDLE);
  Nobject = stack[0].Nobject;

  gettimeofday (&start, (void *) NULL);

  dtime = 0.0;
  for (i = 0; (i < Nobject) && (dtime < MaxDelay); i++) {
    host = PullStackByLocation (stack, STACK_TOP);
    if (host == NULL) break;
    CheckHost (host);
    gettimeofday (&stop, (void *) NULL);
    dtime = DTIME (stop, start);
  }
  if (DEBUG) gprint (GP_ERR, "checked %d idle hosts\n", i);
  return (TRUE);
}

/*

  gettimeofday (&stop, (void *) NULL);
  dtime = DTIME (stop, start);
  if (VerboseMode()) gprint (GP_ERR, "check 4: %f seconds\n", dtime);

  gettimeofday (&start, (void *) NULL);
*/

/** All of the CheckFooBar entries cycle though their respective queues, popping from the top and
    pushing to the bottom.  if we stop before the loop is done there is no tendancy for bias because
    we continue where we left off next round **/
