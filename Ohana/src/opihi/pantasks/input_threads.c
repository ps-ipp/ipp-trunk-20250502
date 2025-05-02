# include "pantasks.h"

/** things related to CheckInputs **/

static int CheckInputsRun = TRUE;

void CheckInputsSetState (int state) {
  CheckInputsRun = state;
}
int CheckInputsGetState () {
  return (CheckInputsRun);
}

void *CheckInputsThread (void *data) {

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
    if (!CheckInputsRun) {
      usleep (100000); // idle if thread action is suspended
      continue;
    }

    // one run of the task checker
    SerialThreadLock ();
    CheckInputs ();
    SerialThreadUnlock ();
    if (VerboseMode() == 2) fprintf (stderr, "I");
    usleep (10000); // allow other threads a chance to run
  }
}
