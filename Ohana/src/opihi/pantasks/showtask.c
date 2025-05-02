# include "pantasks.h"

int showtask (int argc, char **argv) {

  if (argc != 2) {
    gprint (GP_LOG, "USAGE: showtask (task)\n");
    return (FALSE);
  }

  JobTaskLock();
  ShowTask (argv[1]);
  JobTaskUnlock();

  return (TRUE);
}
