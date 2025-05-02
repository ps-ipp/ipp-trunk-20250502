# include "elixir.h"

FILE *LogOpen (char *filename) {

  FILE *f;
  char *path;

  f = fopen (filename, "a");

  /* probably don't have needed directory */
  if ((f == (FILE *) NULL) && (errno == ENOENT)) {
    path = pathname (filename);
    if (!mkdirhier (path, S_IRWXU | S_IRWXG | S_IRWXO)) {
      f = fopen (filename, "a");
    }
  }
  if (f == (FILE *) NULL) {
    fprintf (stderr, "LogOpen could not open the log file %s, errno: %d\n", filename, errno);
    f = stderr;
  }

  return (f);

}
