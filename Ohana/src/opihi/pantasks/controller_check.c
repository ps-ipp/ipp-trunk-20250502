# include "pantasks.h"

int controller_check (int argc, char **argv) {

  int status;
  char command[1024];
  IOBuffer buffer;

  if (argc != 3) goto usage;
  if (strcasecmp (argv[1], "JOB") && 
      strcasecmp (argv[1], "HOST")) goto usage;

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not running\n");
    return (TRUE);
  }

  sprintf (command, "check %s %s", argv[1], argv[2]);
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);
  if (VerboseMode()) {
    gprint (GP_LOG, "controller command sent\n");  
    gprint (GP_LOG, "\n Nbytes received: %d\n", buffer.Nbuffer);  
  }
  gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  FreeIOBuffer (&buffer);
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: controller check job (jobID)\n");
  gprint (GP_ERR, "USAGE: controller check host (hostID)\n");
  return (FALSE);
}
