# include "pantasks.h"

int controller_verbose (int argc, char **argv) {

  int status;
  char command[1024];
  IOBuffer buffer;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: controller verbose (level)\n");
    gprint (GP_ERR, "       (level) : off, on, toggle\n");
    return (FALSE);
  }

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not active\n");
    return (TRUE);
  }

  // XXX this has an error?  test this out...
  sprintf (command, "verbose %s", argv[1]);
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
