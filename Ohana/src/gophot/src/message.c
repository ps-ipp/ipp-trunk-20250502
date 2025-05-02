# include <stdio.h>
# include <stdarg.h>

static FILE *f;
static int verbosity = 0;

int mprint (int level, char *mode, ...) {

  int status;
  va_list argp;
  
  if (level > verbosity) return (0);

  va_start (argp, mode);
  
  status = vfprintf (f, mode, argp);

  va_end (argp);

  return (status);

}

int set_verbosity (int level) {

  f = stderr;
  verbosity = level;
  return 1;
}
