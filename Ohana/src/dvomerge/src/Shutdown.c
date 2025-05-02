# include "dvomerge.h"

/* clean up open / locked ImageCat before shutting down */
int Shutdown (char *format, ...) {  
  va_list argp;
  char formatplus[1024];
  
  snprintf (formatplus, 1024, "%s\n", format);

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  va_end (argp);

  fprintf (stderr, "ERROR: dvomerge halted\n");
  exit (1);
}

