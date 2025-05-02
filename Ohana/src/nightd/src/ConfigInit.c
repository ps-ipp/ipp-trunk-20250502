# include "nightd.h"

static char *file;

int ConfigInit (int *argc, char **argv) {

  char *home;

  /*** load configuration info ***/
  home = getenv ("HOME");
  if (home == (char *) NULL) {
    fprintf (stderr, "can't determine HOME, please set\n");
    exit (2);
  }
  ALLOCATE (file, char, strlen (home) + strlen (Program) + 10);

  sprintf (file, "%s/.%src", home, Program);
  LoadConfig (SIGKILL);
  return (TRUE);
}

void LoadConfig (int sig) {

  int i, Ncmd;
  char *config, *found;
  char line[256];
  double tmp;

  if (sig == SIGUSR1) {
    fprintf (LogFile, "re-loading config file %s\n", file);
    fflush (LogFile);
  }

  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (LogFile, "can't find configuration file %s\n", file);
    exit (2);
  }

  /* need to check for error status */
  ScanConfig (config, "PERIOD",                 "%d", 0, &PERIOD);
  ScanConfig (config, "TIMEOUT",                "%d", 0, &TIMEOUT);

  /* load list of InitCommands */
  found = config;
  Ncmd = 10;
  ALLOCATE (InitCommand, char *, Ncmd);
  for (i = 0; found != NULL; i++) {
    ALLOCATE (InitCommand[i], char, 256);
    bzero (InitCommand[i], 256);
    found = ScanConfig (config, "INIT_COMMAND", "%s", i + 1, InitCommand[i]);
    if (i == Ncmd - 1) {
      Ncmd += 10;
      REALLOCATE (InitCommand, char *, Ncmd);
    }      
  }
  
  /* load list of MainCommands */
  found = config;
  Ncmd = 10;
  ALLOCATE (MainCommand, char *, Ncmd);
  for (i = 0; found != NULL; i++) {
    ALLOCATE (MainCommand[i], char, 256);
    bzero (MainCommand[i], 256);
    found = ScanConfig (config, "MAIN_COMMAND", "%s", i + 1, MainCommand[i]);
    if (i == Ncmd - 1) {
      Ncmd += 10;
      REALLOCATE (MainCommand, char *, Ncmd);
    }      
  }

  /* load list of DoneCommands */
  found = config;
  Ncmd = 10;
  ALLOCATE (DoneCommand, char *, Ncmd);
  for (i = 0; found != NULL; i++) {
    ALLOCATE (DoneCommand[i], char, 256);
    bzero (DoneCommand[i], 256);
    found = ScanConfig (config, "DONE_COMMAND", "%s", i + 1, DoneCommand[i]);
    if (i == Ncmd - 1) {
      Ncmd += 10;
      REALLOCATE (DoneCommand, char *, Ncmd);
    }      
  }

  ScanConfig (config, "PID_FILE",               "%s", 0, PIDFile);
  ScanConfig (config, "LOG_FILE",               "%s", 0, logfile);
    
  ScanConfig (config, "NIGHT_START",            "%s", 0, line);
  if (!ohana_dms_to_ddd (&tmp, line)) { 
    fprintf (LogFile, "format error in NIGHT_START\n");
    exit (2);
  } 
  NightStart = tmp;
  if (NightStart < 12.0) {
    fprintf (LogFile, "warning: night starts before noon!\n");
    fprintf (LogFile, "if you live in the Arctic Circle (or Antarctic), you might need to adjust the program\n");
    exit (2);
  }
  ScanConfig (config, "NIGHT_STOP",             "%s", 0, line);
  if (!ohana_dms_to_ddd (&tmp, line)) { 
    fprintf (LogFile, "format error in NIGHT_STOP\n");
    exit (2);
  } 
  NightStop = tmp;
  if (NightStop > 12.0) {
    fprintf (LogFile, "warning: night ends after noon!\n");
    fflush (LogFile);
  }
  if (NightStop < 12.0) NightStop += 24.0;

  /* check format of command calls: must have a certain set of %d, %s, etc */
  free (config);

}
