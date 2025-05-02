# include "relphot.h"

// I'm getting unlogged errors and failures.  I need a log ouput that the clients can
// write independent of the master

static FILE *logfile = NULL;
int client_logger_init (char *dirname) {

  char filename[DVO_MAX_PATH];

  snprintf (filename, DVO_MAX_PATH, "%s/log.rlpc.XXXXXX", dirname);
    
  int fd = mkstemp (filename);
  if (fd == -1) {
    fprintf (stderr, "failed to open client logger %s, exiting\n", filename);
    exit (50);
  }

  logfile = fdopen (fd, "w");
  if (!logfile)  {
    fprintf (stderr, "failed to fdopen client logger, exiting\n");
    exit (51);
  }
  return TRUE;
}

int client_logger_message (char *format,...) {

  if (!logfile) return FALSE;

  va_list argp;

  va_start (argp, format);
  vfprintf (logfile, format, argp);
  va_end (argp);

  fflush (logfile);
  return TRUE;
}
