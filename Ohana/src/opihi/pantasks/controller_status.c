# include "pantasks.h"

int controller_status (int argc, char **argv) {

  int i, status;
  char command[1024], tmpline[1024];
  IOBuffer buffer;

  if ((argc > 1) && (!strcasecmp(argv[1], "help")))  {
    gprint (GP_ERR, "USAGE: controller status [options]\n");
    gprint (GP_ERR, "  OPTIONS:\n");
    gprint (GP_ERR, "  -cmd command\n");
    gprint (GP_ERR, "  -host hostname\n");
    gprint (GP_ERR, "  -state state\n");
    gprint (GP_ERR, "  -age seconds\n");
    gprint (GP_ERR, "  +jobs [-nohost]\n");
    gprint (GP_ERR, "  +host [-nojobs]\n");
    return (FALSE);
  }

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    gprint (GP_LOG, "controller is not active\n");
    return (TRUE);
  }

  snprintf_nowarn (command, 1024, "status");
  for (i = 1; i < argc; i++) {
    snprintf_nowarn (tmpline, 1024, "%s %s", command, argv[i]);
    strcpy (command, tmpline);
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
}
