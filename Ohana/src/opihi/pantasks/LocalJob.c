# include "pantasks.h"
# include <sys/resource.h>

/* local jobs are forked in the background 
   we might need to limit the maximum number of local jobs.
   should we have a queue/stack of pending local jobs, much
   like controller? */

/* update current state, drain stdout/stderr buffers */
int CheckLocalJob (Job *job) {

  int Nread;

  // XXX do something useful with exit status?
  CheckLocalJobStatus (job);

  if ((job[0].state == JOB_EXIT) || (job[0].state == JOB_CRASH)) {
    if (DEBUG) fprintf (stderr, "empty buffer 0: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
    EmptyIOBuffer (&job[0].stdout_buff, 10, job[0].stdout_fd);
    EmptyIOBuffer (&job[0].stderr_buff, 10, job[0].stderr_fd);
    if (DEBUG) fprintf (stderr, "empty buffer 1: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
    close (job[0].stdout_fd);
    close (job[0].stderr_fd);
    job[0].stdout_fd = -1; // prevent FreeJob from trying to close again
    job[0].stderr_fd = -1; // prevent FreeJob from trying to close again
  } else {
    /* read stdout buffer */
    if (DEBUG) fprintf (stderr, "read buffer 0: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
    while ((Nread = ReadtoIOBuffer (&job[0].stdout_buff, job[0].stdout_fd)) > 0);
    switch (Nread) {
      case -2:  /* error in read (programming error?  system level error?) */
	gprint (GP_ERR, "serious IO error\n");
	exit (2);
      case -1:  /* no data in pipe */
      case 0:   /* pipe is closed, change child state? **/
      default:  /* data in pipe */
	// fprintf (stderr, "read %d bytes (Nblock: %d, Nbuffer: %d)\n", Nread, job[0].stdout.Nblock, job[0].stdout.Nbuffer);
	break;
    }
    if (DEBUG) fprintf (stderr, "read buffer 1: (%zx) %d of %d\n", (size_t) job[0].stdout_buff.buffer, job[0].stdout_buff.Nbuffer, job[0].stdout_buff.Nalloc); 
  
    /* read stderr buffer */
    while ((Nread = ReadtoIOBuffer (&job[0].stderr_buff, job[0].stderr_fd)) > 0);
    switch (Nread) {
      case -2:  /* error in read (programming error?  system level error?) */
	gprint (GP_ERR, "serious IO error\n");
	exit (2);
      case -1:  /* no data in pipe */
      case 0:   /* pipe is closed, change child state? **/
      default:  /* data in pipe */
	break;
    }
  }
  return (TRUE);
}

int CheckLocalJobStatus (Job *job) {

  int result, waitstatus;

  /* check local job status */
  result = waitpid (job[0].pid, &waitstatus, WNOHANG);
  switch (result) {
    case -1:  /* error with waitpid */
      switch (errno) {
	case ECHILD:
	  gprint (GP_ERR, "unknown PID, not a child proc\n");
	  gprint (GP_ERR, "did process already exit?  programming error?\n");
	  job[0].state = JOB_NONE;
	  job[0].exit_status = 0;
	  return (FALSE);
	case EINVAL:
	  gprint (GP_ERR, "error EINVAL (waitpid): programming error\n");
	  exit (1);
	case EINTR:
	  gprint (GP_ERR, "error EINTR (waitpid): programming error\n");
	  exit (1);
	default:
	  gprint (GP_ERR, "unknown error for waitpid (%d): programming error\n", errno);
	  exit (1);
      }
      break;
      
    case 0:  /* process not exited */
      job[0].state = JOB_BUSY;
      job[0].exit_status = 0;
      return (TRUE);

    default:
      if (result != job[0].pid) {
	gprint (GP_ERR, "waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, job[0].pid);
	exit (1);
      }
      if (WIFEXITED(waitstatus)) {
	job[0].state = JOB_EXIT;
	job[0].exit_status = WEXITSTATUS(waitstatus);
      }
      if (WIFSIGNALED(waitstatus)) {
	job[0].state = JOB_CRASH;
	job[0].exit_status = WTERMSIG(waitstatus);
      }
      if (WIFSTOPPED(waitstatus)) {
	gprint (GP_ERR, "waitpid returns 'stopped': programming error\n");
	exit (1);
      }
      job[0].dtime = GetTaskTimer (job[0].start, FALSE);
      break;
  }
  return (FALSE);
}

/* this could be written a just a one-way pipe */
int SubmitLocalJob (Job *job) {

  int status, pid;
  int stdout_fd[2], stderr_fd[2];

  bzero (stdout_fd, 2*sizeof(int));
  bzero (stderr_fd, 2*sizeof(int));

  if (pipe (stdout_fd) < 0) goto pipe_error;
  if (pipe (stderr_fd) < 0) goto pipe_error;

  // XXX nothing to be read at this point
  // other threads are already halted here.
  fflush (stdout);
  fflush (stderr);

  pid = fork ();
  if (pid == -1) {
    gprint_syserror (GP_ERR, errno, "error starting local job: ");
    goto pipe_error;
  }

  if (!pid) { /* must be child process */
    if (VerboseMode()) gprint (GP_ERR, "starting local job\n");

    /* close the other ends of the pipes */
    close (stdout_fd[0]);
    close (stderr_fd[0]);

    // XXX neither of these work to empty the child stdout buffer
    // fflush (stdout);
    // fflush (stderr);
    // close (STDOUT_FILENO);
    // close (STDERR_FILENO);

    /* tie our ends of the pipes to stdin, stdout, stderr */
    dup2 (stdout_fd[1], STDOUT_FILENO);
    dup2 (stderr_fd[1], STDERR_FILENO);

    /* set all three unblocking */
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

    // XXX allow the parent time to read the stdout/stderr buffers
    usleep (10000);

    status = execvp (job[0].argv[0], job[0].argv); 
    exit (1);
  }
  if (VerboseMode()) gprint (GP_ERR, "local job launched\n");

  /* set nice level for the child process -- maybe I should not exit here... */
  if (job[0].nicelevel) {
      status = setpriority (PRIO_PROCESS, pid, job[0].nicelevel);
      if (status == -1) {
	  gprint (GP_ERR, "error setting nice level\n");
	  perror ("setpriority: ");
	  exit (2);
      }
  }

  /* close the other ends of the pipes */
  close (stdout_fd[1]); stdout_fd[1] = 0;
  close (stderr_fd[1]); stderr_fd[1] = 0;

  /* make the pipes non-blocking */
  fcntl (stdout_fd[0], F_SETFL, O_NONBLOCK);
  fcntl (stderr_fd[0], F_SETFL, O_NONBLOCK);

  // XXX There seems to always be extra data on the pipe, specifically the 
  // stdout buffer from the parent.  If I read it here, then it clears out that data.
  // But, how can I be sure I will not start reading data from the exec-ed process?

  { // test read of the stdout buffer
    int Nread;
    char buffer[0x1000];

    Nread = read (stdout_fd[0], buffer, 0x1000);
    if (DEBUG) fprintf (stderr, "read from stdout before exec: %d bytes\n", Nread);

    Nread = read (stderr_fd[0], buffer, 0x1000);
    if (DEBUG) fprintf (stderr, "read from stderr before exec: %d bytes\n", Nread);
  }

  job[0].stdout_fd = stdout_fd[0];
  job[0].stderr_fd = stderr_fd[0];
  job[0].pid = pid;

  return (TRUE);

pipe_error:
  perror ("pipe error:");
  if (stdout_fd[0] != 0) close (stdout_fd[0]);
  if (stdout_fd[1] != 0) close (stdout_fd[1]);
  if (stderr_fd[0] != 0) close (stderr_fd[0]);
  if (stderr_fd[1] != 0) close (stderr_fd[1]);
  return (FALSE);
}

/* should this function close the fd's? */
int KillLocalJob (Job *job) {

  int i, result, waitstatus;

  if (job[0].state != JOB_BUSY) return (TRUE);

  /* send SIGTERM signal to job */
  kill (job[0].pid, SIGTERM);
  result = 0;
  for (i = 0; (i < 10) && (result == 0); i++) {
    usleep (10000);  // wait for job to exit
    result = waitpid (job[0].pid, &waitstatus, WNOHANG);
  }
  if (result) return (TRUE);

  /* send SIGKILL signal to job */
  kill (job[0].pid, SIGKILL);
  result = 0;
  for (i = 0; (i < 10) && (result == 0); i++) {
    usleep (10000);  // wait for job to exit
    result = waitpid (job[0].pid, &waitstatus, WNOHANG);
  }
  if (result) return (TRUE);

  /* total failure, don't reset */
  return (FALSE);
}

