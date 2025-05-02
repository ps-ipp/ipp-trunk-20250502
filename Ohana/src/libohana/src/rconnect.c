# include <ohana.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <signal.h>  //man page says include this for kill

# define DEBUG 1

/* connect to host with the command and start the shell: (command) hostname (shell)
   eg: ssh hostname pclient
   stdio is an array of file descriptors for the I/O to the remote shell:
   stdio[3]: 0 == stdin, 1 == stdout, 2 == stderr

   The handshake test assumes that the remote shell can accept an echo command.  If not,
   the user must do their own handshake test.

   return value is the PID of the remote job, or FALSE on failure
   in case of failure, the error condition is provided in errorInfo.

   
*/

/* connection can take a while, allow up to 5 sec by default */
static int CONNECT_TIMEOUT = 5000;
static char *CONNECT_FLAGS = NULL;

void rconnect_set_timeout (int timeout) {
  CONNECT_TIMEOUT = timeout;
}

void rconnect_set_flags (char *flags) {
  CONNECT_FLAGS = flags;
}

int rconnect (char *command, char *hostname, char *shell, int *stdio, int *errorInfo, int doHandshake) {

  int i = 0, stdin_fd[2], stdout_fd[2], stderr_fd[2], status;
  int result, waitstatus;
  pid_t pid;
  char *p;
  char **argv;
  IOBuffer buffer;
  struct timespec request, remain;

  if (errorInfo) *errorInfo = RCONNECT_ERR_NONE;

  myAssert (command != NULL,  "command is NULL");
  myAssert (hostname != NULL, "hostname is NULL");
  myAssert (shell != NULL,    "shell is NULL");
  myAssert (stdio != NULL,    "stdio is NULL");

  bzero (stdin_fd,  2*sizeof(int));
  bzero (stdout_fd, 2*sizeof(int));
  bzero (stderr_fd, 2*sizeof(int));

  if (pipe (stdin_fd)  < 0) goto pipe_error;
  if (pipe (stdout_fd) < 0) goto pipe_error;
  if (pipe (stderr_fd) < 0) goto pipe_error;

  // make this a user option?
  // char *flag = "-x -o PasswordAuthentication=No";

  ALLOCATE (argv, char *, 7);
  argv[0] = command;
  argv[1] = "-x";
  argv[2] = "-o";
  argv[3] = "PasswordAuthentication=no";
  argv[4] = hostname;
  argv[5] = shell;
  argv[6] = 0;

  pid = fork ();
  if (pid < 0) {
    perror ("fork:");
    fprintf (stderr, "error forking child process\n");
    exit (3);
  }
  if (!pid) { /* must be child process */
    if (DEBUG) fprintf (stderr, "starting remote connection to %s...", hostname);

    /* close the other ends of the pipes */
    close (stdin_fd[1]);
    close (stdout_fd[0]);
    close (stderr_fd[0]);

    /* tie our ends of the pipes to stdin, stdout, stderr */
    dup2 (stdin_fd[0],  STDIN_FILENO);
    dup2 (stdout_fd[1], STDOUT_FILENO);
    dup2 (stderr_fd[1], STDERR_FILENO);

    /* set all three unblocking */
    setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

    status = execvp (argv[0], argv);
    if (DEBUG) fprintf (stderr, "error starting remote shell process\n");
    if (errorInfo) *errorInfo = RCONNECT_ERR_EXEC;
    return FALSE;
  }
  free (argv);

  /* close the other ends of the pipes */
  close (stdin_fd[0]);  stdin_fd[0]  = 0;
  close (stdout_fd[1]); stdout_fd[1] = 0;
  close (stderr_fd[1]); stderr_fd[1] = 0;

  /* make the pipes non-blocking */
  fcntl (stdin_fd[1],  F_SETFL, O_NONBLOCK);
  fcntl (stdout_fd[0], F_SETFL, O_NONBLOCK);
  fcntl (stderr_fd[0], F_SETFL, O_NONBLOCK);

  if (doHandshake) {
    /* perform handshake with shell to verify alive & running */
    InitIOBuffer (&buffer, 0x100);

    /* send handshake command */
    status = write_fmt (stdin_fd[1], "echo CONNECTED\n");
    if ((status == -1) && (errno == EPIPE)) goto connect_error;

    /* try to get evidence connection is alive - wait upto a few seconds */
    p = NULL;
    status = -1;
    for (i = 0; (i < CONNECT_TIMEOUT) && (status != 0) && (p == NULL); i++) {
      status = ReadtoIOBuffer (&buffer, stdout_fd[0]);
      p = memstr (buffer.buffer, "CONNECTED", buffer.Nbuffer);
      usleep (10000); // wait for client to be connected
    }
    if (status == 0) goto connect_error;
    if (status == -1) goto connect_error;
    if (DEBUG) fprintf (stderr, "%d cycles to connect\n", i);
    FreeIOBuffer (&buffer);
  }

  if (DEBUG) fprintf (stderr, "Connected\n");

  stdio[0] = stdin_fd[1];
  stdio[1] = stdout_fd[0];
  stdio[2] = stderr_fd[0];

  if (errorInfo) *errorInfo = RCONNECT_ERR_NONE;
  return (pid);

pipe_error:
  perror ("pipe error:");
  if (errorInfo) *errorInfo = RCONNECT_ERR_PIPE;
  goto close_pipes;

connect_error:
  if (DEBUG) fprintf (stderr, "error while connecting, status: %d, ncycles: %d\n", status, i);

  /* avoid blocking on waitpid, test every 100 usec, up to 50 msec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  /* harvest the child process: kill & wait (< 100 ms) for exit */
  kill (pid, SIGKILL);
  result = waitpid (pid, &waitstatus, WNOHANG);
  for (i = 0; (i < 50) && (result == 0); i++) {
    nanosleep (&request, &remain);
    result = waitpid (pid, &waitstatus, WNOHANG);
  }

  if ((result == -1) && (errno != ECHILD)) {
    fprintf (stderr, "unexpected error from waitpid (%d): programming error\n", errno);
    abort();
  }
  if (result == 0) {
    if (DEBUG) fprintf (stderr, "child did not exit (rconnect)??");
  }
  if (result > 0) {
    myAssert (result == pid, "waitpid error: mis-matched PID (%d vs %d).  programming error", result, pid);
    myAssert (!WIFSTOPPED(waitstatus), "waitpid returns 'stopped': programming error");
  }
  if (errorInfo) *errorInfo = RCONNECT_ERR_CONNECT;

close_pipes:
  if (stdin_fd[0]  != 0) close (stdin_fd[0]);
  if (stdin_fd[1]  != 0) close (stdin_fd[1]);
  if (stdout_fd[0] != 0) close (stdout_fd[0]);
  if (stdout_fd[1] != 0) close (stdout_fd[1]);
  if (stderr_fd[0] != 0) close (stderr_fd[0]);
  if (stderr_fd[1] != 0) close (stderr_fd[1]);
  return (FALSE);
}
