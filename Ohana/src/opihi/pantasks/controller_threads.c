# include "pantasks.h"

/** things related to CheckController **/

static int CheckControllerRun = FALSE;

static int ControllerThreadRuns = TRUE;

void CheckControllerSetState (int state) {
  CheckControllerRun = state;
}
int CheckControllerGetState () {
  return (CheckControllerRun);
}

void QuitControllerThread (void) {
  ControllerThreadRuns = FALSE;
}

void *CheckControllerThread (void *data) {
  OHANA_UNUSED_PARAM(data);

  char log_stdout[128], log_stderr[128];

  gprintInit ();  // each thread needs to init the printing system
  // define server output log files
  if (VarConfig ("PANTASKS_SERVER_STDOUT", "%s", log_stdout) != NULL) {
      gprintSetFileThisThread (GP_LOG, log_stdout);
  }
  if (VarConfig ("PANTASKS_SERVER_STDERR", "%s", log_stderr) != NULL) {
      gprintSetFileThisThread (GP_ERR, log_stderr);
  }

  while (ControllerThreadRuns) {

    // check for thread suspend
    if (!CheckControllerRun) {
      usleep (100000); // idle if thread action is suspended
      continue;
    }

    // one run of the task checker
    ControlLock(__func__);
    CheckController ();
    ControlUnlock(__func__);

    ControlLock(__func__);
    CheckControllerOutput ();
    ControlUnlock(__func__);

    if (VerboseMode() == 2) fprintf (stderr, "C");
    // fprintf (stderr, "**** C ****");
    usleep (500000); // allow other threads a chance to run
  }
  return NULL;
}
