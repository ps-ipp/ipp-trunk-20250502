# include "imregister.h"

int WriteFIFO (char *filename, char *line) {

  int state, mode;
  FILE *f;

  f = fsetlockfile (filename, 20.0, LCK_XCLD, &state);
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't lock fifo %s\n", filename);
    return (FALSE);
  }

  fseeko (f, 0LL, SEEK_END);
  fprintf (f, "%s\n", line);

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (filename, mode);
  fclearlockfile (filename, f, LCK_XCLD, &state);

  return (TRUE);
}

