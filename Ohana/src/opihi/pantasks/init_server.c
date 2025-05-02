# include "pantasks.h"

int controller      PROTO((int, char **));
int task            PROTO((int, char **));
int task_host       PROTO((int, char **));
int task_nice       PROTO((int, char **));
int task_nmax       PROTO((int, char **));
int task_npending   PROTO((int, char **));
int task_active     PROTO((int, char **));
int task_trange     PROTO((int, char **));
int task_macros     PROTO((int, char **));
int task_command    PROTO((int, char **));
int task_options    PROTO((int, char **));
int task_periods    PROTO((int, char **));
int task_stdout     PROTO((int, char **));
int task_stderr     PROTO((int, char **));
int flush_jobs      PROTO((int, char **));
int status_server   PROTO((int, char **));
int showtask        PROTO((int, char **));
int kill_job        PROTO((int, char **));
int delete_job      PROTO((int, char **));
int verbose         PROTO((int, char **));
int version         PROTO((int, char **));
int server          PROTO((int, char **));

int server_run	    PROTO((int, char **));
int server_stop	    PROTO((int, char **));
int server_halt	    PROTO((int, char **));

static Command cmds[] = {  
  {1, "active",     task_active,   "set the active state of a task"},
  {1, "command",    task_command,  "define executed command for a task"},
  {1, "controller", controller,    "controller commands"},
  {1, "delete",     delete_job,    "delete job"},
  {1, "host",       task_host,     "define host machine for a task"},
  {1, "nice",       task_nice,     "set nice level for a task"},
  {1, "kill",       kill_job,      "kill job"},
  {1, "nmax",       task_nmax,     "define maximum number of jobs for a task"},
  {1, "options",    task_options, "define optional variables associated with the job task"},
  {1, "npending",   task_npending, "define maximum number of outstanding jobs for a task"},
  {1, "periods",    task_periods,  "define time scales for a task"},
  {1, "flush",      flush_jobs,    "flush all jobs from the queue"},
  {1, "server",     server,        "server-specific commands"},
  {1, "showtask",   showtask,      "list a task"},
  {1, "status",     status_server, "get system status"},
  {1, "stderr",     task_stderr,   "define a file for the job stderr dump"},
  {1, "stdout",     task_stdout,   "define a file for the job stdout dump"},
  {1, "task",       task,          "define a schedulable task"},
  {1, "task.exec",  task_macros,   "define pre-exec macro for a task"},
  {1, "task.exit",  task_macros,   "define exit macros for a task"},
  {1, "trange",     task_trange,   "define valid/invalid time periods for a task"},
  {1, "verbose",    verbose,       "set/toggle verbose mode"},
  {1, "version",    version,       "show version information"},

  {1, "run",        server_run,    "run scheduler"},
  {1, "stop",       server_stop,   "stop scheduler"},
  {1, "halt",       server_halt,   "halt scheduler"},
}; 

void InitPantasksServer () {
  
  int i;

  InitTasks ();
  InitJobs ();
  InitJobIDs ();

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
}

void FreePantasksServer () {
  FreeTasks ();
  FreeJobs ();
  FreeJobIDs ();
}
