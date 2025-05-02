# include "basic.h"

// XXX add a warning if ohana_memory is not compiled in 
int memory (int argc, char **argv) {
  
  if (argc < 2) goto usage;

  if (!strcasecmp (argv[1], "max-lines")) {
    if (argc != 3) goto usage;
    int MaxLines = atoi(argv[2]);
    ohana_memdump_set_maxlines (MaxLines);
    return TRUE;
  }

  if (!strcasecmp (argv[1], "all")) {
    ohana_memdump (1);
    return (TRUE);
  }
  if (!strcasecmp (argv[1], "leaks")) {
    ohana_memdump (0);
    return (TRUE);
  }
  if (!strcasecmp (argv[1], "check")) {
    ohana_memcheck (0);
    return (TRUE);
  }
  if (!strcasecmp (argv[1], "checkfree")) {
    ohana_memcheck (1);
    return (TRUE);
  }
  if (!strcasecmp (argv[1], "stats")) {
    OhanaMemstats memstats = ohana_memstats (0);
    set_variable ("memory:Ntotal", memstats.Ntotal);
    set_variable ("memory:Nbytes", memstats.Nbytes);
    set_variable ("memory:Ngood",  memstats.Ngood);
    set_variable ("memory:Nbad",   memstats.Nbad);
    return (TRUE);
  }
  if (!strncasecmp ("variables", argv[1], strlen(argv[1]))) {
    ListVariables ();
    return (TRUE);
  }
  if (!strncasecmp ("vectors", argv[1], strlen(argv[1]))) {
    ListVectors ();
    return (TRUE);
  }
  if (!strncasecmp ("buffers", argv[1], strlen(argv[1]))) {
    PrintBuffers (0);
    return (TRUE);
  }
  if (!strncasecmp ("macros", argv[1], strlen(argv[1]))) {
    ListMacros();
    return (TRUE);
  }
  if (!strncasecmp ("commands", argv[1], strlen(argv[1]))) {
    print_commands (stderr);
    return (TRUE);
  }

  if (!strncasecmp ("strings", argv[1], strlen(argv[1]))) {
    ohana_memdump_strings_file (stderr, FALSE);
    return (TRUE);
  }

usage:
  gprint (GP_ERR, "USAGE: memory (mode) [-max-lines]\n");
  gprint (GP_ERR, "USAGE: memory (max-lines) (N)\n");
  gprint (GP_ERR, " mode options: all, leaks, check, checkfree, variables, vectors, buffers, macros, commands, strings\n");
  return (FALSE);
}
