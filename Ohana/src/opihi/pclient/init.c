# include "pclient.h"

int job	            PROTO((int, char **));
int reset           PROTO((int, char **));
int check           PROTO((int, char **));
int status	    PROTO((int, char **));
int stdout_pclient  PROTO((int, char **));
int stderr_pclient  PROTO((int, char **));
int version         PROTO((int, char **));

static Command cmds[] = {  
  {1, "job",       job,      "start job"},
  {1, "reset",     reset,    "reset job"},
  {1, "check",     check,    "check job"},
  {1, "status",    status,   "check job status"},
  {1, "stdout",    stdout_pclient,   "get stdout buffer"},
  {1, "stderr",    stderr_pclient,   "get stderr buffer"},
  {1, "version",   version,      "show version information"},
}; 

void InitPclient () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
  InitChild ();
}

void FreePclient () {
  FreeChild ();
}
