# include "imregister.h"
# define NBYTES 1024
# define WINDOW 1800
/* probes are updated every 600 seconds.  give us some leeway here */

int load_probes (char *filename, unsigned long tzero, int *wantprobe, double *values, int Nprobe) {

  FILE *f;
  char line[256], closeline[256], *buffer, *c, *p;
  int nprobe, probe[10];
  int i, Nstart, Nread, Nbytes, Nshift, Nleft, done, close_enough;
  double time, jdstart, tmp;
  int Nsec, sec;

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open DataLogger file: %s\n", filename);
    return (FALSE);
  }

  /* assume fixed format */
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  sscanf (line, "%*s %*s %*s %*s %lf", &jdstart);
  Nsec = (jdstart - 2440587.5)*86400;
  if (tzero < Nsec - WINDOW) {
    fprintf (stderr, "missing time in DataLogger file\n");
    return (FALSE);
  }

  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  sscanf (line, "%*s %*s %d", &nprobe);
  for (i = 0; (i < nprobe) && (i < 10); i++) {
    dparse (&tmp, i+4, line);
    probe[i] = tmp;
  }

  /* check that probes match desired probes -- demand same order in file */
  if (nprobe != Nprobe) {
    fprintf (stderr, "wrong number of probes\n");
    return (FALSE);
  }
  for (i = 0; (i < nprobe) && (i < 10); i++) {
    if (probe[i] != wantprobe[i]) {
      fprintf (stderr, "probe mismatch\n");
      return (FALSE);
    }
  }
  
  ALLOCATE (buffer, char, NBYTES+1);
    
  Nstart = 0;
  done = FALSE;
  close_enough = FALSE;
  while (!done && ((Nread = fread (&buffer[Nstart], 1, NBYTES - Nstart, f)) > 0)) {
    Nbytes = Nread + Nstart;
    buffer[Nbytes] = 0;
    
    /* we are using strchr lib functions - buffer can't have NULLs */
    c = strrchr (buffer, '\n');
    if (c == (char *) NULL) c = &buffer[Nbytes-1];
    Nshift = c - buffer + 1;
    
    /* limit searches to the range buffer[0] - buffer[Nshift] */
    p = &buffer[0];
    while (1) {
      c = strchr (p, '\n');
      if (c == (char *) NULL) break;
      if (c > &buffer[Nshift]) break;
      *c = 0;
      dparse (&time, 0, p);
      sec = Nsec + time*86400;
      if (sec + WINDOW > tzero) {
	strcpy (closeline, p);
	close_enough = TRUE;
      }
      if (sec > tzero) {
	for (i = 0; i < Nprobe; i++) {
	  dparse (&values[i], i+5, p);
	}
	free (buffer);
	return (TRUE);
      }
      *c = '\n';
      p = c + 1;
    }
    
    Nleft  = Nbytes - Nshift;
    if (Nleft > 0) memmove (buffer, &buffer[Nshift], Nleft);
    
    Nstart = Nleft;
  }

  if (close_enough) {
    for (i = 0; i < Nprobe; i++) {
      dparse (&values[i], i+5, closeline);
    }
    free (buffer);
    return (TRUE);
  }

  fprintf (stderr, "time not in DataLogger file %ld\n", tzero);
  free (buffer);
  return (FALSE);
}
