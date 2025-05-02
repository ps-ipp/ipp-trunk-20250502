# include "opihi.h"

/******************/
int client_shell (int argc, char **argv) {

  int Nbad;
  char *line, *prompt, *history;
  pid_t ppid;

  general_init (&argc, argv);
  program_init (&argc, argv);
  startup (&argc, argv);
  prompt = get_variable("PROMPT");
  history = get_variable("HISTORY");
  welcome ();

  /* attempt to connect to the pantasks server (exit on failure) */
  multicommand_InitServer ();

  Nbad = 0;
  while (1) {  /** must exit with command "exit" or "quit" */
    if (Nbad == 10) exit (3);

    line = opihi_readline (prompt);

    if (line == NULL) { 
      
      ppid = getppid();
      if (ppid == 1) {
	gprint (GP_ERR, "caught parent shutdown\n");
	exit (2);
      }
      if (!isatty (STDIN_FILENO)) exit (2);
      gprint (GP_ERR, "Use \"quit\" to exit\n");
      Nbad ++;
      continue;
    }
    Nbad = 0;

    stripwhite (line);

    if (*line) {
      // status = multicommand (line); do something different if false?
	multicommand (line);
	add_history (line);
	append_history (1, history);
    }
    free (line);
  }
}

/* 
   startup sequence:

   - general_init
   - program_init
   - startup (exit if non-interactive)
   - welcome

*/

/* client issues and questions:

- need to identify the commands to be caught by the client
- pass each input line through the command parser first, then
  send to server if not identified as a valid command
- this raises the question of variables: do we parse variables
  at the client level?  I would not think so.  what about input?
  if the input is to be parsed by the client, but executed on the server, 
  I'll need to re-work the input parsing system.  would be better 
  for the server to parse the input command.  in this case, the client 
  user needs to have access to the directories with input scripts.
  this may be an advantage: it would allow some limitation on who can 
  install server scripts.
- variables, vectors and buffers: where are they valid (client or server?)
  it seems they should all be defined locally on the server.  this makes
  graphics plotting a server-side action as well.  this is probably easier,
  but we do need to be careful about multiple clients trying to make plots
  on the same window at the same time.  Exporting the window can be done 
  for one client with the $KII variable, but could be tricky for more 
  complex interactions
- list of commands which clearly must be parsed by the client:
  - quit / exit
  - exec / !
  - ?

- i may need an alternative version of command, modified to avoid parsing the 
  variables and vectors.  I also need an alternative version of the Init commands
  to just list the blocked ones.

- the client now executes all of the basic opihi commands, and only passes
  through the pantasks commands.  The pantasks server interface should only
  accept functions which immediately return, sending back the result to be
  printed by the client.  

  The server should not accept the 'task' commands from the client command-line.
  To define and run macros or tasks on the server, these need to be added to 
  a script which is loaded with a varient on the input command.  
*/
