# include "pantasks.h"

int controller_host (int argc, char **argv) {

  int N, status, max_threads;
  char command[1024];
  IOBuffer buffer;

  max_threads = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    max_threads = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) goto usage;
  if (max_threads && strcasecmp (argv[1], "ADD")) goto usage;

  /* start controller connection (if needed) */
  if (!StartController ()) {
    gprint (GP_ERR, "failure to start pcontrol\n");
    return (FALSE);
  }

  // the user may issue any of these commands:
  // ADD, ON, RETRY, CHECK, OFF, DELETE
  // we need to catch ADD and DELETE and modify our host table accordingly
  if (!strcasecmp (argv[1], "ADD")) {
    AddHost (argv[2], max_threads);
  } 

  if (!strcasecmp (argv[1], "DELETE")) {
    DeleteHost (argv[2]);
  }

  if (max_threads) {
    sprintf (command, "host %s %s -threads %d", argv[1], argv[2], max_threads);
  } else {
    sprintf (command, "host %s %s", argv[1], argv[2]);
  }
  InitIOBuffer (&buffer, 0x100);

  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);

  if (status) gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);

  FreeIOBuffer (&buffer);
  return (TRUE);
  
usage:
  gprint (GP_LOG, "USAGE: controller host (command) (hostname)\n");
  gprint (GP_ERR, "  valid commands: add, on, retry, check, off, delete\n");
  gprint (GP_ERR, "  -threads Nthreads is optional for 'add'\n");
  return (FALSE);
}
