# include "pantasks.h"

int controller_run (int argc, char **argv) {

  int status;
  char command[1024];
  IOBuffer buffer;

  if ((argc > 2) ||
      get_argument (argc, argv, "help") ||
      get_argument (argc, argv, "-h") ||
      get_argument (argc, argv, "--help")) 
  {
    if (!strcmp (argv[0], "run")) {
      gprint (GP_ERR, "USAGE: controller run [level]\n");
      gprint (GP_ERR, "  allowed levels:\n");
      gprint (GP_ERR, "  all (default) : manage machines, spawn jobs, harvest jobs\n");
      gprint (GP_ERR, "  reap          : manage machines, harvest jobs\n");
      gprint (GP_ERR, "  hosts         : manage machines, not jobs\n");
      gprint (GP_ERR, "  none          : all stop\n");
    } else {
      gprint (GP_ERR, "USAGE: controller stop (immediate processing halt)\n");
    }
    return (FALSE);
  }

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not active\n");
    return (TRUE);
  }

  if (argc == 1) {
    strcpy (command, argv[0]);
  } else {
    sprintf (command, "run %s", argv[1]);
  }

  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);
  if (status) {
    gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  } else {
    gprint (GP_LOG, "controller is not responding\n");
  }
  FreeIOBuffer (&buffer);
  return (TRUE);
}
