# include "gcompare.h"
# define D_NVALUES 1000
# define D_NBYTES  10000

data_type input (filename, X, Y, Nskip)
char *filename;
int   X;
int   Y;
int   Nskip;
{
  
  data_type data;
  int i, status, NVALUES, NBYTES, nbytes;
  FILE *f;
  char dummy_line[1000], *next;
  
  if (!strcmp (filename, "-")) {
    f = stdin;
  }
  else {
    f = fopen (filename, "r");
    if (f == NULL) {
      fprintf (stderr, "error opening file %s\n", filename);
      exit (0);
    }
  }

  for (i = 0; i < Nskip; i++) 
    scan_line (f, dummy_line);
 
  /* read in entire file */
  NBYTES = D_NBYTES + 1;
  ALLOCATE (data.buffer, char, NBYTES);
  for (i = 0, nbytes = D_NBYTES; nbytes == D_NBYTES; i++) {
    nbytes = fread (&data.buffer[i*D_NBYTES], 1, D_NBYTES, f);
    NBYTES += D_NBYTES;
    REALLOCATE (data.buffer, char, NBYTES);
  }
  NBYTES -= 2*D_NBYTES - nbytes;
  data.buffer[NBYTES] = 0;
  fprintf (stderr, "got %d bytes\n", NBYTES);

  NVALUES = D_NVALUES;
  ALLOCATE (data.values, value_type, NVALUES);
  next = data.buffer;
  for (i = 0; next != (char *) NULL; ) {
    data.values[i].line  = next;
    data.values[i].match = FALSE;
    status  = dparse (&data.values[i].X, X, data.values[i].line);
    status &= dparse (&data.values[i].Y, Y, data.values[i].line);
    next = nextline (data.values[i].line);
    if (status && (data.values[i].line[0] != '#')) {
      i++;
    }
    if (i == NVALUES - 3) {
      NVALUES += D_NVALUES;
      REALLOCATE (data.values, value_type, NVALUES);
    }
  }
  data.Nvalues = i;
  return (data);
}

