# include "pantasks.h"

int controller_pulse (int argc, char **argv) {

  int status;
  char command[1024];
  IOBuffer buffer;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: controller pulse (usec)\n");
    return (FALSE);
  }

  /* start controller connection (if needed) */
  if (!StartController ()) {
    gprint (GP_ERR, "failure to start pcontrol\n");
    return (FALSE);
  }

  sprintf (command, "pulse %d", atoi(argv[1]));
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);
  if (status) gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  FreeIOBuffer (&buffer);
  return (TRUE);
}
