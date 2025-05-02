# include "pantasks.h"
# include <regex.h>

static Task **tasks;
static int    Ntasks;
static int    NTASKS;

/* counter marking task being visited by the run loop */
static int   ActiveTask;

/* temporary holder for a new task */
static Task *NewTask = NULL;

/* set up the task list system */
void InitTasks () {
  NTASKS = 20;
  Ntasks = 0;
  ALLOCATE (tasks, Task *, NTASKS);
  ActiveTask = -1;
}

void FreeTasks () {
  int i;
  int ntasks = Ntasks;
  Ntasks = 0;
  for (i = 0; i < ntasks; i++) {
    FreeTask (tasks[i]);
  }
  free (tasks);
}

/* provide a mechanism to loop over the list of tasks */
Task *NextTask () {
  
  Task *task;

  /* move to the next task and return it */
  ActiveTask ++;
  if (ActiveTask < 0) ActiveTask = 0;
  if (ActiveTask >= Ntasks) {
    ActiveTask = -1;
    return (NULL);
  }
  task = tasks[ActiveTask];
  return (task);
}

/* return task with given name */
Task *FindTask (char *name) {

  int i;

  /* try for an exact match first */
  for (i = 0; i < Ntasks; i++) {
    if (!strcmp (tasks[i][0].name, name)) {
      return (tasks[i]);
    }
  }
  return (NULL);
}  

# define getnchars(LEN, MINLEN) { float loglength; loglength = log10(LEN); LEN = ((int)loglength < loglength) ? loglength + 1 : loglength; LEN = (LEN < MINLEN) ? MINLEN : LEN; }

/* list known tasks */
void ListTasks (int verbose) {

  int i, j, valid, nameLength, cmdLength, maxJobs, maxRun, maxDone, maxFail, maxTime, Nrun;
  char *start, *stop;
  char format[128];

  gprint (GP_LOG, "\n");
  if (Ntasks == 0) {
    gprint (GP_LOG, " no defined tasks\n");
    return;
  }

  /* find string lengths */
  nameLength = cmdLength = 0;
  maxJobs = maxRun = maxDone = maxFail = maxTime = 0;
  for (i = 0; i < Ntasks; i++) {
    nameLength = MAX (nameLength, strlen(tasks[i][0].name));
    if (tasks[i][0].argv == NULL) {
      cmdLength = MAX (nameLength, strlen("(dynamic)"));
    } else {
      cmdLength = MAX (nameLength, strlen(tasks[i][0].argv[0]));
    }
    maxJobs = MAX(maxJobs, tasks[i][0].Njobs);
    maxDone = MAX(maxDone, tasks[i][0].Nsuccess);
    maxFail = MAX(maxFail, tasks[i][0].Nfailure);
    maxTime = MAX(maxTime, tasks[i][0].Ntimeout);
    maxRun  = MAX(maxRun,  tasks[i][0].Njobs - tasks[i][0].Nsuccess - tasks[i][0].Nfailure - tasks[i][0].Ntimeout);
  }

  gprint (GP_LOG, " Task Status\n");

  getnchars(maxJobs, 5);
  getnchars(maxRun,  5);
  getnchars(maxDone, 5);
  getnchars(maxFail, 5);
  getnchars(maxTime, 5);

  snprintf (format, 128, "  AV %%-%ds  %%%ds  %%%ds  %%%ds %%%ds %%%ds %%-%ds\n", nameLength, maxRun, maxJobs, maxDone, maxFail, maxTime, cmdLength);
  gprint (GP_LOG, format, "Name", "Nrun", "Njobs", "Ngood", "Nfail", "Ntime", "Command");

  snprintf (format, 128, "%%-%ds  %%%dd  %%%dd  %%%dd %%%dd %%%dd %%-%ds\n", nameLength, maxRun, maxJobs, maxDone, maxFail, maxTime, cmdLength);
  for (i = 0; i < Ntasks; i++) {
    valid = CheckTimeRanges (tasks[i][0].ranges, tasks[i][0].Nranges);
    if (verbose) gprint (GP_LOG, "\n");
    if (tasks[i][0].active) {
      gprint (GP_LOG, "  +");
    } else {
      gprint (GP_LOG, "  -");
    }
    if (valid) {
      gprint (GP_LOG, "+ ");
    } else {
      gprint (GP_LOG, "- ");
    }
    Nrun = tasks[i][0].Njobs - tasks[i][0].Nsuccess - tasks[i][0].Nfailure - tasks[i][0].Ntimeout;
    if (tasks[i][0].argv == NULL) {
      gprint (GP_LOG, format, tasks[i][0].name, Nrun, tasks[i][0].Njobs, tasks[i][0].Nsuccess, tasks[i][0].Nfailure, tasks[i][0].Ntimeout, "(dynamic)");
    } else {
      gprint (GP_LOG, format, tasks[i][0].name, Nrun, tasks[i][0].Njobs, tasks[i][0].Nsuccess, tasks[i][0].Nfailure, tasks[i][0].Ntimeout, tasks[i][0].argv[0]);
    }
    if (verbose) {
      gprint (GP_LOG, "    spawn period: %f, polling period: %f, timeout period: %f\n", 
	       tasks[i][0].exec_period, tasks[i][0].poll_period, tasks[i][0].timeout_period);
      for (j = 0; j < tasks[i][0].Nranges; j++) {
	switch (tasks[i][0].ranges[j].type) {
	  case RANGE_ABS:
	    start = ohana_sec_to_date (tasks[i][0].ranges[j].start);
	    stop  = ohana_sec_to_date (tasks[i][0].ranges[j].stop);
	    break;
	  case RANGE_DAY:
	    start = ohana_sec_to_hms (tasks[i][0].ranges[j].start);
	    stop  = ohana_sec_to_hms (tasks[i][0].ranges[j].stop);
	    break;
	  case RANGE_WEEK:
	    start = ohana_sec_to_day (tasks[i][0].ranges[j].start);
	    stop  = ohana_sec_to_day (tasks[i][0].ranges[j].stop);
	    break;
	  default:
	    abort ();
	}
	if (tasks[i][0].ranges[j].include) {
	  gprint (GP_LOG, "     active : %s - %s", start, stop);
	  if (tasks[i][0].ranges[j].Nmax) gprint (GP_LOG, " (%d of %d)", tasks[i][0].ranges[j].Nrun, tasks[i][0].ranges[j].Nmax);
	  gprint (GP_LOG, "\n");
	} else {
	  gprint (GP_LOG, "     avoid  : %s - %s\n", start, stop);
	}
	free (start);
	free (stop);
      }
      gprint (GP_LOG, "     Nskip exec: %d\n", tasks[i][0].Nskipexec);
      if (tasks[i][0].host == NULL) {
	gprint (GP_LOG, "    task runs locally\n");
	continue;
      }
      if (!strcasecmp(tasks[i][0].host, "ANYHOST")) {
	gprint (GP_LOG, "    task host selected by controller\n");
	continue;
      }
      if (tasks[i][0].host_required) {
	gprint (GP_LOG, "    host %s (required)\n", tasks[i][0].host);
      } else {
	gprint (GP_LOG, "    host %s (desired)\n", tasks[i][0].host);
      }
    }
  }
  return;
}

/* list known tasks */
void ListTaskStats (char *regex) {

  int i, valid, nameLength;
  char format[128];
  regex_t preg;

  gprint (GP_LOG, "\n");
  if (Ntasks == 0) {
    gprint (GP_LOG, " no defined tasks\n");
    return;
  }

  if (regex != NULL) {
    regcomp (&preg, regex, REG_EXTENDED);
  }

  /* find string lengths */
  nameLength = 0;
  for (i = 0; i < Ntasks; i++) {
    nameLength = MAX (nameLength, strlen(tasks[i][0].name));
  }

  gprint (GP_LOG, " Task Statistics\n");

  snprintf (format, 128, "     %%-%ds |           alljobs          |           success          |           failure          |\n", nameLength);
  gprint (GP_LOG, format, "");
  snprintf (format, 128, "  AV %%-%ds | Njobs   Tmin   Tave   Tmax | Njobs   Tmin   Tave   Tmax | Njobs   Tmin   Tave   Tmax |\n", nameLength);
  gprint (GP_LOG, format, "Name");

  snprintf (format, 128, "%%-%ds", nameLength);
  for (i = 0; i < Ntasks; i++) {
      
    if ((regex != NULL) && regexec (&preg, tasks[i][0].name, 0, NULL, 0)) continue;

    valid = CheckTimeRanges (tasks[i][0].ranges, tasks[i][0].Nranges);
    if (tasks[i][0].active) {
      gprint (GP_LOG, "  +");
    } else {
      gprint (GP_LOG, "  -");
    }
    if (valid) {
      gprint (GP_LOG, "+ ");
    } else {
      gprint (GP_LOG, "- ");
    }
    if (tasks[i][0].argv == NULL) {
      gprint (GP_LOG, format, tasks[i][0].name);
    } else {
      gprint (GP_LOG, format, tasks[i][0].name);
    }
    if (tasks[i][0].dtimeMin_alljobs < 0) {
      gprint (GP_LOG, " | %5d %6s %6.2f %6.2f",     tasks[i][0].Njobs,    "NONE",                       tasks[i][0].dtimeAve_alljobs, tasks[i][0].dtimeMax_alljobs);
    } else {
      gprint (GP_LOG, " | %5d %6.2f %6.2f %6.2f",   tasks[i][0].Njobs,    tasks[i][0].dtimeMin_alljobs, tasks[i][0].dtimeAve_alljobs, tasks[i][0].dtimeMax_alljobs);
    }      
    if (tasks[i][0].dtimeMin_success < 0) {
      gprint (GP_LOG, " | %5d %6s %6.2f %6.2f",     tasks[i][0].Nsuccess, "NONE",                       tasks[i][0].dtimeAve_success, tasks[i][0].dtimeMax_success);
    } else {
      gprint (GP_LOG, " | %5d %6.2f %6.2f %6.2f",   tasks[i][0].Nsuccess, tasks[i][0].dtimeMin_success, tasks[i][0].dtimeAve_success, tasks[i][0].dtimeMax_success);
    }
    if (tasks[i][0].dtimeMin_failure < 0) {
      gprint (GP_LOG, " | %5d %6s %6.2f %6.2f |\n",   tasks[i][0].Nfailure, "NONE",                       tasks[i][0].dtimeAve_failure, tasks[i][0].dtimeMax_failure);
    } else {
      gprint (GP_LOG, " | %5d %6.2f %6.2f %6.2f |\n", tasks[i][0].Nfailure, tasks[i][0].dtimeMin_failure, tasks[i][0].dtimeAve_failure, tasks[i][0].dtimeMax_failure);
    }
  }
  return;
}

/* list known tasks */
void ResetTaskStats (char *regex) {

  int i, nameLength;
  regex_t preg;

  if (Ntasks == 0) {
    return;
  }

  if (regex != NULL) {
    regcomp (&preg, regex, REG_EXTENDED);
  }

  /* find string lengths */
  nameLength = 0;
  for (i = 0; i < Ntasks; i++) {
    nameLength = MAX (nameLength, strlen(tasks[i][0].name));
  }

  for (i = 0; i < Ntasks; i++) {
      
    if ((regex != NULL) && regexec (&preg, tasks[i][0].name, 0, NULL, 0)) continue;

    tasks[i][0].Njobs = 0;
    tasks[i][0].dtimeMin_alljobs = 0;
    tasks[i][0].dtimeAve_alljobs = 0;
    tasks[i][0].dtimeMax_alljobs = 0;

    tasks[i][0].Nsuccess = 0;
    tasks[i][0].dtimeMin_success = 0;
    tasks[i][0].dtimeAve_success = 0;
    tasks[i][0].dtimeMax_success = 0;

    tasks[i][0].Nfailure = 0;
    tasks[i][0].dtimeMin_failure = 0;
    tasks[i][0].dtimeAve_failure = 0;
    tasks[i][0].dtimeMax_failure = 0;
  }
  return;
}

/* show details of a task */
int ShowTask (char *name) {

  int i, j;
  char *start, *stop;
  Task *task;

  task = FindTask (name);
  if (task == NULL) {
    gprint (GP_LOG, "task %s not found\n", name);
    return (FALSE);
  }

  gprint (GP_LOG, "\n macro %s\n", task[0].name);

  gprint (GP_LOG, "\n command: ");
  for (i = 0; i < task[0].argc; i++) {
    gprint (GP_LOG, "%s ", task[0].argv[i]);
  }
  gprint (GP_LOG, "\n\n");

  gprint (GP_LOG, "\n options: ");
  for (i = 0; i < task[0].optc; i++) {
    gprint (GP_LOG, "%s ", task[0].optv[i]);
  }
  gprint (GP_LOG, "\n\n");

  if (task[0].host == NULL) {
    gprint (GP_LOG, " task runs locally\n");
    goto periods;
  }
  if (!strcasecmp(task[0].host, "ANYHOST")) {
    gprint (GP_LOG, " task host selected by controller\n");
    goto periods;
  }
  if (task[0].host_required) {
    gprint (GP_LOG, " host %s (required)\n", task[0].host);
  } else {
    gprint (GP_LOG, " host %s (desired)\n", task[0].host);
  }

periods:
  gprint (GP_LOG, " time periods: exec: %f  poll: %f  timeout: %f\n", 
	  task[0].exec_period, task[0].poll_period, task[0].timeout_period);

  for (j = 0; j < tasks[i][0].Nranges; j++) {
    switch (tasks[i][0].ranges[j].type) {
      case RANGE_ABS:
	start = ohana_sec_to_date (tasks[i][0].ranges[j].start);
	stop  = ohana_sec_to_date (tasks[i][0].ranges[j].stop);
	break;
      case RANGE_DAY:
	start = ohana_sec_to_hms (tasks[i][0].ranges[j].start);
	stop  = ohana_sec_to_hms (tasks[i][0].ranges[j].stop);
	break;
      case RANGE_WEEK:
	start = ohana_sec_to_day (tasks[i][0].ranges[j].start);
	stop  = ohana_sec_to_day (tasks[i][0].ranges[j].stop);
	break;
      default:
	abort ();
    }
    if (tasks[i][0].ranges[j].include) {
      gprint (GP_LOG, "     active : %s - %s", start, stop);
      if (tasks[i][0].ranges[j].Nmax) gprint (GP_LOG, " (%d of %d)", tasks[i][0].ranges[j].Nrun, tasks[i][0].ranges[j].Nmax);
      gprint (GP_LOG, "\n");
    } else {
      gprint (GP_LOG, "     avoid  : %s - %s\n", start, stop);
    }
    free (start);
    free (stop);
  }

  gprint (GP_LOG, "\n pre-execute macro\n");
  ListMacro (task[0].exec);

  gprint (GP_LOG, "\n timeout macro\n");
  ListMacro (task[0].timeout);

  gprint (GP_LOG, "\n crash macro\n");
  ListMacro (task[0].crash);

  gprint (GP_LOG, "\n default exit macro\n");
  ListMacro (task[0].defexit);

  for (i = 0; i < task[0].Nexit; i++) {
    gprint (GP_LOG, "\n exit macro (status == %d)\n", atoi(task[0].exit[i][0].name));
    ListMacro (task[0].exit[i]);
  }

  return (TRUE);
}

/* make a new named task */
int FreeTask (Task *task) {
  
  int i;

  if (task == NULL) return (FALSE);
  
  if (task[0].name != NULL) free (task[0].name);
  if (task[0].host != NULL) free (task[0].host);
  if (task[0].argv != NULL) {
    for (i = 0; i < task[0].argc; i++) {
      free (task[0].argv[i]);
    }
    free (task[0].argv);
  }
  if (task[0].optv != NULL) {
    for (i = 0; i < task[0].optc; i++) {
      free (task[0].optv[i]);
    }
    free (task[0].optv);
  }
  if (task[0].exec != NULL) {
    FreeMacro (task[0].exec);
    free (task[0].exec);
  }
  if (task[0].crash != NULL) {
    FreeMacro (task[0].crash);
    free (task[0].crash);
  }
  if (task[0].timeout != NULL) {
    FreeMacro (task[0].timeout);
    free (task[0].timeout);
  }
  for (i = 0; i < task[0].Nexit; i++) {
    if (task[0].exit[i] != NULL) {
      FreeMacro (task[0].exit[i]);
    }
    free (task[0].exit[i]);
  }
  free (task[0].exit);

  if (task[0].ranges != NULL) {
    free (task[0].ranges);
  }
  return (TRUE);
}

/**** new task functions ***/

/* make a new named task */
Task *CreateTask (char *name) {
  
  ALLOCATE (NewTask, Task, 1);

  NewTask[0].name = strcreate (name);;

  NewTask[0].host = NULL;
  NewTask[0].host_required = FALSE;

  NewTask[0].argc = 0;
  NewTask[0].argv = NULL;

  NewTask[0].optc = 0;
  NewTask[0].optv = NULL;

  NewTask[0].stdout_dump = NULL;
  NewTask[0].stderr_dump = NULL;

  NewTask[0].exec = NULL;
  NewTask[0].crash = NULL;
  NewTask[0].timeout = NULL;
  NewTask[0].defexit = NULL;

  NewTask[0].Nexit = 0;
  NewTask[0].NEXIT = 10;
  ALLOCATE (NewTask[0].exit, Macro *, NewTask[0].NEXIT);
  /* don't free tasks[0].exit, keep at least 1 allocated */

  NewTask[0].exec_period = 1.0;
  NewTask[0].poll_period = 1.0;
  NewTask[0].timeout_period = 1.0;

  NewTask[0].Nranges = 0;
  ALLOCATE (NewTask[0].ranges, TimeRange, 1);

  /* init task timer (is reset by 'run') */  
  gettimeofday (&NewTask[0].last, (void *) NULL);
  NewTask[0].Nmax = 0;  /* default value means 'no limit' */

  NewTask[0].NpendingMax = 0;  /* default value means 'no limit' */
  NewTask[0].Npending = 0;  /* default value means 'no limit' */

  NewTask[0].Njobs = 0;
  NewTask[0].Nsuccess = 0;
  NewTask[0].Nfailure = 0;
  NewTask[0].Ntimeout = 0;
  NewTask[0].Nskipexec = 0;

  /* jobs timing statistics */
  NewTask[0].dtimeAve_alljobs =  0.0;
  NewTask[0].dtimeMin_alljobs = -1.0;
  NewTask[0].dtimeMax_alljobs =  0.0;

  NewTask[0].dtimeAve_success =  0.0;
  NewTask[0].dtimeMin_success = -1.0;
  NewTask[0].dtimeMax_success =  0.0;

  NewTask[0].dtimeAve_failure =  0.0;
  NewTask[0].dtimeMin_failure = -1.0;
  NewTask[0].dtimeMax_failure =  0.0;

  NewTask[0].active = TRUE;
  NewTask[0].nicelevel = 0;
  return (NewTask);
}

/* remove the task from the task list */
int RemoveTask (Task *task) {
  
  int i, Nt;

  /* find task in task list */
  Nt = -1;
  for (i = 0; i < Ntasks; i++) {
    if (task == tasks[i]) {
      Nt = i;
      break;
    }
  }
  if (Nt == -1) {
    gprint (GP_ERR, "programming error: task not found\n");
    return (FALSE);
  }
  for (i = Nt; i < Ntasks - 1; i++) {
    tasks[i] = tasks[i+1];
  }
  Ntasks --;
  return (TRUE);
}

int ValidateTask (Task *task, int RequireStatic) {

  int i, hash;

  /* is a static command defined? */
  if (task[0].argc != 0) {
    if (task[0].argv == NULL) {
      gprint (GP_ERR, "task command arguments not defined (programming error)\n");
      return (FALSE);
    }
    return (TRUE);
  }
  if (RequireStatic) {
    gprint (GP_ERR, "task command not defined\n");
    return (FALSE);
  }

  /* no static command; dynamic command? */
  if (task[0].exec != NULL) {
    for (i = 0; i < task[0].exec[0].Nlines; i++) {
      hash = TaskHash (task[0].exec[0].line[i]);
      if (hash == TASK_COMMAND) return (TRUE);
    }
  }
  gprint (GP_ERR, "task command not defined\n");
  return (FALSE);
}

int RegisterNewTask () {
  
  int N;

  N = Ntasks;
  Ntasks ++;
  if (Ntasks == NTASKS) {
    NTASKS += 20;
    REALLOCATE (tasks, Task *, NTASKS);
  }
  tasks[N] = NewTask;
  NewTask = NULL;
  return (TRUE);
}

int DeleteNewTask () {
  if (NewTask != NULL) {
    FreeTask (NewTask);
    free (NewTask);
    NewTask = NULL;
  }
  return (TRUE);
}

Task *SetNewTask (Task *task) {
  NewTask = task;
  return (task);
}

Task *GetNewTask () {
  return (NewTask);
}

Task *GetActiveTask () {
  Task *task;
  if (ActiveTask < 0) return (NULL);
  task = tasks[ActiveTask];
  return (task);
}

int TaskHash (char *input) {
  
  int hash;
  char *command;

  hash = TASK_NONE;

  command = thisword (input);
  if (command == NULL) return (TASK_EMPTY);

  if (command[0] == '#')                  hash = TASK_COMMENT;
  if (!strcasecmp (command, "END"))       hash = TASK_END;
  if (!strcasecmp (command, "HOST"))      hash = TASK_HOST;
  if (!strcasecmp (command, "NICE"))      hash = TASK_NICE;
  if (!strcasecmp (command, "NMAX"))      hash = TASK_NMAX;
  if (!strcasecmp (command, "ACTIVE"))    hash = TASK_ACTIVE;
  if (!strcasecmp (command, "TRANGE"))    hash = TASK_TRANGE;
  if (!strcasecmp (command, "STDOUT"))    hash = TASK_STDOUT;
  if (!strcasecmp (command, "STDERR"))    hash = TASK_STDERR;
  if (!strcasecmp (command, "COMMAND"))   hash = TASK_COMMAND;
  if (!strcasecmp (command, "OPTIONS"))   hash = TASK_OPTIONS;
  if (!strcasecmp (command, "PERIODS"))   hash = TASK_PERIODS;
  if (!strcasecmp (command, "NPENDING"))  hash = TASK_NPENDING;
  if (!strcasecmp (command, "TASK.EXIT")) hash = TASK_EXIT;
  if (!strcasecmp (command, "TASK.EXEC")) hash = TASK_EXEC;

  free (command);
  return (hash);
}

/*** task timer functions ***/

double GetTaskTimer (struct timeval start, int verbose) {

  double dtime;
  struct timeval now;
  
  gettimeofday (&now, (void *) NULL);
  dtime = DTIME (now, start);
  
  if (verbose) {
      fprintf (stderr, "tt: %d %6d  - %d %6d : %f\n", 
	       (int) now.tv_sec, (int) now.tv_usec, 
	       (int) start.tv_sec, (int) start.tv_usec, dtime);
  }

  return (dtime);
}

void SetTaskTimer (struct timeval *timer) {
  gettimeofday (timer, (void *) NULL);
}

/* start the clock for all tasks */
void InitTaskTimers () {

  Task *task;
  double isec, fsec;

  while ((task = NextTask ()) != NULL) {
    gettimeofday (&task[0].last, (void *) NULL);
    fsec = modf (task[0].exec_period, &isec);
    task[0].last.tv_sec -= isec;
    task[0].last.tv_usec -= 1e6*fsec;
 }
}

/* must call this after updating the corresponding counter */
void UpdateTaskTimerStats (Task *task, int mode, double dtime) {

  double total;

  switch (mode) {
    case TIMER_ALLJOBS:
      total = task[0].dtimeAve_alljobs * (task[0].Njobs - 1);
      total += dtime;
      task[0].dtimeAve_alljobs = total / (float) task[0].Njobs;
      if (task[0].dtimeMin_alljobs < 0) {
	task[0].dtimeMin_alljobs = dtime;
      } else {
	task[0].dtimeMin_alljobs = MIN (task[0].dtimeMin_alljobs, dtime);
      }
      task[0].dtimeMax_alljobs = MAX (task[0].dtimeMax_alljobs, dtime);
      break;
    case TIMER_SUCCESS:
      total = task[0].dtimeAve_success * (task[0].Nsuccess - 1);
      total += dtime;
      task[0].dtimeAve_success = total / (float) task[0].Nsuccess;
      if (task[0].dtimeMin_success < 0) {
	task[0].dtimeMin_success = dtime;
      } else {
	task[0].dtimeMin_success = MIN (task[0].dtimeMin_success, dtime);
      }
      task[0].dtimeMax_success = MAX (task[0].dtimeMax_success, dtime);
      break;
    case TIMER_FAILURE:
      total = task[0].dtimeAve_failure * (task[0].Nfailure - 1);
      total += dtime;
      task[0].dtimeAve_failure = total / (float) task[0].Nfailure;
      if (task[0].dtimeMin_failure < 0) {
	task[0].dtimeMin_failure = dtime;
      } else {
	task[0].dtimeMin_failure = MIN (task[0].dtimeMin_failure, dtime);
      }
      task[0].dtimeMax_failure = MAX (task[0].dtimeMax_failure, dtime);
      break;
    default:
      abort();
  }
}
