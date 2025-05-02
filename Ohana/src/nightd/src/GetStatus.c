# include "nightd.h"

int GetStatus () {

  char user[256], machine[256];
  pid_t pid;

  if (SetPID (&pid, user, machine)) {
    fprintf (stderr, "%s is not running\n", Program);
    unlink (PIDFile);
    exit (1);
  }

  fprintf (stderr, "%s apparently running:\n\n", Program);
  fprintf (stderr, "  machine: %s\n", machine);
  fprintf (stderr, "  user: %s\n", user);
  fprintf (stderr, "  PID: %d\n", pid);
  fprintf (stderr, "  remove %s if %s has died unexpectedly\n", PIDFile, Program);
  exit (0);

}
  
int ResetConfig () {

  char user[MY_MAX_PATH], machine[MY_MAX_PATH], line[MY_MAX_PATH];
  pid_t pid;

  if (SetPID (&pid, user, machine)) {
    fprintf (stderr, "%s is not running\n", Program);
    unlink (PIDFile);
    exit (1);
  }

  snprintf_nowarn (line, MY_MAX_PATH, "rsh %s kill -USR1 %d", machine, pid);
  if (system (line) == -1) {
    fprintf (stderr, "failed to send signal\n");
  }
  exit (0);

}
  
int SendShutdown () {

  char user[MY_MAX_PATH], machine[MY_MAX_PATH], line[MY_MAX_PATH];
  pid_t pid;

  if (SetPID (&pid, user, machine)) {
    fprintf (stderr, "%s is not running\n", Program);
    unlink (PIDFile);
    exit (1);
  }

  snprintf_nowarn (line, MY_MAX_PATH, "rsh %s kill -TERM %d", machine, pid);
  if (system (line) == -1) {
    fprintf (stderr, "failed to send signal\n");
  }
  exit (0);

}
  
