# include "elixir.h"

static struct timeval then = {0,0};
static char *MessageFile = (char *) NULL;

void SetMessageFile (char *filename) {
  MessageFile = filename;
}

int CheckMessages () {
  
  int status;
  struct timeval now;
  char *message, *p, *p2;
  char file[256], cmd[64];

  /* we should only do this check every 200ms or so */
  gettimeofday (&now, (void *) NULL);
  if (DTIME (now, then) < 0.2) return (1);

  if (MessageFile == (char *) NULL) {
    fprintf (stderr, "not ready for messages\n");
    return (FALSE);
  }

  status = WaitMsg (MessageFile, &message, 0.1);
  then = now;
  if (!status) return (FALSE);

  /* loop over all lines in message */
  p = message;
  while (strlen (p) > 0) {
    p2 = strchr (p, '\n');
    if (p2 == (char *) NULL) {
      p2 = p + strlen (p) - 1;
    } else {
      *p2 = 0;
    }

    sscanf (p, "%s %s", cmd, file);

    if (!strcasecmp (cmd, "ALIVE"))  {
      WriteMsg (file, "BUSY");
    }
    if (!strcasecmp (cmd, "TIMES")) {
      DumpProcessTimes (file);
    }
    if (!strcasecmp (cmd, "STATUS")) {
      DumpStatus (file); 
    }
    if (!strcasecmp (cmd, "STOP"))   {
      WriteMsg (file, "Elixir will end when processes are done");
      ElixirStop ();
    }
    if (!strcasecmp (cmd, "ABORT"))  {
      WriteMsg (file, "Elixir is exiting without finishing");
      Shutdown (1);
    }

    p = p2 + 1;

  }

  return (TRUE);

}
