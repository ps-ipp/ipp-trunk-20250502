# include "pantasks.h"

/* most of these are defined in cmd.basic */
int quit            PROTO((int, char **));
int input      	    PROTO((int, char **));
int module     	    PROTO((int, char **));
int server_run	    PROTO((int, char **));
int server_stop	    PROTO((int, char **));
int server_halt	    PROTO((int, char **));
int cd              PROTO((int, char **));
int pwd        	    PROTO((int, char **));
int output     	    PROTO((int, char **));

CommandF *FindServerCommand (char *cmd);

static Command server_cmds[] = {
  {1, "exit",   quit,   "shutdown server"},
  {1, "quit",   quit,   "shutdown server"},
  {1, "input",  input,  "load input file on server"},
  {1, "module", module, "load module file on server"},
  {1, "run",    server_run,  "run scheduler"},
  {1, "stop",   server_stop, "stop scheduler"},
  {1, "halt",   server_halt, "halt scheduler"},
  {1, "cd",     cd,     "set local directory"},
  {1, "pwd",    pwd,    "check local directory"},
  {1, "output", output, "set server output destinations"},
};

int server (int argc, char **argv) {

  int i, N, status;
  CommandF *func;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: server (command) ... \n");
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "help")) {
    N = sizeof (server_cmds) / sizeof (Command);

    for (i = 0; i < N; i++) {
      gprint (GP_LOG, "%-15s %s\n", server_cmds[i].name, server_cmds[i].help);
    }
    return (TRUE);
  }

  func = FindServerCommand (argv[1]);
  if (func == NULL) {
    gprint (GP_ERR, "invalid server command\n");
    return (FALSE);
  }

  status = (*func)(argc - 1, argv + 1);
  return (status);
}

CommandF *FindServerCommand (char *cmd) {

  int i, N;

  N = sizeof (server_cmds) / sizeof (Command);

  for (i = 0; i < N; i++) {
    if (!strcmp (server_cmds[i].name, cmd)) {
      return (server_cmds[i].func);
    }
  }
  return (NULL);
}
