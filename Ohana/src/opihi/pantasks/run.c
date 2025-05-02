# include "pantasks.h"

// XXX for client/server, we need to simply start or stop the
// appropriate threads
// with one thread for each of the major actions, this would
// make it easy to keep the controller running and stop the 
// scheduler (don't run CheckTasks, but run everything else 
// until nothing is left...

int run (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: run\n");
    return (FALSE);
  }

  CheckTasksSetState (TRUE);
  CheckControllerSetState (TRUE);
  CheckJobsSetState (TRUE);

  // InitTaskTimers ();
  // rl_event_hook = CheckSystem;

  return (TRUE);
}
