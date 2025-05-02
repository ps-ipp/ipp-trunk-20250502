# include "pantasks.h"

static Job **jobs;
static int   Njobs;
static int   NJOBS;

/* counter marking job being visited by the run loop */
static int   ActiveJob;

/* set up the jobs list */
void InitJobs () {
  NJOBS = 20;
  Njobs = 0;
  ALLOCATE (jobs, Job *, NJOBS);
  ActiveJob = -1;
}

/* free all jobs, only used on shutdown */
void FreeJobs () {
  int i;
  for (i = 0; i < Njobs; i++) {
    FreeJob (jobs[i]);
  }
  free (jobs);
}

/* provide a mechanism to loop over the list of jobs */
Job *NextJob () {
  
  Job *job;

  ActiveJob ++;
  if (ActiveJob < 0) ActiveJob = 0;
  if (ActiveJob >= Njobs) {
    ActiveJob = -1;
    return (NULL);
  }
  job = jobs[ActiveJob];
  return (job);
}

/* return job with given ID */
Job *FindJob (IDtype JobID) {

  int i;

  /* return job with matching JobID */
  for (i = 0; i < Njobs; i++) {
    if (jobs[i][0].JobID == JobID) {
      return (jobs[i]);
    }
  }
  return (NULL);
}  

/* return job with given controller Job ID */
Job *FindControllerJob (IDtype JobID) {

  int i;

  /* return job with matching JobID */
  for (i = 0; i < Njobs; i++) {
    if (jobs[i][0].mode == JOB_LOCAL) continue;
    if (jobs[i][0].pid  == JobID) {
      return (jobs[i]);
    }
  }
  return (NULL);
}  

/* list known jobs */
void ListJobs () {

  int i;

  gprint (GP_LOG, "\n");
  if (Njobs == 0) {
    gprint (GP_LOG, " no defined jobs\n");
    return;
  }

  gprint (GP_LOG, " Jobs in Pantasks Queue\n");
  for (i = 0; i < Njobs; i++) {
    gprint (GP_LOG, " %4d  %8d: %-25s %10s %20s\n", Njobs, jobs[i][0].JobID, jobs[i][0].task[0].name, JobStateToString(jobs[i][0].state), jobs[i][0].argv[0]);
  }

  return;
}

/* make a new job from a task */
Job *CreateJob (Task *task) {
  
  int i;
  Job *job;
  
  ALLOCATE (job, Job, 1);

  job[0].JobID = NextJobID ();

  // this should not happen; abort?
  if (job[0].JobID < 0) {
    free (job);
    return NULL;
  }

  job[0].pid = 0;
  job[0].mode = JOB_LOCAL;
  if (task[0].host != NULL) {
    job[0].mode = JOB_CONTROLLER;
  }
  job[0].nicelevel = task[0].nicelevel;

  /* we need our own copy of task[0].argv argc is the number of valid args, like the usual command line.  we
   *  allocate one extra element, with value 0 to be passed to execvp
   */
  job[0].argc = task[0].argc;
  ALLOCATE (job[0].argv, char *, MAX (task[0].argc + 1, 1));
  for (i = 0; i < task[0].argc; i++) {
    job[0].argv[i] = strcreate (task[0].argv[i]);
  }
  job[0].argv[i] = 0;

  /* we need our own copy of task[0].optv: optc is the number of valid opts.  */
  job[0].optc = task[0].optc;
  ALLOCATE (job[0].optv, char *, MAX (task[0].optc, 1));
  for (i = 0; i < job[0].optc; i++) {
    job[0].optv[i] = strcreate (task[0].optv[i]);
  }

  /* Other data from the task is needed by the job. We carry a pointer back to the task.  Changes to an
     executing task are applied to the existing jobs (exit macros, poll_period, timeout) */

  job[0].task = task;
  job[0].realhost = NULL;
  
  /* if we decide we need to be able to dynamically set task qualities (like host, timeouts, etc), the we will
     need to have matched entries to these quantites in the job structure */

  InitIOBuffer (&job[0].stdout_buff, 0x100);
  InitIOBuffer (&job[0].stderr_buff, 0x100);

  job[0].stdout_dump = NULL;
  job[0].stderr_dump = NULL;
  if (task[0].stdout_dump != NULL) job[0].stdout_dump = strcreate (task[0].stdout_dump);
  if (task[0].stderr_dump != NULL) job[0].stderr_dump = strcreate (task[0].stderr_dump);

  job[0].stdout_fd = -1;
  job[0].stderr_fd = -1;

  jobs[Njobs] = job;
  Njobs ++;
  if (Njobs == NJOBS) {
    NJOBS += 20;
    REALLOCATE (jobs, Job *, NJOBS);
  }

  /* increment job counters */
  task[0].Npending ++;

  return (jobs[Njobs-1]);
}

void FreeJob (Job *job) {
  
  int i;

  if (job == NULL) return;
  
  for (i = 0; i < job[0].argc; i++) {
    free (job[0].argv[i]);
  }
  free (job[0].argv);

  for (i = 0; i < job[0].optc; i++) {
    free (job[0].optv[i]);
  }
  free (job[0].optv);

  if (job[0].stdout_fd >= 0) close (job[0].stdout_fd);
  if (job[0].stderr_fd >= 0) close (job[0].stderr_fd);

  if (job[0].stdout_dump != NULL) free (job[0].stdout_dump);
  if (job[0].stderr_dump != NULL) free (job[0].stderr_dump);
  if (job[0].realhost != NULL) free (job[0].realhost);

  FreeIOBuffer (&job[0].stdout_buff);
  FreeIOBuffer (&job[0].stderr_buff);
  free (job);
  return;
}

/* delete the job from the job list & adjust ActiveJob counter */
int DeleteJob (Job *job) {

  int i, Nm;

  Nm = -1;
  for (i = 0; i < Njobs; i++) {
    if (job == jobs[i]) {
      Nm = i;
      break;
    }
  }
  if (Nm == -1) {
    gprint (GP_ERR, "programming error: job not found\n");
    return (FALSE);
  }

  Task *task = job->task;
  FreeJob (jobs[Nm]);
  for (i = Nm; i < Njobs - 1; i++) {
    jobs[i] = jobs[i + 1];
  }
  Njobs --;

  /* adjust active job number */
  if (ActiveJob >= Nm) {
    ActiveJob --;
  }

  task->Npending --;
  return (TRUE);
}

int FlushJobs () {

  int i;

  // kill outstanding jobs
  for (i = 0; i < Njobs; i++) {
    if (jobs[i][0].mode == JOB_LOCAL) {
      if (!KillLocalJob (jobs[i])) {
	jobs[i][0].state = JOB_HUNG;
	if (VerboseMode()) gprint (GP_LOG, "child process %d is hung, cannot kill\n", jobs[i][0].pid);
      }
    } else {
      if (!KillControllerJob (jobs[i])) {
	jobs[i][0].state = JOB_HUNG;
	if (VerboseMode()) gprint (GP_LOG, "child process %d is hung, cannot kill\n", jobs[i][0].pid);
      }
    }    
    jobs[i][0].task[0].Npending = 0;
    FreeJob (jobs[i]);
  }
    
  Njobs = 0;
  NJOBS = 20;
  REALLOCATE (jobs, Job *, NJOBS);
  ActiveJob = -1;
  return (TRUE);
}

int SubmitJob (Job *job) {

  int status;

  if (job[0].mode == JOB_LOCAL) {
    if (DEBUG) fprintf (stderr, "submit job: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
    status = SubmitLocalJob (job);
  } else {
    status = SubmitControllerJob (job);
  }
  if (!status) {
    return FALSE;
  }

  /* reset clock for start and poll-test */
  gettimeofday (&job[0].start, (void *) NULL);
  job[0].last = job[0].start;
  job[0].state = JOB_PENDING;
  return (TRUE);
}

int CheckJob (Job *job) {

  /* add checks for timeouts */

  if (job[0].mode == JOB_LOCAL) {
    CheckLocalJob (job);
    /* controller jobs are checked en masse by CheckController */
  }
  return (job[0].state);
}

static char *JobStateNone    = "NONE";
static char *JobStateBusy    = "BUSY";
static char *JobStateExit    = "EXIT";
static char *JobStateHung    = "HUNG";
static char *JobStateCrash   = "CRASH";
static char *JobStatePending = "PENDING";

char *JobStateToString (JobStat state) {

  switch (state) {
    case JOB_NONE:
      return JobStateNone;
    case JOB_BUSY:
      return JobStateBusy;
    case JOB_EXIT:
      return JobStateExit;
    case JOB_HUNG:
      return JobStateHung;
    case JOB_CRASH:
      return JobStateCrash;
    case JOB_PENDING:
      return JobStatePending;
    default:
      return JobStateNone;
  }      
  return JobStateNone;
}

