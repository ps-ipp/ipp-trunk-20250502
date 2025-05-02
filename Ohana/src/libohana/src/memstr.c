# include "ohana.h"

/* memstr returns a view, not an allocated string : don't free */
/* returns pointer to start of m2 in m1, or NULL if failure */ 
char *memstr (char *m1, char *m2, int n) {

  int i, N;

  N = strlen (m2);
  for (i = 0; (i < n - N + 1) && memcmp (m1, m2, N); i++, m1++);
  if (memcmp (m1, m2, N)) return (NULL);
  return (m1);

}

/* formatted write statement, with intelligent allocation */
int write_fmt (int fd, char *format, ...) {

  int Nbyte, status;
  char tmp, *line;
  va_list argp;  

  va_start (argp, format);
  Nbyte = vsnprintf (&tmp, 0, format, argp);
  va_end (argp);

  va_start (argp, format);
  ALLOCATE (line, char, Nbyte + 1);
  vsnprintf (line, Nbyte + 1, format, argp);
  status = write (fd, line, strlen(line));
  va_end (argp);

  free (line);
  return (status);
}
