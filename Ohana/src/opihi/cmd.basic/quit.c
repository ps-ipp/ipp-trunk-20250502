# include "basic.h"

int quit (int argc, char **argv) {

  int state;

  cleanup ();

  state = 0;
  if (argc > 1) {
    state = atof (argv[1]);
  } 

// the libedit version of readline does not support an incremental write to history file
# ifndef RL_READLINE_VERSION
  history = get_variable("HISTORY");
  if (history != NULL) write_history (history);
# endif

  exit (state);

}
