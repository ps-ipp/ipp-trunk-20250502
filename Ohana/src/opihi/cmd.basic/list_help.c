# include "basic.h"

int list_help (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  FILE *f;
  int fd;
  char filename[128], line[256];

  sprintf (filename, "/tmp/status.XXXXXX");
  if ((fd = mkstemp (filename)) == -1) {
    gprint (GP_ERR, "error opening output\n");
    return (FALSE);
  }
  f = fdopen (fd, "w");
  if (f == (FILE *) NULL) f = stdout;
  print_commands (f);
  if (f != stdout) {
    fclose (f);
    sprintf (line, "more %s", filename);
    if (system (line) == -1) {
      fprintf (stderr, "help list unavailable\n");
    }
  }
  unlink (filename);
  return (TRUE);
}
