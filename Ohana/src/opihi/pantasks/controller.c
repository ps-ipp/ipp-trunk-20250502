# include "pantasks.h"

int controller_host    	  PROTO((int, char **));
int controller_exit    	  PROTO((int, char **));
int controller_status  	  PROTO((int, char **));
int controller_jobstack   PROTO((int, char **));
int controller_hoststack  PROTO((int, char **));
int controller_machines   PROTO((int, char **));
int controller_parameters PROTO((int, char **));
int controller_verbose    PROTO((int, char **));
int controller_version    PROTO((int, char **));
int controller_run     	  PROTO((int, char **));
int controller_stop    	  PROTO((int, char **));
int controller_check   	  PROTO((int, char **));
int controller_output  	  PROTO((int, char **));
int controller_pulse   	  PROTO((int, char **));

static Command controller_cmds[] = {
  {1, "check",     controller_check,      "check controller host/job"},
  // {1, "delete",    controller_delete,   "delete job on controller"},
  {1, "exit",      controller_exit,       "shutdown controller"},
  {1, "host",      controller_host,       "define host for controller"},
  {1, "hoststack", controller_hoststack,  "define host for controller"},
  // {1, "job",       controller_job,      "add jobs to controller"},
  {1, "jobstack",  controller_jobstack,   "check controller status"},
  {1, "machines",  controller_machines,   "print controller machine status"},
  {1, "parameters",controller_parameters, "modify controller parameters"},
  {1, "output",    controller_output,     "print controller output"},
  {1, "run",       controller_run,        "start controller operation / set run levels"},
  {1, "status",    controller_status,     "check controller status"},
  {1, "stop",      controller_run,        "stop controller (no disconnect)"},
  {1, "verbose",   controller_verbose,    "set controller verbosity"},
  {1, "version",   controller_version,    "show controller version"},
  {1, "pulse",     controller_pulse,      "set controller pulse"},
};

int controller (int argc, char **argv) {

  int i, N, status;
  CommandF *func;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: controller (command) ... \n");
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "help")) {
    N = sizeof (controller_cmds) / sizeof (Command);

    for (i = 0; i < N; i++) {
      gprint (GP_LOG, "%-15s %s\n", controller_cmds[i].name, controller_cmds[i].help);
    }
    return (TRUE);
  }

  func = FindControllerCommand (argv[1]);
  if (func == NULL) {
    gprint (GP_ERR, "invalid controller command\n");
    return (FALSE);
  }

  ControlLock(__func__);
  status = (*func)(argc - 1, argv + 1);
  ControlUnlock(__func__);
  return (status);
}

CommandF *FindControllerCommand (char *cmd) {

  int i, N;

  N = sizeof (controller_cmds) / sizeof (Command);

  for (i = 0; i < N; i++) {
    if (!strcmp (controller_cmds[i].name, cmd)) {
      return (controller_cmds[i].func);
    }
  }
  return (NULL);
}
