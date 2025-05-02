# include "pantasks.h"
static int Ncheck = 0; 

float CheckTasks () {

  Job *job;
  Task *task;
  int status;
  float time_running, next_timeout, fuzz;

  Ncheck ++;

  // actual maximum delay is controlled in job_threads.c
  next_timeout = 1.0;

  JobTaskLock();
  /** test all tasks: ready to test? ready to run? **/
  while ((task = NextTask ()) != NULL) {

    /*** test for all reasons we should skip this task ***/

    /* task has been de-activated by the user */
    if (!task[0].active) {
      continue;
    }
    /* current time is not within valid/invalid periods */
    if (!CheckTimeRanges (task[0].ranges, task[0].Nranges)) {
      continue;
    }
    /* all allowed tasks have been run in this period */
    if (task[0].Nmax && (task[0].Njobs >= task[0].Nmax)) {
      continue;
    }
    /* too many outstanding jobs */
    if (task[0].NpendingMax && (task[0].Npending >= task[0].NpendingMax)) {
      continue;
    }

    /* ready to test? : check time since last exec */
    time_running = GetTaskTimer(task[0].last, FALSE);
    if (time_running < task[0].exec_period) {
      // is we are to ready to run, set time to timeout, if shortest of all tasks
      next_timeout = MIN (next_timeout, task[0].exec_period - time_running);
      continue;
    }

    /* ready to try running the task : reset the timer */
    next_timeout = 0.0;
    gettimeofday (&task[0].last, (void *) NULL);

    // add random offset between 0 and 5% of exec_period
    // XXX this should be optional
    fuzz = task[0].exec_period*(0.5*drand48() - 0.25);
    task[0].last.tv_usec += 1e6*(fuzz - (int)fuzz);
    task[0].last.tv_sec += (int) fuzz;

    /* ready to run? : run task.exec macro */
    if (task[0].exec != NULL) {
      // we need to unlock JobTask since JobTaskLock is called inside CommandLock in the client thread
      JobTaskUnlock();
      CommandLock();
      status = exec_loop (task[0].exec);
      CommandUnlock();
      JobTaskLock();
      if (!status) {
	task[0].Nskipexec ++;
	continue;
      }
    }

    /* check if there are errors with this task */
    if (!ValidateTask (task, TRUE)) { 
      continue;
    }
    
    /* construct job from task */
    job = CreateJob (task);
    if (!job) {
      continue;
    }

    if (DEBUG) fprintf (stderr, "create job: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 

    /* execute job */
    if (!SubmitJob (job)) {
      DeleteJob (job);
      continue;
    }
    task[0].Njobs ++; // number of jobs successfully submitted

    /* increment Nrun for inclusive ranges with Nmax */
    BumpTimeRanges (task[0].ranges, task[0].Nranges);
  }
  JobTaskUnlock();
  return (next_timeout);
}
