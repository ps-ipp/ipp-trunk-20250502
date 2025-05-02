# include "delstar.h"

static FITS_DB *db;

void set_db (FITS_DB *in) {
  db = in;
}

/* clean up open / locked ImageCat before shutting down */
int Shutdown (char *format, ...) {  
  va_list argp;
  char *formatplus;
  
  ALLOCATE (formatplus, char, strlen(format));
  strcpy (formatplus, format);
  strcat (formatplus, "\n");

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  free (formatplus);
  va_end (argp);

  fprintf (stderr, "ERROR: delstar halted\n");
  exit (1);
}

