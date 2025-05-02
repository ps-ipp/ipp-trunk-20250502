# include "basic.h"
# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))

static char *defshell = "/bin/sh";
static char *cmdflag = "-c";

// XXX add the ability to exec without a separate shell
// XXX add an option to modify the timeout

int shell (int argc, char **argv) {
  
  int i, pid, N;
  int exit_status;
  int wait_status;
  int result;
  char **args, *shell;
  struct timeval start, now;
  float timeout;

  timeout = 0;
  if ((N = get_argument (argc, argv, "-timeout"))) {
    remove_argument (N, &argc, argv);
    timeout = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  shell = getenv ("SHELL");
  if (shell == NULL) shell = defshell;

  // we are creating a command of the form /bin/sh -c argv[1] argv[2] etc, where
  // the argv[1], etc elements are concatenated into a single string
  ALLOCATE (args, char *, 4);
  args[0] = shell;
  args[1] = cmdflag;
  args[2] = paste_args (argc - 1, &argv[1]);
  args[3] = NULL;

  // send the commands to the shell specified in the env variable SHELL, or else /bin/sh

  gettimeofday (&start, NULL);

  // use execvp to enable a timeout on the system call 
  pid = fork ();
  if (!pid) { /* must be child process */
    execvp (shell, args);
    exit (1);
  }
  free (args[2]);
  free (args);
  
  // wait for process to finish or timeout
  // loop forever if desired, but catch C-C and stop the process on interrupt
  struct sigaction *old_sigaction = SetInterrupt();
  while (!interrupt) {
    result = waitpid (pid, &wait_status, WNOHANG);
    switch (result) {
      case -1:   // error on waitpid
	switch (errno) {
	  case ECHILD:
	    gprint (GP_ERR, "unknown PID, not a child process: %d\n", pid);
	    ClearInterrupt (old_sigaction);
	    return (FALSE);
	  default:
	    gprint (GP_ERR, "unexpected response to waitpid: %d\n", result);
	    abort();
	}
	break;

      case 0:   // child not yet exited
	usleep (10000);
	if (timeout > 0.0) {
	  gettimeofday (&now, NULL);
	  if (DTIME(now, start) > timeout) {
	    gprint (GP_ERR, "timeout on %s (pid %d)\n", argv[1], pid);
	    ClearInterrupt (old_sigaction);
	    return (FALSE);
	  }
	}
	continue;

      default:
	if (result != pid) {
	  gprint (GP_ERR, "waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, pid);
	  abort();
	}
	if (WIFEXITED(wait_status)) {
	  ClearInterrupt (old_sigaction);
	  exit_status = WEXITSTATUS(wait_status);
	  if (exit_status) {
	    return FALSE;
	  } else {
	    return TRUE;
	  }
	}
	if (WIFSIGNALED(wait_status)) {
	  ClearInterrupt (old_sigaction);
	  gprint (GP_ERR, "job %d exited on signal %d\n", pid, WTERMSIG(wait_status));
	  return (FALSE);
	}
	if (WIFSTOPPED(wait_status)) {
	  gprint (GP_ERR, "waitpid returns 'stopped' programming error\n");
	  abort();
	}
    }
  }
  ClearInterrupt (old_sigaction);
  gprint (GP_ERR, "caught interrupt, killing %s (%d)\n", argv[1], pid);

  // user hit interrupt: kill the process and return
  kill (pid, SIGKILL);
  result = 0;
  for (i = 0; (i < 10) && (result == 0); i++) {
    usleep (10000);  // wait for job to exit
    result = waitpid (pid, &wait_status, WNOHANG);
  }
  if (!result) {
    gprint (GP_ERR, "trouble killing %s (pid %d)\n", argv[1], pid);
  } else {
    gprint (GP_ERR, "killed %s (pid %d)\n", argv[1], pid);
  }

  return (FALSE);
}
