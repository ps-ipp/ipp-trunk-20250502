# include "nightd.h"

int StartUp () {

  int i;
  int night, NightTime;
  char user[256], machine[256];
  float time;
  pid_t pid;

  if (!SetPID (&pid, user, machine)) {
    fprintf (stderr, "%s apparently running:\n\n", Program);
    fprintf (stderr, "  machine: %s\n", machine);
    fprintf (stderr, "  user: %s\n", user);
    fprintf (stderr, "  PID: %d\n", pid);
    fprintf (stderr, "  remove %s if %s has died unexpectedly\n", PIDFile, Program);
    exit (1);
  }

  SetSignals ();

  SuspendAction = FALSE;
  NightTime = FALSE;

  /* Event Loop */
  while (1) {

    if (SuspendAction) {
      usleep (100000);
      continue;
    }

    /* events happen at time % PERIOD */
    WaitForPeriod ();

    /* time should be UTCmidnight - UTCmidnight + 24 */
    GetDateTime (DateStr, TimeStr, &time);
    night = (time > NightStart) && (time < NightStop);

    /* start of night */
    if (night && !NightTime) {
      NightTime = TRUE;
      fprintf (LogFile, "beginning of night %s, running INIT commands\n", DateStr);
      fflush (LogFile);
      for (i = 0; InitCommand[i][0]; i++) {
	DoCommand (InitCommand[i], "INIT");
      }
    }

    /* end of night */
    if (!night && NightTime) {
      NightTime = FALSE;
      fprintf (LogFile, "end of night %s, running DONE commands\n", DateStr);
      fflush (LogFile);
      for (i = 0; DoneCommand[i][0]; i++) {
	DoCommand (DoneCommand[i], "DONE");
      }
    }

    /* main event */
    if (NightTime) {
      for (i = 0; MainCommand[i][0]; i++) {
	DoCommand (MainCommand[i], "MAIN");
      }
    }

  }
}

/* 
   SuspendAction - loop without operations, used for testing
   NightStart    - time in hours of beginning of night
   NightStop     - time in hours of end of night
   NightTime     - current night / day state
*/
