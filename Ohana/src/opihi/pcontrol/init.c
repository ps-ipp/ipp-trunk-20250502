# include "pcontrol.h"

int check       PROTO((int, char **));
int delete      PROTO((int, char **));
int host        PROTO((int, char **));
int hoststack   PROTO((int, char **));
int job	        PROTO((int, char **));
int jobstack    PROTO((int, char **));
int kill_pc     PROTO((int, char **));
int machines    PROTO((int, char **));
int parameters  PROTO((int, char **));
int run         PROTO((int, char **));
int status      PROTO((int, char **));
int stderr_pc   PROTO((int, char **));
int stdout_pc   PROTO((int, char **));
int verbose     PROTO((int, char **));
int version     PROTO((int, char **));

// pulse is only available in the un-threaded version
int pulse       PROTO((int, char **));

static Command cmds[] = {  
  {1, "check",      check,      "get job or host status"},
  {1, "delete",     delete,     "delete job"},
  {1, "host",       host,       "add / delete / modify host"},
  {1, "hoststack",  hoststack,  "list hosts for a single stack"},
  {1, "job",        job,        "add job"},
  {1, "jobstack",   jobstack,   "list jobs for a single stack"},
  {1, "kill",       kill_pc,    "kill job"},
  {1, "machines",   machines,   "list machines"},
  {1, "parameters", parameters, "get / set system parameters"},
  {1, "run",        run,        "set controller runlevel"},
  {1, "status",     status,     "get system status"},
  {1, "stderr",     stderr_pc,  "get stderr buffer for job"},
  {1, "stdout",     stdout_pc,  "get stdout buffer for job"},
  {1, "stop",       run,        "stop controller processing"},
  {1, "verbose",    verbose,    "set the verbose mode for job"},
  {1, "version",    version,    "show version information"},
# ifndef THREADED   	        
  {1, "pulse",      pulse,      "set system pulse"},
# endif
}; 

void InitPcontrol () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
  InitJobStacks ();
  InitHostStacks ();
  InitMachines ();
}

void FreePcontrol () {
  FreeJobStacks ();
  FreeHostStacks ();
  FreeMachines ();
}
