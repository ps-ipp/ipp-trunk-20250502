# include "basic.h"
# define prompt    "if: "

int run_if (int argc, char **argv) {

  int ThisList, depth, done, status, InlineCommand;
  int i, length, logic, foundElse;
  char *input, *val, *line;
  int nloop, size;

  InlineCommand = FALSE;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: if (conditional) : follow with commands, end with the word 'END'\n");
    gprint (GP_ERR, "   OR: if (conditional) command\n");
    return (FALSE);
  }
  if (argc > 2) {
    InlineCommand = TRUE;
  }

  /* determine value of conditional expression */
  val = dvomath (1, &argv[1], &size, 0);
  if (val == NULL) {
    gprint (GP_ERR, "syntax error in logic: %s\n", argv[1]);
    return (FALSE);
  }
  logic = atof (val); /* is round-off error a danger? */ 
  free (val);

  if (InlineCommand) {
    if (logic) {
      /* re-build a command line from the remaining strings */
      length = 0;
      for (i = 2; i < argc; i++) {
	length += strlen(argv[i]) + 1;
      }
      length++;
      ALLOCATE (line, char, length);
      memset (line, 0, length);
      for (i = 2; i < argc; i++) {
	if (i == 2) {
	  strcpy (line, argv[i]);
	} else {
	  strcat (line, " ");
	  strcat (line, argv[i]);
	}
      }
      status = multicommand (line);
      free (line);
      return (status);
    } else {
      return (TRUE);
    }
  }    

  /* read in if-list */
  nloop = 0;
  depth = 0;
  ThisList = current_list_depth();

  foundElse = FALSE;
  done = FALSE;
  while (!done) {

    nloop ++;
    /* get the next line (from correct place) */
    if (ThisList == 0) {
      input = opihi_readline (prompt);
    } else {
      input = get_next_listentry (ThisList);
    }

    if ((ThisList == 0) && (input == NULL)) {
      gprint (GP_ERR, "end if-block with 'END'\n");
      continue;
    }
    if ((ThisList >  0) && (input == NULL)) {
      gprint (GP_ERR, "missing 'END' in if-block\n");
      input = strcreate ("end");
    }

    stripwhite (input);

    /* test for new macro, search for "end" statement */
    if (!logic && is_list (input)) {
      depth ++;
      free (input);
      continue;
    }
    
    /* check for an "else", invert logic */
    if ((depth == 0) && !strncasecmp (input, "ELSE", 4)) {
      logic = !logic;
      free (input);

      if (foundElse) {
	gprint (GP_ERR, "multiple 'ELSE' statements in if-block with 'END'\n");
	return FALSE;
      }
      foundElse = TRUE;
      continue;
    }

    /* test for end of nested block -- if not nested, END refers to this if */
    if (!strncasecmp (input, "END", 3)) {
      depth --;
      if (depth < 0) { 
	/* we hit the last "END", if-block is done */
	free (input);
	return (TRUE);	
      }
      free (input);  /* a do-nothing line */
      continue;
    }

    if (logic) {
      if (*input) { 
	status = multicommand (input);
	if (ThisList == 0) add_history (input);
	if (auto_break && !status) {
	  free (input);
	  return (FALSE);
	}
      }
    } 
    free (input);
  }
  return (TRUE);
}

/*
     If we are entering at the keyboard (ThisList == 0), use readline.
     Otherwise, read from the current list, remove list lines.
     End when we hit the final "END" (the true END -- we count macro defines!!) 
     */
