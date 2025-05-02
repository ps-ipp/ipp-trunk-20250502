# include "opihi.h"
# define DEBUG 0

static int VERBOSE_SHELL = OPIHI_VERBOSE_OFF;

int command (char *line, char **outline, int VERBOSE) {

  int i, status, argc;
  char **argv, **targv, *rawline;
  Command *cmd;

  // the input line is never NULL
  if (!line) { fprintf (stderr, "programming error\n"); abort(); }

  rawline = strcreate (line);  // used for error messages which should echo the unparsed line

  /* force a space between ! and first word: !ls becomes ! ls */
  if (line[0] == '!') {
    REALLOCATE (line, char, strlen(line) + 5);
    memmove (&line[2], &line[1], strlen(&line[1]) + 1);
    line[1] = ' ';
  }

  /* expand anything of the form $fred or $fred$sam, etc */ 
  line = expand_vars (line);     /* line is freed here, new one allocated */
  if (!line) goto escape;

  /* expand anything of the form fred[N] */ 
  line = expand_vectors (line);  /* line is freed here, new one allocated */
  if (!line) goto escape;

  // print the line with the variables and vectors expanded, but before evaluating 
  // any in-line math expression
  if (VERBOSE_SHELL == OPIHI_VERBOSE_ON) gprint (GP_ERR, "opihi: %s\n", line);

  /* solve math expresions, assign variable, if needed.  any entry in line of the form {foo}
   * returns value or tmp vector / buffer */
  line = parse (&status, line);        /* line is freed here, new one allocated */
  if (!status) goto escape;

  /* we may have reallocated line, return new pointer */
  *outline = line;
  
  argv = parse_commands (line, &argc);
  if (argc == 0) {
      FREE (rawline);
      set_int_variable ("STATUS", TRUE);
      return (TRUE);  /* empty command or assignment */
  }

  /* save the original values of argv since command may modify the array */
  ALLOCATE (targv, char *, argc);
  for (i = 0; i < argc; i++) targv[i] = argv[i];

  cmd = MatchCommand (argv[0], VERBOSE, FALSE);
  if (cmd == NULL) {
    status = -1;
  } else {
    free (argv[0]);
    argv[0] = strcreate (cmd[0].name);
    targv[0] = argv[0];
    status = (*cmd[0].func) (argc, argv);
  }
  for (i = 0; i < argc; i++) free (targv[i]);
  free (targv);
  free (argv);

  if (!status) {
    char *msg;
    msg = get_variable_ptr ("ERRORMSG");
    if (msg != (char *) NULL) gprint (GP_ERR, "%s\n", msg);
    if (VERBOSE_SHELL != OPIHI_VERBOSE_OFF) gprint (GP_ERR, "error on line: %s\n", rawline);
  }

  set_int_variable ("STATUS", status);

  # if (DEBUG) 
  gprint (GP_ERR, "command: %s, status: %d\n", line, status);
  # endif

  FREE (rawline);
  return (status);

 escape:
  set_int_variable ("STATUS", FALSE);

  # if (DEBUG) 
  gprint (GP_ERR, "command: %s, status: %d\n", line, FALSE);
  # endif

  FREE (rawline);

  if (VERBOSE_SHELL != OPIHI_VERBOSE_OFF) gprint (GP_ERR, "error on line: %s\n", rawline);

  // return the current value of line, in case it was modified
  *outline = line;
  return FALSE;
}

void set_verbose_shell(int mode) {
    VERBOSE_SHELL = mode;
}

int get_verbose_shell(void) {
    return VERBOSE_SHELL;
}

    

/* parse the input line, search for the corresponding command, and execute it
   if no match is found, return -1; this is used above to distinguish between
   a command error and an unknown command.  if VERBOSE is true, unknown commands
   result in an error message.  The input line is freed and the resulting parsed
   line is returned on 'outline'
*/
