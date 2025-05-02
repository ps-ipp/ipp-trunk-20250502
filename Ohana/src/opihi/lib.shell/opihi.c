# include "opihi.h"

/******************/
int opihi (int argc, char **argv) {

  int Nbad;
  char *line, *prompt, *history;
  pid_t ppid;

  general_init (&argc, argv); // in startup.c : init drand48, signals, gprint, opihi_error
  program_init (&argc, argv); // in mana.c.in, dvo.c.in, etc
  startup (&argc, argv);      // in startup.c : set global variables, load history, --norc, --only
  prompt = get_variable("PROMPT");
  history = get_variable("HISTORY");
  welcome ();

  Nbad = 0;
  while (1) {  /** must exit with command "exit" or "quit" */
    if (Nbad == 10) exit (20);

    line = opihi_readline (prompt);

    if (line == NULL) { 
      
      ppid = getppid();
      if (ppid == 1) {
	signal (SIGPIPE, SIG_IGN);
	gprint (GP_ERR, "caught parent shutdown\n");
	exit (21);
      }
      if (!isatty (STDIN_FILENO)) exit (21);
      gprint (GP_LOG, "Use \"quit\" to exit\n");
      Nbad ++;
      continue;
    }

    Nbad = 0;

    stripwhite (line);
    if (*line) {
      multicommand (line);
      add_history (line);

// the libedit version of readline does not support an incremental write to history file
# ifdef RL_READLINE_VERSION
      if (history != NULL) append_history (1, history);
# endif

    }
    free (line);
    line = (char *) NULL;
  }
}

/* 
   startup sequence:

   - general_init
   - program_init
   - startup (exit if non-interactive)
   - welcome

*/
