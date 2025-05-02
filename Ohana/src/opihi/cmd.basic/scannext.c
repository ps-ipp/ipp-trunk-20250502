# include "basic.h"

int scannext (int argc, char **argv) {

  int N, status;
  char *line;
  static char *filename = NULL;
  static FILE *f = NULL;

  // clear all sections
  if ((N = get_argument (argc, argv, "-close"))) {
    remove_argument (N, &argc, argv);
    if (f) {
      fclose (f);
      f = NULL;
    } else {
      gprint (GP_ERR, "file is not currently open\n");
    }
    if (filename) { 
      free (filename);
      filename = NULL;
    }
    return (TRUE);
  }

  if ((argc != 2) && (argc != 3)) {
    gprint (GP_ERR, "USAGE: scannext <filename> [var]\n");
    return (FALSE);
  }

  if (!f || !filename || strcmp(filename, argv[1])) {
    if (f) fclose (f);
    f = fopen (argv[1], "r");
    if (f == NULL) {
      gprint (GP_ERR, "file %s not found\n", argv[1]);
      return (FALSE);
    }
    if (filename) free (filename);
    filename = strcreate (argv[1]);
  }
  
  ALLOCATE (line, char, 2048);
  status = scan_line_maxlen (f, line, 2048);
  if (status == EOF) {
    set_str_variable (argv[2], "EOF");
  } else {
    set_str_variable (argv[2], line);
  }
  free (line);

  return (TRUE);
}
