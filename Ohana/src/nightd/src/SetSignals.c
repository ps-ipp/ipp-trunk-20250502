# include "nightd.h"

int SetSignals () {

  signal (SIGTERM, Shutdown);
  signal (SIGUSR1, LoadConfig);
  signal (SIGUSR2, ToggleSuspend);
  return (TRUE);
}

void ToggleSuspend (int sig) {

  SuspendAction = SuspendAction ^ TRUE;

}

void Shutdown (int sig) {

  fprintf (LogFile, "shutting down skyprobed\n");
  fflush (LogFile);

  if (unlink (PIDFile)) {
    fprintf (LogFile, "error deleting PID File %s\n", PIDFile);
    exit (1);
  }   

  exit (0);
}
