# include "basic.h"

int scan (int argc, char **argv) {

  int i, N, status;
  char *line;
  FILE *f;

  if ((argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: scan <filename> <var> [N]\n");
    return (FALSE);
  }

  f = stdin;
  if (strcmp (argv[1], "stdin")) {
    f = fopen (argv[1], "r");
    if (f == (FILE *) NULL) {
      gprint (GP_ERR, "file %s not found\n", argv[1]);
      return (FALSE);
    }
  }
  
  ALLOCATE (line, char, 1024);
  N = 1;
  if (argc == 4) {
    N = atof(argv[3]);
    if (N < 1) {
      gprint (GP_ERR, "scan: line numbers must start at 1\n");
      return (FALSE);
    }
  }

  for (i = 0; (i < N) && ((status = scan_line (f, line)) != EOF); i++);
  if (i < N) {
    set_str_variable (argv[2], "EOF");
  } else {
    set_str_variable (argv[2], line);
  }
  free (line);

  if (f != stdin) {
    fclose (f);
  }
  return (TRUE);
 

}
