# include "pantasks.h"

int invalid           PROTO((int, char **));
int server_shutdown   PROTO((int, char **));
int server_disconnect PROTO((int, char **));
int server_connect    PROTO((int, char **));

/* we also list here commands which are not valid for the client, but represent valid
   pantasks commands.  these are caught by the client and sent to the 'invalid'
   function */

static Command cmds[] = {  
  {1, "task",       invalid,  	 "define a schedulable task"},
  {1, "task.exit",  invalid,  	 "define exit macros for a task"},
  {1, "task.exec",  invalid,  	 "define pre-exec macro for a task"},

  {1, "shutdown",   server_shutdown,   "send exit signal to server"},
  {1, "disconnect", server_disconnect, "close connection to server"},
  {1, "connect",    server_connect,    "open connection to server"},

}; 

void InitPantasksClient () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }

}

void FreePantasksClient () {
}
