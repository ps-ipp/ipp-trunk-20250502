# include "nightd.h"

int main (int argc, char **argv) {

  Program = filebasename (argv[0]);

  LogFile = stderr;
  ConfigInit (&argc, argv);  
  
  if (argc != 2) {
    fprintf (stderr, "USAGE: %s [mode]\n", Program);
    fprintf (stderr, " mode = start, stop, status, config\n");
    exit (2);
  }

  if (!strcasecmp(argv[1], "status")) GetStatus ();
  if (!strcasecmp(argv[1], "config")) ResetConfig ();
  if (!strcasecmp(argv[1], "stop"))   SendShutdown ();

  /* execute in background, send output to logfile */
  LogFile = fopen (logfile, "w");
  if (LogFile == (FILE *) NULL) {
    LogFile = stderr;
    fprintf (LogFile, "can't open Log File %s, writing to stderr\n", logfile);
  }
  if (!strcasecmp(argv[1], "start"))  StartUp ();
  /* if (!strcasecmp(argv[1], "test"))   TestMode (); */

  fprintf (stderr, "invalid %s command %s\n", Program, argv[1]);
  exit (2);

}

