# include "pantasks.h"

/** things related to CheckJobs **/

static int CheckJobsRun = FALSE;

void CheckJobsSetState (int state) {
  CheckJobsRun = state;
}
int CheckJobsGetState () {
  return (CheckJobsRun);
}

void *CheckJobsThread (void *data) {

  char log_stdout[128], log_stderr[128];
  float next_timeout;

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
    if (!CheckJobsRun) {
      usleep (100000); // idle if thread action is suspended
      continue;
    }

    // one run of the task checker
    SerialThreadLock ();
    next_timeout = CheckJobs ();
    SerialThreadUnlock ();
    if (VerboseMode() == 2) fprintf (stderr, "J");

    next_timeout = MIN (next_timeout, 0.1);
    if (next_timeout > 0.001) {
      usleep ((int)(1000000*next_timeout)); // allow other threads a chance to run
    }      
  }
}
