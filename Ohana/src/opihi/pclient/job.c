# include "pclient.h"
# include <sys/resource.h>

int job (int argc, char **argv) {

  int i, N, pid, status, nicelevel;
  char **targv;

  nicelevel = 0;
  if ((N = get_argument (argc, argv, "-nice"))) {
    remove_argument (N, &argc, argv);
    nicelevel = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: job (arg0) (arg1) ... (argN)\n");
    gprint (GP_LOG, "PCLIENT_PID: %d\n", -2);
    return (FALSE);
  }
  
  if (ChildStatus != PCLIENT_NONE) {
    gprint (GP_ERR, "need to clear existing child\n");
    gprint (GP_LOG, "PCLIENT_PID: %d\n", -3);
    return (FALSE);
  }

  if (pipe (child_stdin_fd)  < 0) goto pipe_error;
  if (pipe (child_stdout_fd) < 0) goto pipe_error;
  if (pipe (child_stderr_fd) < 0) goto pipe_error;

  /* need to define arg list with NULL termination */
  ALLOCATE (targv, char *, argc);
  for (i = 1; i < argc; i++) {
    targv[i-1] = strcreate (argv[i]);
  }
  targv[i-1] = 0;

  pid = fork ();
  if (!pid) { /* must be child process */
    /* gprint (GP_ERR, "starting child process %s...\n", targv[0]); */

    /* close the other ends of the pipes */
    close (child_stdin_fd[1]);
    close (child_stdout_fd[0]);
    close (child_stderr_fd[0]);

    /* tie our ends of the pipes to stdin, stdout, stderr */
    dup2 (child_stdin_fd[0],  STDIN_FILENO);
    dup2 (child_stdout_fd[1], STDOUT_FILENO);
    dup2 (child_stderr_fd[1], STDERR_FILENO);

    /* set all three unblocking */
    setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

    /* exec job */ 
    status = execvp (targv[0], targv); 
    gprint (GP_ERR, "error starting child process: %d\n", status);
    exit (1);
  }

  /* set nice level for the child process */
  if (nicelevel) {
      status = setpriority (PRIO_PROCESS, pid, nicelevel);
      if (status == -1) {
	  gprint (GP_ERR, "error setting nicelevel\n");
	  perror ("setpriority: ");
	  exit (2);
      }
  }

  /* free temporary arg list */
  for (i = 0; i < argc - 1; i++) {
    free (targv[i]);
  }
  free (targv);

  /* close the other ends of the pipes */
  close (child_stdin_fd[0]);
  close (child_stdout_fd[1]);
  close (child_stderr_fd[1]);

  /* make the pipes non-blocking */
  fcntl (child_stdin_fd[1],  F_SETFL, O_NONBLOCK);
  fcntl (child_stdout_fd[0], F_SETFL, O_NONBLOCK);
  fcntl (child_stderr_fd[0], F_SETFL, O_NONBLOCK);
  
  ChildStatus = PCLIENT_BUSY;
  ChildPID = pid;

  gprint (GP_LOG, "PCLIENT_PID: %d\n", ChildPID);
  return (TRUE);

pipe_error:
  perror ("pipe error:");
  if (child_stdin_fd[0]  != 0) close (child_stdin_fd[0]);
  if (child_stdin_fd[1]  != 0) close (child_stdin_fd[1]);
  if (child_stdout_fd[0] != 0) close (child_stdout_fd[0]);
  if (child_stdout_fd[1] != 0) close (child_stdout_fd[1]);
  if (child_stderr_fd[0] != 0) close (child_stderr_fd[0]);
  if (child_stderr_fd[1] != 0) close (child_stderr_fd[1]);

  gprint (GP_LOG, "PCLIENT_PID: %d\n", -1);
  return (FALSE);
}

/* possible responses:

PCLIENT_PID: -1 - pipe error
PCLIENT_PID: -2 - syntax error
PCLIENT_PID: -3 - existing child
PCLIENT_PID: >0 - success (PID)

*/
