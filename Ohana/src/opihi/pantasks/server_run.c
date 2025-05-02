# include "pantasks.h"

int server_run (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: server run\n");
    return (FALSE);
  }

  CheckTasksSetState (TRUE);
  CheckJobsSetState (TRUE);
  CheckControllerSetState (TRUE);
  return (TRUE);
}

int server_stop (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: server stop\n");
    return (FALSE);
  }

  CheckTasksSetState (FALSE);
  return (TRUE);
}

int server_halt (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: server halt\n");
    return (FALSE);
  }

  CheckTasksSetState (FALSE);
  CheckJobsSetState (FALSE);
  CheckControllerSetState (FALSE);
  return (TRUE);
}
