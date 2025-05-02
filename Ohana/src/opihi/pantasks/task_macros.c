# include "pantasks.h"
# define D_NLINES 100
# define prompt "> "

int task_macros (int argc, char **argv) {

  int i, N, NLINES, done, depth, ThisList;
  char *input;
  Macro *macro;
  Task *task;

  if (!strcmp (argv[0], "task.exec") && (argc != 1)) {
    gprint (GP_ERR, "USAGE: task.exec\n");
    gprint (GP_ERR, "  (define pre-exec macro for this task)\n");
    return (FALSE);
  }

  if (!strcmp (argv[0], "task.exit") && (argc != 2)) {
    gprint (GP_ERR, "USAGE: task.exit (state)\n");
    gprint (GP_ERR, "  (define exit state macro for this task)\n");
    return (FALSE);
  }

  JobTaskLock();
  task = GetNewTask ();
  if (task == NULL) {
    gprint (GP_ERR, "ERROR: not defining or running a task\n");
    JobTaskUnlock();
    return (FALSE);
  }

  macro = NULL;

  /*** identify which Macro in Task this particular macro goes with ***/ 
  if (!strcmp (argv[0], "task.exec")) {
    if (task[0].exec != NULL) {
      FreeMacro (task[0].exec);
      free (task[0].exec);
    }
    ALLOCATE (task[0].exec, Macro, 1);
    macro = task[0].exec;
    macro[0].name = strcreate ("exec");
    goto found;
  }
  if (!strcmp (argv[0], "task.exit") && !strcmp (argv[1], "timeout")) {
    if (task[0].timeout != NULL) {
      FreeMacro (task[0].timeout);
      free (task[0].timeout);
    }
    ALLOCATE (task[0].timeout, Macro, 1);
    macro = task[0].timeout;
    macro[0].name = strcreate ("timeout");
    goto found;
  }
  if (!strcmp (argv[0], "task.exit") && !strcmp (argv[1], "crash")) {
    if (task[0].crash != NULL) {
      FreeMacro (task[0].crash);
      free (task[0].crash);
    }
    ALLOCATE (task[0].crash, Macro, 1);
    macro = task[0].crash;
    macro[0].name = strcreate ("crash");
    goto found;
  }
  if (!strcmp (argv[0], "task.exit") && !strcmp (argv[1], "default")) {
    if (task[0].defexit != NULL) {
      FreeMacro (task[0].defexit);
      free (task[0].defexit);
    }
    ALLOCATE (task[0].defexit, Macro, 1);
    macro = task[0].defexit;
    macro[0].name = strcreate ("default");
    goto found;
  }
  if (!strcmp (argv[0], "task.exit")) {	/* generic exit function */
    for (i = 0; i < task[0].Nexit; i++) { /* find exiting exit function with same name */
      if (!strcmp (task[0].exit[i][0].name, argv[1])) {	/*** replace this one ***/
	FreeMacro (task[0].exit[i]);
	free (task[0].exit[i]);
	ALLOCATE (task[0].exit[i], Macro, 1);
	macro = task[0].exit[i];
	macro[0].name = strcreate (argv[1]);
	goto found;
      }
    }
    /** create a new exit status macro **/
    N = task[0].Nexit;
    ALLOCATE (task[0].exit[N], Macro, 1);
    task[0].Nexit ++;
    if (task[0].Nexit == task[0].NEXIT) {
      task[0].NEXIT += 10;
      REALLOCATE (task[0].exit, Macro *, task[0].NEXIT);
    }
    macro = task[0].exit[N];
    macro[0].name = strcreate (argv[1]);
    goto found;
  }

found:

  macro[0].Nlines = 0;
  NLINES = D_NLINES;
  ALLOCATE (macro[0].line, char *, NLINES);

  /* load macro lines from appropriate source (readline / list) */

  depth = 0;
  done = FALSE;
  ThisList = current_list_depth();
  while (!done) {

    /* get the next line (from correct place) */
    if (ThisList == 0) 
      input = opihi_readline (prompt);
    else 
      input = get_next_listentry (ThisList);

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
	REALLOCATE (macro[0].line, char *, MAX (1, macro[0].Nlines));
	JobTaskUnlock();
	return (TRUE);
      }
    }

    if (*input) { 
      macro[0].line[macro[0].Nlines] = input;
      macro[0].Nlines ++;
      if (macro[0].Nlines == NLINES - 1) {
	NLINES += D_NLINES;
	REALLOCATE (macro[0].line, char *, NLINES);
      }
    }
  }
  JobTaskUnlock();
  return (TRUE);
}
