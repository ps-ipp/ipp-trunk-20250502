# include "basic.h"

int which (int argc, char **argv) {

  Command *cmd;
 
  if (argc != 2) {
    gprint (GP_ERR, "USAGE: which <filename>\n");
    return (FALSE);
  }

  cmd = MatchCommand (argv[1], TRUE, TRUE);
  if (cmd == NULL) return (FALSE);

  gprint (GP_ERR, "%-25s -- %s\n", cmd[0].name, cmd[0].help);
  return (TRUE);

}
