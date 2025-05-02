# include "basic.h"

int help (int argc, char **argv) {

  int Nbytes;
  FILE *f;
  char *helpdir, *file, buff[512];

  helpdir = get_variable ("HELPDIR");
  if (helpdir == (char *) NULL) {
    gprint (GP_ERR, "variable HELPDIR not found\n");
    return (FALSE);
  }

  if (argc == 1) {
    sprintf (buff, "ls %s", helpdir);
    if (system (buff) == -1) {
      fprintf (stderr, "help directory unavailable\n");
      return (FALSE);
    }
    return (TRUE);
  }

  Nbytes = strlen(helpdir) + strlen(argv[1]) + 2;
  ALLOCATE (file, char, Nbytes);
  snprintf (file, Nbytes, "%s/%s", helpdir, argv[1]);

  f = fopen (file, "r");
  free (file);

  if (f == NULL) {
    gprint (GP_ERR, "No help for: %s\n", argv[1]);
    return (FALSE);
  }

  while (scan_line (f, buff) != EOF)
    gprint (GP_LOG, "%s\n", buff);

  fclose (f);
  return (TRUE);
}

