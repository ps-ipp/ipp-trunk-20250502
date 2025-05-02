# include "opihi.h"

int macro_delete (int argc, char **argv) {

  Command *cmd;
  Macro *macro;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: macro delete <macro>\n");
    return (FALSE);
  }

  macro = MatchMacro (argv[1], FALSE, TRUE);
  if (macro == NULL) {
    gprint (GP_ERR, "Macro %s not found\n", argv[1]);
    return (FALSE);
  }
  cmd = MatchCommand (argv[1], FALSE, TRUE);
  if (cmd == NULL) {
    gprint (GP_ERR, "programming error: macro exists but not command\n");
    return (FALSE);
  }

  DeleteMacro (macro);
  DeleteCommand (cmd);
  return (TRUE);
}

