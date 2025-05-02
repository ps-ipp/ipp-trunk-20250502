# include "basic.h"
# define NLINE 1024

enum {MODE_NONE, MODE_BOOL, MODE_VALUE};

int getargs (int argc, char **argv) {

  int N;
  char templine[NLINE];

  // variable name to save the value
  char *varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: getargs (key) (mode) [-var out]\n");
    gprint (GP_ERR, "  find argument key and remove from the arglist, optionally saving value on variable\n");
    return (FALSE);
  }

  int mode = MODE_NONE;
  if (!strcasecmp (argv[2], "bool"))    mode = MODE_BOOL;
  if (!strcasecmp (argv[2], "boolean")) mode = MODE_BOOL;
  if (!strcasecmp (argv[2], "val"))     mode = MODE_VALUE;
  if (!strcasecmp (argv[2], "value"))   mode = MODE_VALUE;
  if (mode == MODE_NONE) { gprint (GP_ERR, "ERROR: invalid mode %s\n", argv[2]); return (FALSE); }
  
  // get macro name and depth
  int   MacroDepth = GetMacroDepth ();

  // real name of $0 (number of arguments)
  snprintf (templine, NLINE, "%d.%d", MacroDepth, 0);
  char *p = get_variable_ptr (templine);
  myAssert (p, "coding error");

  int argnum = atoi (p);

  // look for the requested key (only selects the first example)
  int argmatch = 0;
  for (int i = 1; (i < argnum) && !argmatch; i++) {
    // real name of $i (number of arguments)
    snprintf (templine, NLINE, "%d.%d", MacroDepth, i);
    char *p = get_variable_ptr (templine);
    myAssert (p, "coding error");
    if (strcmp (argv[1], p)) continue;
    argmatch = i;
  }
  
  if (!argmatch) {
    if (varName) {
      // default for a boolean is FALSE
      // default for a value is to keep an existing value
      if (mode == MODE_BOOL) {
	set_str_variable (varName, "0"); // NOTE: if a local variable with the same name exists, that will be used
      }
    }
    return TRUE;
  }

  if (mode == MODE_VALUE) {
    if (argnum < argmatch + 2) { gprint (GP_ERR, "ERROR: missing value for argument %s\n", argv[1]); return (FALSE); }
    snprintf (templine, NLINE, "%d.%d", MacroDepth, argmatch + 1);
    char *p = get_variable_ptr (templine);
    myAssert (p, "coding error");

    if (varName) set_str_variable (varName, p); // NOTE: if a local variable with the same name exists, that will be used

    for (int i = argmatch + 2; i < argnum; i++) {
      // move $i+2 to $i
      snprintf (templine, NLINE, "%d.%d", MacroDepth, i);
      char *p = get_variable_ptr (templine);
      myAssert (p, "coding error");

      snprintf (templine, NLINE, "%d.%d", MacroDepth, i - 2);
      set_str_variable (templine, p);
    }
    // reduce $0 by 2
    snprintf (templine, NLINE, "%d.%d", MacroDepth, 0);
    set_int_variable (templine, argnum - 2);
  } 
  if (mode == MODE_BOOL) {
    if (varName) set_str_variable (varName, "1"); // NOTE: if a local variable with the same name exists, that will be used

    for (int i = argmatch + 1; i < argnum; i++) {
      // move $i+1 to $i
      snprintf (templine, NLINE, "%d.%d", MacroDepth, i);
      char *p = get_variable_ptr (templine);
      myAssert (p, "coding error");

      snprintf (templine, NLINE, "%d.%d", MacroDepth, i - 1);
      set_str_variable (templine, p);
    }
    // reduce $0 by 1
    snprintf (templine, NLINE, "%d.%d", MacroDepth, 0);
    set_int_variable (templine, argnum - 1);
  }
  return (TRUE);
}
