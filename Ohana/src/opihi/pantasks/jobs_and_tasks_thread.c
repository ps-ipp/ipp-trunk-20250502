# include "pantasks.h"

void ResetTaskTimers ();

static int CheckTasksRun = FALSE;
static int CheckJobsRun = FALSE;

static int JobsAndTasksThreadRuns = TRUE;

void CheckJobsSetState (int state) {
  CheckJobsRun = state;
}
int CheckJobsGetState () {
  return (CheckJobsRun);
}

void CheckTasksSetState (int state) {
  if (state && !CheckTasksRun) {
    ResetTaskTimers ();
  }
  CheckTasksRun = state;
}
int CheckTasksGetState () {
  return (CheckTasksRun);
}

void QuitJobsAndTasksThread (void) {
  JobsAndTasksThreadRuns = FALSE;  
}

void *CheckJobsAndTasksThread (void *data) {
  OHANA_UNUSED_PARAM(data);

  char log_stdout[128], log_stderr[128];
  float job_timeout, task_timeout, next_timeout;

  gprintInit ();  // each thread needs to init the printing system

  // define server output log files
  if (VarConfig ("PANTASKS_SERVER_STDOUT", "%s", log_stdout) != NULL) {
      gprintSetFileThisThread (GP_LOG, log_stdout);
  }
  if (VarConfig ("PANTASKS_SERVER_STDERR", "%s", log_stderr) != NULL) {
      gprintSetFileThisThread (GP_ERR, log_stderr);
  }

  while (JobsAndTasksThreadRuns) {

    // one run of the task checker
    task_timeout = 0.25;
    if (CheckTasksRun) {
      task_timeout = CheckTasks ();
      if (VerboseMode() == 2) fprintf (stderr, "T");
    }

    // one run of the task checker
    job_timeout = 0.25;
    if (CheckJobsRun) {
      job_timeout = CheckJobs ();
      if (VerboseMode() == 2) fprintf (stderr, "J");
    }

    // job_timeout, task_timeout is time until next job,task is ready
    // sleep more-or-less that long (but no longer than 250msec)
    next_timeout = MIN (job_timeout, task_timeout);
    next_timeout = MIN (next_timeout, 0.25);
    if (next_timeout > 0.001) {
      usleep ((int)(1000000*next_timeout)); // allow other threads a chance to run
    }      
  }
  return NULL;
}

// I sleep for a small amount of time here, based on how long until the next expected
// timeout.  this enforces a certain granularity in the task creation, but prevents the task
// thread from driving the load up to silly levels.

// reset all of the task timers, with a bit of fuzz.  this is called 
// whenever we transition from 'stop' to 'run'
void ResetTaskTimers () {

  Task *task;
  struct timeval now;
  float fuzz;

  // get the current time
  gettimeofday (&now, NULL);

  // check all tasks
  while ((task = NextTask ()) != NULL) {
    task[0].last.tv_usec = now.tv_usec;
    task[0].last.tv_sec  = now.tv_sec;

    // add random offset between 0 and 10% of exec_period
    // XXX this should be optional
    fuzz = task[0].exec_period*drand48();
    task[0].last.tv_usec += 1e6*(fuzz - (int)fuzz);
    task[0].last.tv_sec += (int) fuzz;

    // gprint (GP_LOG, "fuzz: %f, last: %d %d\n", fuzz, task[0].last.tv_sec, task[0].last.tv_usec);
  }
  return;
}
