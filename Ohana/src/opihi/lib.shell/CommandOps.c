# include "opihi.h"

// the command table is only modified by macro_create: if thread protection is needed, it
// should be added there.  it may not be needed if programs only call macro_create in a
// single thread.  pantasks_server only calls macro_create in the 'input' thread, so it is
// safe.

// if the user has installed the libedit version of readline, we need to modify a couple symbols:
# ifndef RL_READLINE_VERSION
# define rl_completion_matches(A,B) completion_matches(A,B)
# endif

static Command  *commands;
static int      Ncommands;
static int      NCOMMANDS;

void InitCommands () {
  NCOMMANDS = 20;
  Ncommands = 0;
  ALLOCATE (commands, Command, NCOMMANDS);
}

void FreeCommands () {
  int i;

  for (i = 0; i < Ncommands; i++) {
    if (commands[i].real) continue;
    free (commands[i].name);
    free (commands[i].help);
  }
  free (commands);
}

void AddCommand (Command *new) {
  
  commands[Ncommands] = *new;
  Ncommands ++;
  if (Ncommands == NCOMMANDS) {
    NCOMMANDS += 20;
    REALLOCATE (commands, Command, NCOMMANDS);
  }
}

int DeleteCommand (Command *command) {

  int i, Nc;

  Nc = -1;
  for (i = 0; i < Ncommands; i++) {
    if (command == &commands[i]) {
      Nc = i;
      break;
    }
  }
  if (Nc == -1) {
    gprint (GP_ERR, "programming error: command not found\n");
    return (FALSE);
  }

  free (commands[Nc].name);
  for (i = Nc + 1; i < Ncommands; i++)
    commands[i - 1] = commands[i];
  Ncommands --;
  REALLOCATE (commands, Command, Ncommands);
  return (TRUE);
}

/* return command which unambiguously matches name */
Command *MatchCommand (char *name, int VERBOSE, int EXACT) {

  int i, match[10], Nmatch;

  /* try for an exact match first */
  for (i = 0; i < Ncommands; i++) {
    if (!strcmp (commands[i].name, name)) {
      return (&commands[i]);
    }
  }
  if (EXACT) {
    if (VERBOSE) gprint (GP_ERR, "no exact match to %s\n", name);
    return (NULL);
  }

  /* not found as complete command, try partial */
  Nmatch = 0;
  for (i = 0; (Nmatch < 10) && (i < Ncommands); i++) {
    if (!strncmp (commands[i].name, name, strlen(name))) {  /* found a command */
      match[Nmatch] = i;
      Nmatch ++;
    }
  }
  if (Nmatch == 1) return (&commands[match[0]]);

  if (Nmatch > 1) {
    if (VERBOSE) {
      gprint (GP_ERR, "ambiguous command: %s ( ", name);
      for (i = 0; i < Nmatch; i++) {
	gprint (GP_ERR, "%s ", commands[match[i]].name);
      }
      gprint (GP_ERR, ")\n");
    }
    return (NULL);
  }
  if (VERBOSE) gprint (GP_ERR, "%s: Command not found.\n", name);
  return (NULL);
}  

/* we need a strcreate function that does NOT use the ohana memory management
 * system to interact with some external libraries (which themselves free the memory)
 */
char *strcreate_sans_ohana (char *string) {

  char *line;

  if (string == (char *) NULL) return ((char *) NULL);
  
  line = malloc (MAX (1, strlen(string)) + 1);
  line = strcpy (line, string);

  return (line);
}

/* generate a command completion list for readline */
/**** these probably do not interact well with OHANA memory!!! ****/
char *command_generator (const char *text, int state) {

  /* i must be remembered from call to call */
  static int i, len;

  /* On first call, state is set to 0: initial the state */
  if (!state) {
    i = -1;  
    len = strlen (text);
  }

  /* Return the next partial match from the command list */
  for (i++; i < Ncommands; i++) {
    if (!len) 


      return (strcreate_sans_ohana(commands[i].name));
    if (!strncmp (commands[i].name, text, len)) {
      return (strcreate_sans_ohana(commands[i].name));
    }
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

/* tell readline to use out command_generator rather than basic completion */
char **command_completer (const char *text, int start, int end) {
  OHANA_UNUSED_PARAM(end);
  
  char **matches;

  matches = (char **) NULL;
  
  if (start == 0) {
    matches = rl_completion_matches (text, command_generator);
  }

  return (matches);
}

void sort_commands (int *seq) {

  int i;

  for (i = 0; i < Ncommands; i++) seq[i] = i;

# define SWAPFUNC(A,B){ int tmp = seq[A]; seq[A] = seq[B]; seq[B] = tmp; }
# define COMPARE(A,B)(strcmp (commands[seq[A]].name, commands[seq[B]].name) < 0)

  OHANA_SORT (Ncommands, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
}

void print_commands (FILE *f) {

  int i, *seq;

  ALLOCATE (seq, int, Ncommands);
  sort_commands (seq);
  for (i = 0; i < Ncommands; i++) {
    fprintf (f, "%-25s -- %s\n", commands[seq[i]].name, commands[seq[i]].help);
  }
  free (seq);

  return;
}
