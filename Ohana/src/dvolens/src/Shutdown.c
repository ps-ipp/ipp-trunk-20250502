# include "dvolens.h"

int Shutdown (char *format, ...) {  
  va_list argp;
  char *formatplus;
  
  ALLOCATE (formatplus, char, strlen(format) + 2);
  strcpy (formatplus, format);
  strcat (formatplus, "\n");

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  free (formatplus);
  va_end (argp);

  fprintf (stderr, "ERROR: dvolens halted\n");
  exit (1);
}

