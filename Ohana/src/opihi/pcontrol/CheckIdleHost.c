# include "pcontrol.h"

// The connection to the remote host is only allowed to live for MAX_CONNECT_TIME seconds.
// We disconnect and reconnect if a remote host has been connected for too long.  This is
// a (temporary?) work-around for the problem that the remote pclient job tends to grow
// too large over time.

/****
     
     queueing strategy:

     We have a problem when the queue contains many jobs of different processing times.  If we
     start with an equal number of jobs in two classes, fast and slow, eventually, we will end
     up with all hosts running the slow jobs and the fast jobs completely blocked.  If J1 takes
     T1 and J2 takes T2, and jobs have equal probability to land in a slot... ??

     It seems like we should boost the probability of the fast jobs over the slow jobs (of
     course, the end result of that will be all fast jobs draining and the slow jobs hanging
     around).  

     There is also a problem related to LAP, in that chip stage has a huge number of tasks in
     the queue (effectively infinite).  Make the probability of the job being selected
     proportional to something.  In any case, we need a user-controllable way to change the
     probability of selection

     As things are implemented below, the selection order depends on the queue order.  The
     easiest way to modify the probabilities is to re-sort based on something.

 ****/

static float MAX_WANTHOST_WAIT = 10.0;
static float MAX_CONNECT_TIME = 36000.0;

  /* if this host has been connected for too long, disconnect (will automatically reconnect) */
int CheckResetHost (Host *host) {

  struct timeval now;
  float dtime;

  /* if this host has been connected for too long, disconnect (will automatically reconnect) */
  gettimeofday (&now, (void *) NULL);
  dtime = DTIME (now, host[0].connect_time);
  if (dtime > MAX_CONNECT_TIME) {
      if (VerboseMode()) gprint (GP_ERR, "disconnect from %s\n", host[0].hostname);
      StopHost (host, PCONTROL_HOST_DOWN);
      return (TRUE);
  }
  return FALSE;
}

/* the supplied host is not on a stack: it cannot be taken by the other thread */
int CheckIdleHost (Host *host, int Stage) {

  int i;
  Stack *stack;
  Job *job;
  struct timeval now;
  float dtime;

  ASSERT (host, "host not set");

  /* if this host has been marked to be turned off, do that and return */
  if (host[0].markoff) {
    host[0].markoff = FALSE;
    StopHost (host, PCONTROL_HOST_OFF);
    return (TRUE);
  }
    
  /* check if host has been connected for too long */
  if (CheckResetHost (host)) {
    return (TRUE);
  }

  /* search the JOB_PENDING stack for an appropriate job */
  stack = GetJobStack (PCONTROL_JOB_PENDING);
  LockStack (stack);
  
  /* look for first NEEDHOST matching this host */
  for (i = 0; (Stage == PCONTROL_JOB_STAGE_NEEDHOST) && (i < stack[0].Nobject); i++) {
    job = (Job *) stack[0].object[i];
    if (job[0].mode != PCONTROL_JOB_NEEDHOST) continue;
    ASSERT (job[0].hostname != NULL, "NEEDHOST hostname missing");
    if (strcasecmp (job[0].hostname, host[0].hostname)) continue;

    if (!CheckMachineJobs (host, job)) continue;

    /* we have found an appropriate job; link it to the host and send to StartJob */
    job[0].host = (struct Host *) host;
    host[0].job = (struct Job *) job;

    // gprint (GP_ERR, "start needhost %s (job host %s) : %s\n", host[0].hostname, job[0].hostname, job[0].argv[0]);
    AddMachineJob (host, job);

    /* take the job off the stack and unlock the stack */
    RemoveStackEntry (stack, i);
    UnlockStack (stack);
    StartJob (job, host);
    return (TRUE);
  }

  /* no NEEDHOST entry, look for first WANTHOST matching this host */
  for (i = 0; (Stage == PCONTROL_JOB_STAGE_WANTHOST) && (i < stack[0].Nobject); i++) {
    job = (Job *) stack[0].object[i];
    if (job[0].mode != PCONTROL_JOB_WANTHOST) continue;
    ASSERT (job[0].hostname != NULL, "WANTHOST hostname missing");
    if (strcasecmp (job[0].hostname, host[0].hostname)) continue;

    if (!CheckMachineJobs (host, job)) continue;

    /* we have found an appropriate job; link it to the host and send to StartJob */
    job[0].host = (struct Host *) host;
    host[0].job = (struct Job *) job;

    // gprint (GP_ERR, "start wanthost %s (job host %s) : %s\n", host[0].hostname, job[0].hostname, job[0].argv[0]);
    AddMachineJob (host, job);

    /* take the job off the stack and unlock the stack */
    RemoveStackEntry (stack, i);
    UnlockStack (stack);
    StartJob (job, host);
    return (TRUE);
  }

  /* no WANTHOST entry, look for first ANYHOST matching this host */
  for (i = 0; (Stage == PCONTROL_JOB_STAGE_ANYHOST) && (i < stack[0].Nobject); i++) {
    job = (Job *) stack[0].object[i];
    if (job[0].mode != PCONTROL_JOB_ANYHOST) continue;

    if (!CheckMachineJobs (host, job)) continue;

    /* we have found an appropriate job; link it to the host and send to StartJob */
    job[0].host = (struct Host *) host;
    host[0].job = (struct Job *) job;

    // gprint (GP_ERR, "start  anyhost %s (job host %s) : %s\n", host[0].hostname, job[0].hostname, job[0].argv[0]);
    AddMachineJob (host, job);

    /* take the job off the stack and unlock the stack */
    RemoveStackEntry (stack, i);
    UnlockStack (stack);
    StartJob (job, host);
    return (TRUE);
  }

  /* no ANYHOST entry, look for first WANTHOST with old time */
  for (i = 0; (Stage == PCONTROL_JOB_STAGE_OLDWANT) && (i < stack[0].Nobject); i++) {
    job = (Job *) stack[0].object[i];
    if (job[0].mode != PCONTROL_JOB_WANTHOST) continue;

    // allow WANT jobs to wait up to 10.0 sec for the host to be free before giving up
    gettimeofday (&now, (void *) NULL);
    dtime = DTIME (now, job[0].start);
    if (dtime < MAX_WANTHOST_WAIT) continue;

    if (!CheckMachineJobs (host, job)) continue;

    gprint (GP_ERR, "start wanthost(2) %s (job host %s) : %s\n", host[0].hostname, job[0].hostname, job[0].argv[0]);
    AddMachineJob (host, job);

    /* we have found an appropriate job; link it to the host and send to StartJob */
    job[0].host = (struct Host *) host;
    host[0].job = (struct Job *) job;

    /* take the job off the stack and unlock the stack */
    RemoveStackEntry (stack, i);
    UnlockStack (stack);
    StartJob (job, host);
    return (TRUE);
  }
  UnlockStack (stack);

  /* no jobs for host, put it back on IDLE stack */
  PutHost (host, PCONTROL_HOST_IDLE, STACK_BOTTOM);
  return (TRUE);
}

void SetMaxWantHostWait (float value) {

  MAX_WANTHOST_WAIT = value;
  return;
}

float GetMaxWantHostWait (void) {

  return MAX_WANTHOST_WAIT;
}

void SetMaxConnectTime (float value) {

  MAX_CONNECT_TIME = value;
  return;
}

float GetMaxConnectTime (void) {

  return MAX_CONNECT_TIME;
}

/** note : host and job popped off IDLE and PENDING stacks, 
    unless no job is available **/
