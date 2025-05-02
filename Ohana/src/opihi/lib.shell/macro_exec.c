# include "opihi.h"

int macro_exec (int argc, char **argv) {

  int i, status, MacroDepth;
  char *PreviousName, **params, tmp[1024];
  Macro *macro;
  
  macro = MatchMacro (argv[0], FALSE, TRUE);
  if (macro == NULL) {
    gprint (GP_ERR, "%s: Command not found.\n", argv[0]);
    return (FALSE);
  }

  /* increase MacroDepth by one - governs macro args */
  PreviousName = GetMacroName ();
  MacroDepth = GetMacroDepth ();
  MacroDepth ++;
  SetCurrentMacroData (macro[0].name, MacroDepth);

  ALLOCATE (params, char *, argc);
  sprintf (tmp, "%d.%d", MacroDepth, 0);
  params[0] = strcreate (tmp);
  sprintf (tmp, "%d", argc);
  set_str_variable (params[0], tmp);
  for (i = 1; i < argc; i++) {
    sprintf (tmp, "%d.%d", MacroDepth, i);
    params[i] = strcreate (tmp);
    set_str_variable (params[i], argv[i]);
  }

  /* process this list */
  status = exec_loop (&macro[0]);
  loop_last = loop_next = FALSE; 
  /* 'last' and 'next' should only affect one loop */

  /* clear out the command line variables */
  for (i = 0; i < argc; i++) {
    DeleteNamedScalar (params[i]);
    free (params[i]);
  }
  free (params);
  
  MacroDepth --;
  SetCurrentMacroData (PreviousName, MacroDepth);
  return (status);
}
