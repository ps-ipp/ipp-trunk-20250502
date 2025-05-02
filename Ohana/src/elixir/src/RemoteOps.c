# include "elixir.h"

void HaltElixir (char *pidfile) {
  
  pid_t pid;
  char username[256], machine[256];
  char line[512];
  int i, wsock, rsock;
  struct stat filestat;

  if (!LoadPID (pidfile, &pid, username, machine)) {
    fprintf (stderr, "elixir is not running\n");
    exit (0);
  }

  /* set signal to remote machine */
  if (!rconnect_elixir (machine, CONNECT, &rsock, &wsock)) {
    fprintf (stderr, "can't make connection to machine %s to kill process\n", machine);
    exit (1);
  }
  sprintf (line, "kill -USR1 %d\n", pid);
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }

  for (i = 0; i < 100; i++) {
    if (stat (pidfile, &filestat) == -1) exit (0);
    usleep (100000);
  }
  fprintf (stderr, "elixir is still running\n");
  exit (2);
}

void KillElixir (char *pidfile) {
  
  pid_t pid;
  char username[256], machine[256];
  char line[512];
  int i, wsock, rsock;
  struct stat filestat;

  if (!LoadPID (pidfile, &pid, username, machine)) {
    fprintf (stderr, "elixir is not running\n");
    exit (0);
  }

  /* send signal to remote machine */
  if (!rconnect_elixir (machine, CONNECT, &rsock, &wsock)) {
    fprintf (stderr, "can't make connection to machine %s to kill process\n", machine);
    exit (1);
  }
  sprintf (line, "kill -TERM %d\n", pid);
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }

  for (i = 0; i < 100; i++) {
    if (stat (pidfile, &filestat) == -1) exit (0);
    usleep (100000);
  }
  fprintf (stderr, "elixir is still running\n");
  exit (2);
}

void StatusElixir (char *pidfile, char *msgfile) {
  
  pid_t pid;
  char username[256], machine[256], response[256], message[512];
  char *answer;
  char line[512];
  int i, done, status, wsock, rsock;
  struct stat filestat;
  struct timeval now, then;

  if (!LoadPID (pidfile, &pid, username, machine)) {
    fprintf (stderr, "elixir is not running\n");
    exit (0);
  }

  sprintf (response, "%s.XXXXXX", msgfile);
  if (mkstemp (response) == -1) {
    fprintf (stderr, "can't make temp file\n");
    exit (1);
  }
  sprintf (message, "STATUS %s", response);
  if (VERBOSE) fprintf (stderr, "sending message: %s\n", message);
  WriteMsg (msgfile, message);

  /* send signal to remote machine */
  if (!rconnect_elixir (machine, CONNECT, &rsock, &wsock)) {
    fprintf (stderr, "can't make connection to machine %s to signal process\n", machine);
    exit (1);
  }
  sprintf (line, "kill -USR2 %d\n", pid);
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }

  /* wait (2 sec) for file to exist, then try to read it */
  for (i = 0; ((status = stat (response, &filestat)) == -1) && (i < 20); i++) {
    if (VERBOSE) fprintf (stderr, "waiting for response: %d\n", status);
    usleep (100000);
  }
  if (i >= 20) {
    fprintf (stderr, "no response\n");
    exit (2);
  }

  done = FALSE;
  gettimeofday (&then, (void *) NULL);
  if (VERBOSE) fprintf (stderr, "reading message: %s\n", message);
  while (!done) {
    status = ReadMsg (response, &answer);
    switch (status) {
    case 1:
      done = TRUE;
      break;
    default:
      gettimeofday (&now, (void *) NULL);
      if (DTIME (now, then) > 5.0) {
	fprintf (stderr, "no response from elixir\n");
	exit (2);
      }
    }
    usleep (10000);
  }
  fprintf (stderr, "%s\n", answer);
  gettimeofday (&now, (void *) NULL);
  if (VERBOSE) fprintf (stderr, "response in %f\n", DTIME (now, then));

  unlink (response);
  exit (0);
}

int HalttoRestart (char *pidfile) {
  
  pid_t pid;
  char username[256], machine[256];
  char line[512];
  int i, wsock, rsock;
  struct stat filestat;

  if (!LoadPID (pidfile, &pid, username, machine)) {
    fprintf (stderr, "previous elixir not running\n");
    return (TRUE);
  }
  /* check username matches: can only send signals to own process */

  /* make connection to remote machine */
  if (!rconnect_elixir (machine, CONNECT, &rsock, &wsock)) {
    fprintf (stderr, "can't make connection to machine %s to kill process\n", machine);
    exit (1);
  }
	     
  /* send TERM signal */
  sprintf (line, "kill -TERM %d\n", pid);
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }

  /* wait for cleanup to finish */
  for (i = 0; i < 100; i++) {
    if (stat (pidfile, &filestat) == -1) {
      fprintf (stderr, "previous elixir halted\n");
      goto success;
    }
    usleep (100000);
  }

  /* kill meanly */
  sprintf (line, "kill -KILL %d", pid);
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }

  unlink (pidfile);

  /* test if process is still alive? */
 success:
  sprintf (line, "exit\n");
  if (write (wsock, line, strlen (line)) != strlen(line)) {
    fprintf (stderr, "error sending signal\n");
    exit (1);
  }
  close (wsock);
  close (rsock);
  return (TRUE);
  
}

int LoadPID (char *file, pid_t *pid, char *username, char *machine) {

  int t1, t2, t3;
  FILE *f;

  f = fopen (file, "r");
  if (f == (FILE *) NULL) { 
    return (FALSE);
  }

  t1 = fscanf (f, "%*s %d", pid);
  t2 = fscanf (f, "%*s %s", username);
  t3 = fscanf (f, "%*s %s", machine);
  if ((t1 != 1) || (t2 != 2) || (t3 != 1)) {
    fprintf (stderr, "error reading pid info\n");
  }
  fclose (f);

  return (TRUE);
}
