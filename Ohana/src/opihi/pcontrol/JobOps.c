# include "pcontrol.h"

Stack *JobPool_AllJobs;  // virtual pool for user status queries

Stack *JobPool_Pending;
Stack *JobPool_Busy;
Stack *JobPool_Resp;
Stack *JobPool_Done;
Stack *JobPool_Kill;
Stack *JobPool_Exit;
Stack *JobPool_Crash;

void InitJobStacks () {
  JobPool_AllJobs = InitStack ();

  JobPool_Pending = InitStack ();
  JobPool_Busy    = InitStack ();
  JobPool_Resp    = InitStack ();
  JobPool_Done    = InitStack ();
  JobPool_Kill    = InitStack ();
  JobPool_Exit    = InitStack ();
  JobPool_Crash   = InitStack ();
}

void FreeJobStack (Stack *stack) {
  Job *job;
  while ((job = PullStackByLocation (stack, stack[0].Nobject - 1)) != NULL) {
    DelJob (job);
  }
  FreeStack (stack);
}

void FreeJobStacks () {
  FreeJobStack (JobPool_Pending);
  FreeJobStack (JobPool_Busy   );
  FreeJobStack (JobPool_Resp   );
  FreeJobStack (JobPool_Done   );
  FreeJobStack (JobPool_Kill   );
  FreeJobStack (JobPool_Exit   );
  FreeJobStack (JobPool_Crash  );

  // AllJobs is a virtual stack : all jobs are references
  FreeStack (JobPool_AllJobs);
}

char *GetJobStackName (int StackID) {
  switch (StackID) {
    case PCONTROL_JOB_ALLJOBS: return ("ALLJOBS");

    case PCONTROL_JOB_PENDING: return ("PENDING");
    case PCONTROL_JOB_BUSY:    return ("BUSY");
    case PCONTROL_JOB_RESP:    return ("RESP");
    case PCONTROL_JOB_DONE:    return ("DONE");
    case PCONTROL_JOB_KILL:    return ("KILL");
    case PCONTROL_JOB_EXIT:    return ("EXIT");
    case PCONTROL_JOB_CRASH:   return ("CRASH");
  }
  gprint (GP_ERR, "error: unknown host stack : programming error\n");
  pcontrol_exit (53);
  return (NULL);
}

int GetJobStackIDbyName (char *name) {
  if (!strcasecmp (name, "ALLJOBS"))  return PCONTROL_JOB_ALLJOBS;
  if (!strcasecmp (name, "PENDING"))  return PCONTROL_JOB_PENDING;
  if (!strcasecmp (name, "BUSY"))     return PCONTROL_JOB_BUSY   ;
  if (!strcasecmp (name, "RESP"))     return PCONTROL_JOB_RESP   ;
  if (!strcasecmp (name, "DONE"))     return PCONTROL_JOB_DONE   ;
  if (!strcasecmp (name, "KILL"))     return PCONTROL_JOB_KILL   ;
  if (!strcasecmp (name, "EXIT"))     return PCONTROL_JOB_EXIT   ;
  if (!strcasecmp (name, "CRASH"))    return PCONTROL_JOB_CRASH  ;
  return (PCONTROL_JOB_NONE);
}

Stack *GetJobStack (int StackID) {
  switch (StackID) {
    case PCONTROL_JOB_ALLJOBS: return (JobPool_AllJobs);

    case PCONTROL_JOB_PENDING: return (JobPool_Pending);
    case PCONTROL_JOB_BUSY:    return (JobPool_Busy);
    case PCONTROL_JOB_RESP:    return (JobPool_Resp);
    case PCONTROL_JOB_DONE:    return (JobPool_Done);
    case PCONTROL_JOB_KILL:    return (JobPool_Kill);
    case PCONTROL_JOB_EXIT:    return (JobPool_Exit);
    case PCONTROL_JOB_CRASH:   return (JobPool_Crash);
  }
  gprint (GP_ERR, "error: unknown job stack : programming error\n");
  pcontrol_exit (54);
  return (NULL);
}

Stack *GetJobStackByName (char *name) {

  if (!strcasecmp (name, "all"))     return (JobPool_AllJobs);

  if (!strcasecmp (name, "pending")) return (JobPool_Pending);
  if (!strcasecmp (name, "busy"))    return (JobPool_Busy);
  if (!strcasecmp (name, "resp"))    return (JobPool_Resp);
  if (!strcasecmp (name, "done"))    return (JobPool_Done);
  if (!strcasecmp (name, "exit"))    return (JobPool_Exit);
  if (!strcasecmp (name, "crash"))   return (JobPool_Crash);
  return (NULL);
}

/* add job to position in stack, use StackID as default state */
int PutJob (Job *job, int StackID, int where) {

  int stat;
  Stack *stack;

  // fprintf (stderr, "move job %s to %s\n", job[0].argv[0], GetJobStackName(StackID));

  stack = GetJobStack (StackID);
  if (stack == NULL) return (FALSE);

  /* by default, these are both the same - to override, use PutJobSetState */
  job[0].state = StackID;
  job[0].stack = StackID;
  stat = PushStack (stack, where, job, job[0].JobID, job[0].argv[0]);
  // XXX need to handle the error conditions, or we drop the host & leak memory
  return (stat);
}
  
/* add job to position in stack.  set state to 'state' */
int PutJobSetState (Job *job, int StackID, int where, int state) {

  int stat;
  Stack *stack;

  stack = GetJobStack (StackID);
  if (stack == NULL) return (FALSE);

  /* alternate state specified by user */
  job[0].state = state;
  job[0].stack = StackID;
  stat = PushStack (stack, where, job, job[0].JobID, job[0].argv[0]);
  // XXX need to handle the error conditions, or we drop the host & leak memory
  return (stat);
}
  
Job *PullJobByID (IDtype JobID, int *StackID) {

  Job *job;

  *StackID = PCONTROL_JOB_PENDING;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_BUSY;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_RESP;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_EXIT;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_CRASH;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_DONE;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  *StackID = PCONTROL_JOB_KILL;
  job = PullJobFromStackByID (*StackID, JobID);
  if (job != NULL) return (job);

  return (NULL);
}

/* remove job from position in stack */
Job *PullJobFromStackByID (int StackID, int ID) {

  Job *job;
  Stack *stack;

  stack = GetJobStack (StackID);
  if (stack == NULL) return (NULL);

  job = PullStackByID (stack, ID);
  return (job);
}

int InitJobOutput (JobOutput *output) {

  output[0].size = 0;
  output[0].requested = FALSE;
  output[0].completed = FALSE;
  InitIOBuffer (&output[0].buffer, 0x1000);

  return TRUE;
}

int ResetJobOutput (JobOutput *output) {

  output[0].size = 0;
  output[0].requested = FALSE;
  output[0].completed = FALSE;
  FlushIOBuffer (&output[0].buffer);

  return TRUE;
}

IDtype AddJob (char *hostname, JobMode mode, int timeout, int nicelevel, int argc, char **argv, int Nxhosts, char **xhosts) {
  OHANA_UNUSED_PARAM(timeout);

  int JobID;
  Job *job;

  ALLOCATE (job, Job, 1);

  job[0].JobID    = NextJobID();
  // XXX this test does not make sense
  // if (job[0].JobID < 0) {
  //   free (job);
  //   return -1;
  // }

  job[0].argc     = argc;
  job[0].argv     = argv;
  job[0].hostname = hostname;
  job[0].realhost = NULL;

  job[0].exit_status = 0;
  job[0].Reset    = FALSE;

  InitJobOutput (&job[0].stdout_buf);
  InitJobOutput (&job[0].stderr_buf);

  job[0].mode     = mode;
  job[0].nicelevel = nicelevel;

  job[0].state = 0;
  job[0].stack = 0;

  job[0].dtime = 0.0;
  job[0].pid = 0;
  job[0].host     = NULL;

  job[0].xhosts = xhosts;
  job[0].Nxhosts = Nxhosts;

  JobID = job[0].JobID;

  // Put a copy of all created jobs on the ALLJOBS stack
  // This is a virtual stack: do not free the job from this stack
  PutJob (job, PCONTROL_JOB_ALLJOBS, STACK_BOTTOM);
  PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);

  // until the job is launched, we use 'start' to time how long the job is waiting on the queue
  gettimeofday (&job[0].start, (void *) NULL);

  if (VerboseMode()) gprint (GP_ERR, "added new job\n");
  return (JobID);
}

void DelJob (Job *job) {

  int i;

  Job *copy;

  copy = PullStackByID (JobPool_AllJobs, job[0].JobID);
  ASSERT (copy == job, "programming error: ALLJOBS entry does not match");

  FREE (job[0].hostname);
  for (i = 0; i < job[0].argc; i++) {
    FREE (job[0].argv[i]);
  }
  FREE (job[0].argv);

  for (i = 0; i < job[0].Nxhosts; i++) {
    FREE (job[0].xhosts[i]);
  }
  FREE (job[0].xhosts);

  FreeIOBuffer (&job[0].stdout_buf.buffer);
  FreeIOBuffer (&job[0].stderr_buf.buffer);

  FREE (job);
}

/*** ResortJobStack can be used to adjust priorities based on some info we supply.  This
     is not finished -- to finish this, I need to define the metric of interest and
     arrange for that to be passed to the jobs in pcontrol

void ResortJobStack (int StackID) {

  Stack *stack = GetJobStack (StackID);
  LockStack (stack);

# define SWAPFUNC(A,B){				\
    void *tmpObject = stack[0].object[A];	\
    stack[0].object[A] = stack[0].object[B];	\
    stack[0].object[B] = tmpObject;		\
    void *tmpName = stack[0].name[A];		\
    stack[0].name[A] = stack[0].name[B];	\
    stack[0].name[B] = tmpName;			\
    void *tmpID = stack[0].id[A];		\
    stack[0].id[A] = stack[0].id[B];		\
    stack[0].id[B] = tmpID;			\
  }

# define COMPARE(A,B)(((Job *)stack[0].object[A]).VALUE < ((Job *)stack[0].object[B]).VALUE)

  OHANA_SORT (stack[0].Nobject, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

  UnlockStack (stack);
}

***/
