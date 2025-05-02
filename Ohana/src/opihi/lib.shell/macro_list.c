# include "opihi.h"

int macro_list_f (int argc, char **argv) {

  int i;
  Macro *macro;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: macro list <macro>\n");
    return (FALSE);
  }

  macro = MatchMacro (argv[1], FALSE, TRUE);
  if (macro == NULL) {
    gprint (GP_ERR, "%s: Macro not found\n", argv[1]);
    return (FALSE);
  }

  for (i = 0; i < macro[0].Nlines; i++) {
    gprint (GP_ERR, "%s\n", macro[0].line[i]);
  }
  return (TRUE);
}
