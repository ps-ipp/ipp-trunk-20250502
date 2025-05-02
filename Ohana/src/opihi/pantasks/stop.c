# include "pantasks.h"

int stop (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: stop\n");
    return (FALSE);
  }

  CheckTasksSetState (FALSE);
  // CheckControllerSetState (FALSE);
  // CheckJobsSetState (FALSE);

  return (TRUE);
}

int halt (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: halt\n");
    return (FALSE);
  }

  CheckTasksSetState (FALSE);
  CheckControllerSetState (FALSE);
  CheckJobsSetState (FALSE);

  return (TRUE);
}
