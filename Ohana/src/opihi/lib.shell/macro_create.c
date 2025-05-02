# include "opihi.h"
# define D_NLINES 100
# define prompt "> "
int macro_exec   PROTO((int, char **));

int macro_create (int argc, char **argv) {

  int ThisList, depth, i, done, NLINES, N;
  char *input, *help;
  Command *cmd;
  Macro *macro;

  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    help = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  } else {
    help = strcreate ("(macro)");
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: macro <name> [-c \"comment line\"]\n");
    gprint (GP_ERR, "  (enter commands, end with the word 'END')\n");
    return (FALSE);
  }

  /**** Check for existence of this macro ****/
  cmd = MatchCommand (argv[1], FALSE, TRUE);
  macro = MatchMacro (argv[1], FALSE, TRUE);
  
  if ((macro == NULL) && (cmd != NULL)) {
    gprint (GP_ERR, "cannot redefine inherent command %s\n", argv[1]);
    return (FALSE);
  }
  if ((macro != NULL) && (cmd == NULL)) {
    gprint (GP_ERR, "programming error: macro not in command list (%s)\n", argv[1]);
    return (FALSE);
  }

  if (macro == NULL) { /**** New Macro ****/
    ALLOCATE (cmd, Command, 1);
    cmd[0].real = FALSE;
    cmd[0].name = strcreate (argv[1]);
    cmd[0].help = help;
    cmd[0].func = macro_exec;
    AddCommand (cmd);
    free (cmd);
    macro = NewMacro (argv[1]);
  } else { /**** Old Macro ****/
    /* replace existing command help with new value */
    free (cmd[0].help);
    cmd[0].help = help;
  }

  /* reallocate space for macro lines */
  NLINES = D_NLINES;
  for (i = 0; i < macro[0].Nlines; i++) free (macro[0].line[i]);
  REALLOCATE (macro[0].line, char *, NLINES);

  /* read in macro
     If we are entering at the keyboard (ThisList == 0), use readline.
     Otherwise, read from the current list
     End when we hit the final "END" (the true END -- we count macro defines!!) */
     
  depth = 0;
  ThisList = current_list_depth();
  for (i = 0, done = FALSE; !done; ) {

    /* get the next line (from correct place) */
    if (ThisList == 0) {
      input = opihi_readline (prompt);
    } else {
      input = get_next_listentry (ThisList);
    }

    if ((ThisList == 0) && (input == (char *) NULL)) {
      gprint (GP_ERR, "end macro with 'END'\n");
      continue;
    }

    if ((ThisList > 0) && (input == (char *) NULL)) {
      gprint (GP_ERR, "missing 'END' in macro definition\n");
      input = strcreate ("end");
    }

    stripwhite (input);

    /* test for new macro (or other list, in the future?) */
    if (is_list (input)) depth ++;

    /* test for end of nested list -- if not nested, END refers to this macro */
    if (!strncasecmp (input, "END", 3)) {
      depth --;
      if (depth < 0) { /* we hit the last "END", macro is done */
	free (input);
	macro[0].Nlines = i;
	if (macro[0].Nlines == 0) i = 1;
	REALLOCATE (macro[0].line, char *, i);
	return (TRUE);
      }
    }

    if (*input) { 
      macro[0].line[i] = input;
      i++;
      if (i == NLINES - 1) {
	NLINES += D_NLINES;
	REALLOCATE (macro[0].line, char *, NLINES);
      }
    }
  }
  return (TRUE);
}
