# include "pantasks.h"

int controller_version (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  int status;
  char command[1024];
  IOBuffer buffer;

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: controller version\n");
    return (FALSE);
  }

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not active\n");
    return (TRUE);
  }


  sprintf (command, "version");
  InitIOBuffer (&buffer, 0x100);

  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);

  if (status) {
    gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  } else {
    gprint (GP_LOG, "controller is not communicating\n");
  }
  FreeIOBuffer (&buffer);
  return (TRUE);
}
