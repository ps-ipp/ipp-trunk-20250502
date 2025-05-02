# include "pantasks.h"

int controller_parameters (int argc, char **argv) {

  int status;
  char command[1024];
  IOBuffer buffer;

  if (argc < 2) goto usage;
  if (argc > 4) goto usage;
  if (argc == 3) goto usage;
  if ((argc == 4) && strcmp(argv[2], "=")) goto usage;

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not active\n");
    return (TRUE);
  }

  // XXX this has an error?  test this out...
  if (argc == 2) {
    sprintf (command, "parameters %s", argv[1]);
  } else {
    sprintf (command, "parameters %s = %s", argv[1], argv[3]);
  }
  InitIOBuffer (&buffer, 0x100);

  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);

  if (status) {
    gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  } else {
    gprint (GP_LOG, "controller is not communicating\n");
  }
  FreeIOBuffer (&buffer);
  return (TRUE);

  usage:

  gprint (GP_LOG, "USAGE: controller parameters (param) [= value])\n");
  gprint (GP_LOG, "  valid parameters: connect_time, wanthost_wait, unwanted_host_jobs\n");
  gprint (GP_LOG, "  (minimum matching word is allowed)\n");
  gprint (GP_LOG, "  example: controller parameters connect = 2.0\n");
  return (FALSE);
}
