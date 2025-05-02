# include "pantasks.h"
# define prompt "> "

int task (int argc, char **argv) {

  int hash;
  int ThisList;
  char *input, *outline;
  Task *task;

  if (argc != 2) goto usage;

  JobTaskLock();
  task = FindTask (argv[1]);
  if (task == NULL) { /**** new task ****/
    task = CreateTask (argv[1]);
  } else {
    RemoveTask (task);
    SetNewTask (task);
  }
  JobTaskUnlock();

  /* While a task is being defined, it is removed from the task list.  The new task is added to the task list
     when the definition process is complete.  
     XXX If an outstanding job has a task deleted, it will not be able to complete... */

  /* read in task from appropriate source (keyboard or list) until end */

  /* allowed tokens: command, host, stderr, periods, trange, nmax, task.exit, task.exec, end */

  ThisList = current_list_depth();
  while (1) {

    /* get the next line (from correct place) */
    if (ThisList == 0) 
      input = opihi_readline (prompt);
    else 
      input = get_next_listentry (ThisList);

    if ((ThisList == 0) && (input == (char *) NULL)) {
      gprint (GP_ERR, "end task with 'END'\n");
      continue;
    }
    if ((ThisList > 0) && (input == (char *) NULL)) {
      gprint (GP_ERR, "missing 'END' in task definition\n");
      input = strcreate ("end");
    }

    stripwhite (input);
    hash = TaskHash (input);
    switch (hash) {

      case TASK_EMPTY:
      case TASK_COMMENT:
	free (input);
	break;

      case TASK_END:
	free (input);
	/* validate the new task: all mandatory elements defined? */ 
	if (!ValidateTask (task, FALSE)) {
	  DeleteNewTask ();
	  return (FALSE);
	}
	JobTaskLock();
	RegisterNewTask ();
	JobTaskUnlock();
	return (TRUE);
	break;

      case TASK_TRANGE:
      case TASK_NMAX:
      case TASK_HOST:
      case TASK_NICE:
      case TASK_EXIT:
      case TASK_EXEC:
      case TASK_STDOUT:
      case TASK_STDERR:
      case TASK_COMMAND:
      case TASK_OPTIONS:
      case TASK_PERIODS:
      case TASK_NPENDING:
      case TASK_ACTIVE:
	// status = command(); do something with this info?
	command (input, &outline, TRUE);
	if (outline != NULL) free (outline);
	break;

      case TASK_NONE:
	gprint (GP_ERR, "unknown task command %s\n", input);
	break;
    }
  }

usage:
  gprint (GP_ERR, "USAGE: task -list\n");
  gprint (GP_ERR, "USAGE: task -longlist\n");
  gprint (GP_ERR, "USAGE: task -show (task)\n");
  gprint (GP_ERR, "USAGE: task <name>\n");
  gprint (GP_ERR, "  (enter commands & task functions; end with the word 'END')\n");
  return (FALSE);
}
