# include "pantasks.h"

/** things related to CheckTasks **/
void ResetTaskTimers ();

static int CheckTasksRun = FALSE;

void CheckTasksSetState (int state) {
  if (state && !CheckTasksRun) {
    ResetTaskTimers ();
  }
  CheckTasksRun = state;
}
int CheckTasksGetState () {
  return (CheckTasksRun);
}

void *CheckTasksThread (void *data) {

  float next_timeout;
  char log_stdout[128], log_stderr[128];

  gprintInit ();  // each thread needs to init the printing system

  // define server output log files
  if (VarConfig ("PANTASKS_SERVER_STDOUT", "%s", log_stdout) != NULL) {
      gprintSetFileThisThread (GP_LOG, log_stdout);
  }
  if (VarConfig ("PANTASKS_SERVER_STDERR", "%s", log_stderr) != NULL) {
      gprintSetFileThisThread (GP_ERR, log_stderr);
  }

  while (1) {

    // check for thread suspend
    if (!CheckTasksRun) {
      usleep (100000); // idle if thread action is suspended
      continue;
    }

    // one run of the task checker
    SerialThreadLock ();
    next_timeout = CheckTasks ();
    SerialThreadUnlock ();
    if (VerboseMode() == 2) fprintf (stderr, "T");

    next_timeout = MIN (next_timeout, 0.1);
    if (next_timeout > 0.001) {
      usleep ((int)(1000000*next_timeout)); // allow other threads a chance to run
    }      
  }
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
