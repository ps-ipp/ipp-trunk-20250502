# include "pantasks.h"
static int Ncheck = 0;

float CheckJobs () {

  FILE *f;
  Job *job;
  Task *task;
  Macro *macro;
  int i, status;
  char varname[64];
  Queue *queue;
  float time_running, next_timeout;

  Ncheck ++;

  // actual maximum delay is controlled in job_threads.c
  next_timeout = 1.0;

  JobTaskLock();
  /** test all jobs: ready to test?  finished? **/
  while ((job = NextJob ()) != NULL) {

    task = job[0].task;
    // XXX we need to guarantee that the task exists
    // if we delete a task, we need to keep a copy until all task jobs are
    // removed

    /* check poll period (ready to ask for status?) */
    time_running = GetTaskTimer(job[0].last, FALSE);
    // fprintf (stderr, "next: %f, poll: %f, run: %f\n", next_timeout, task[0].poll_period, time_running);
    if (time_running < task[0].poll_period) {
      next_timeout = MIN (next_timeout, task[0].poll_period - time_running);
      continue;
    }
    next_timeout = 0.0;

    /* check current status */
    status = CheckJob (job);
    switch (status) {
      case JOB_PENDING:
	/* if (VerboseMode()) gprint (GP_LOG, "job %s (%d) pending\n", task[0].name, job[0].JobID); */
	break;

      case JOB_BUSY:
	/* if (VerboseMode()) gprint (GP_LOG, "job %s (%d) busy\n", task[0].name, job[0].JobID); */
	break;

      case JOB_CRASH:
      case JOB_EXIT:
	/* push output buffer data to the stdout and stderr queues */
	/* XXX this will break on 0 values in output streams */
	if (DEBUG) fprintf (stderr, "job: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
	PushNamedQueue ("stdout", job[0].stdout_buff.buffer);
	PushNamedQueue ("stderr", job[0].stderr_buff.buffer);

	/* save the stdout and stderr if desired */
	if ((job[0].stdout_dump != NULL) && strcasecmp(job[0].stdout_dump, "NULL")) {
	  f = fopen (job[0].stdout_dump, "a");
	  if (f == NULL) {
	    gprint (GP_ERR, "unable to open stdout dump file %s\n", job[0].stdout_dump);
	  } else {
	    fwrite (job[0].stdout_buff.buffer, 1, job[0].stdout_buff.Nbuffer, f);
	    fclose (f);
	  }
	}
	if ((job[0].stderr_dump != NULL) && strcasecmp(job[0].stderr_dump, "NULL")) {
	  f = fopen (job[0].stderr_dump, "a");
	  if (f == NULL) {
	    gprint (GP_ERR, "unable to open stderr dump file %s\n", job[0].stderr_dump);
	  } else {
	    fwrite (job[0].stderr_buff.buffer, 1, job[0].stderr_buff.Nbuffer, f);
	    fclose (f);
	  }
	}

	/* set taskarg variables */
	for (i = 0; i < job[0].argc; i++) {
	  sprintf (varname, "taskarg:%d", i);
	  set_str_variable (varname, job[0].argv[i]);
	}
	set_int_variable ("taskarg:n", job[0].argc);

	/* set options variables */
	for (i = 0; i < job[0].optc; i++) {
	  sprintf (varname, "options:%d", i);
	  set_str_variable (varname, job[0].optv[i]);
	}
	set_int_variable ("options:n", job[0].optc);

	set_variable ("JOB_DTIME", job[0].dtime);

	if (job[0].realhost == NULL) {
	  set_str_variable ("JOB_HOSTNAME", "localhost");
	} else {
	  set_str_variable ("JOB_HOSTNAME", job[0].realhost);
	}	    

	if (status == JOB_CRASH) {
	  /* XXX add an Ncrash element? */
	  task[0].Nfailure ++;
	  UpdateTaskTimerStats (task, TIMER_FAILURE, job[0].dtime);

	  /* run task[0].crash macro, if it exists */
	  /* perhaps define PushNamedQueueBuffer */

	  set_str_variable ("JOB_STATUS", "CRASH");

	  if (VerboseMode()) gprint (GP_LOG, "job %s (%d) crash\n", task[0].name, job[0].JobID);
	  if (task[0].crash != NULL) {
	    // we need to unlock JobTask since JobTaskLock is called inside CommandLock in the client thread
	    JobTaskUnlock();
	    CommandLock();
	    exec_loop (task[0].crash);
	    CommandUnlock();
	    JobTaskLock();
	  }
	}
	if (status == JOB_EXIT) {
	  /* update the exit status counters */
	  if (job[0].exit_status) {
	    task[0].Nfailure ++;
	    UpdateTaskTimerStats (task, TIMER_FAILURE, job[0].dtime);
	  } else {
	    task[0].Nsuccess ++;
	    UpdateTaskTimerStats (task, TIMER_SUCCESS, job[0].dtime);
	  }

	  set_int_variable ("JOB_STATUS", job[0].exit_status);

	  /* run corresponding task[0].exit macro, if it exists */
	  if (VerboseMode()) gprint (GP_LOG, "job %s (%d) exit\n", task[0].name, job[0].JobID);
	  macro = task[0].defexit;
	  for (i = 0; i < task[0].Nexit; i++) {
	    if (job[0].exit_status == atoi(task[0].exit[i][0].name)) {
	      macro = task[0].exit[i];
	      break;
	    }
	  }
	  if (macro != NULL) {
	    // we need to unlock JobTask since JobTaskLock is called inside CommandLock in the client thread
	    JobTaskUnlock();
	    CommandLock();
	    exec_loop (macro);
	    CommandUnlock();
	    JobTaskLock();
	  }
	}

	/* flush the stderr and stdout queues */
	queue = FindQueue ("stdout");
	if (queue) InitQueue (queue);
	queue = FindQueue ("stderr");
	if (queue) InitQueue (queue);

	UpdateTaskTimerStats (task, TIMER_ALLJOBS, job[0].dtime);

	DeleteJob (job);
	continue;

      default:
	if (VerboseMode()) gprint (GP_LOG, "unknown exit status: %d\n", status);
	/** do something more useful here ?? **/
	break;
    }

    /* check for timeout - (local jobs only) 
       we only check timeout after a poll (forces at least one poll)
    */
    if (job[0].mode == JOB_LOCAL) {
      if (GetTaskTimer(job[0].start, FALSE) < task[0].timeout_period) { 
	/* reset polling clock */
	SetTaskTimer (&job[0].last);
	continue;
      }
      if (VerboseMode()) gprint (GP_LOG, "timeout on %s\n", task[0].name);

      // XXX harvest STDERR and STDOUT from timeout job (should be available...)
      // XXX add this to controller as well

      /* update the timeout counter */
      task[0].Ntimeout ++;

      if (!KillLocalJob (job)) {
	job[0].state = JOB_HUNG;
	if (VerboseMode()) gprint (GP_LOG, "child process %d is hung, cannot kill\n", job[0].pid);
	continue;
      }

      /* set taskarg variables */
      for (i = 0; i < job[0].argc; i++) {
	sprintf (varname, "taskarg:%d", i);
	set_str_variable (varname, job[0].argv[i]);
      }
      set_int_variable ("taskarg:n", job[0].argc);

      /* set options variables */
      for (i = 0; i < job[0].optc; i++) {
	sprintf (varname, "options:%d", i);
	set_str_variable (varname, job[0].optv[i]);
      }
      set_int_variable ("options:n", job[0].optc);

      /* run task[0].timeout macro, if it exists */
      if (task[0].timeout != NULL) {
	// we need to unlock JobTask since JobTaskLock is called inside CommandLock in the client thread
	JobTaskUnlock();
	CommandLock();
	exec_loop (task[0].timeout);
	CommandUnlock();
	JobTaskLock();
      }

      DeleteJob (job);
      continue;
    }

    /* reset polling clock */
    SetTaskTimer (&job[0].last);
  }

  JobTaskUnlock();
  return (next_timeout);
}

/* 

   job / task timeline:

   task:
   0           exec     
   start       create
   task clock  new job

   job:
   0           1xpoll     2xpoll     3xpoll
   start       check      check      check 
   job clock   status     status     status

   .           .          .          timeout
   run
   timeout

   must be at least one poll before timeout 
   (timeout >= poll)
*/
