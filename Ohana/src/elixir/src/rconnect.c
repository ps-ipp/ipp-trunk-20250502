# include "elixir.h"

/* rconnect opens a remote shell on hostname and returns two file descriptors:
   one for read, one for write */

// this is a slightly different version than the one in libohana -- use a local name
int rconnect_elixir (char *hostname, char *command, int *rsock, int *wsock) {

  int i, rfd[2], wfd[2], status;
  pid_t pid;
  char buffer[0x4000];
  char *file;
  Fifo fifo;

  InitFifo (&fifo, 0x4000, 0x1000);

  status = pipe (rfd);
  if (status < 0) {
    perror ("pipe");
    return (FALSE);
  }
  status = pipe (wfd);
  if (status < 0) {
    perror ("pipe");
    return (FALSE);
  }

  file = filebasename (command);
  pid = fork ();
  if (!pid) { /* must be child process */
    fprintf (stderr, "starting remote connection to %s...", hostname);
    /* close the excess sockets */
    close (wfd[1]);
    close (rfd[0]);
    dup2 (wfd[0], STDIN_FILENO);
    dup2 (rfd[1], STDOUT_FILENO);
    dup2 (rfd[1], STDERR_FILENO);
    setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);
    fprintf (stderr, "child is spawned\n");

    status = execl (command, file, hostname, "/bin/csh", NULL); 
    fprintf (stderr, "error starting remote shell process\n");
    Shutdown (1);
  }
  *wsock = wfd[1];
  *rsock = rfd[0];
  close (wfd[0]);
  close (rfd[1]);
   
  fcntl (*rsock, F_SETFL, O_NONBLOCK);
  fcntl (*wsock, F_SETFL, O_NONBLOCK);

  sprintf (buffer, "echo PTOLEMY STARTED\n");
  status = write (*wsock, buffer, strlen(buffer));
  if ((status == -1) && (errno == EPIPE)) {
    fprintf (stderr, "socket closed unexpectedly\n");
    close (*wsock);
    close (*rsock);
    return (FALSE);
  }

  /* try to get evidence connection is alive - wait upto a few seconds */
  status = -1;
  for (i = 0; (i < 300) && (status == -1); i++) {
    fcntl (*rsock, F_SETFL, O_NONBLOCK);
    status = SockScan ("PTOLEMY STARTED", &fifo, *rsock);
    if (!(i % 30)) fprintf (stderr, ".");
    if (status == 0) {
      fprintf (stderr, "socket closed unexpectedly\n");
      close (*wsock);
      close (*rsock);
      return (FALSE);
    }
  }
  if (status == -1) {
    fprintf (stderr, "timeout while connecting\n");
    close (*wsock);
    close (*rsock);
    return (FALSE);
  }
  fprintf (stderr, "Connected\n");

  /* the onintr command works with csh/tcsh type shells to
     prevent trapping of SIGTERM, used to kill hung jobs.
     for bash/sh type shells, you can use SIGQUIT instead
     without setting onintr */
  sprintf (buffer, "onintr\n");
  status = write (*wsock, buffer, strlen(buffer));
  if ((status == -1) && (errno == EPIPE)) {
    fprintf (stderr, "socket closed unexpectedly\n");
    close (*wsock);
    close (*rsock);
    return (FALSE);
  }

  FreeFifo (&fifo);
  return (pid);

}
