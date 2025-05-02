# include "relphot.h"

static FITS_DB *db = NULL;

void set_db (FITS_DB *in) {
  db = in;
}

/* clean up open / locked ImageCat before shutting down */
int Shutdown (char *format, ...) {  
  va_list argp;
  char formatplus[1024];
  
  // we cannot allocate this with the ohana allocation tools
  // because it could deadlock
  snprintf (formatplus, 1024, "%s\n", format);

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  va_end (argp);

  fprintf (stderr, "ERROR: relphot halted\n");
  exit (1);
}

