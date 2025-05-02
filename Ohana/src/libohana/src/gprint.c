# include "ohana.h"

// print to stdout or stderr depending on dest.
// opihi overrides this with the real gprint

int gprint (gpDest dest, char *format, ...) {

  int status;
  va_list argp;  

  FILE *file = (dest == GP_LOG) ?  stdout : stderr;

  va_start (argp, format);
  status = vfprintf (file, format, argp);
  va_end (argp);
  return (status);
}
