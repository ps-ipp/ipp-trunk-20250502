# include "gastro.h"
# define NBYTE_LINE 53
# define NLINES 100

int get_region_coords (double *ra, double *dec, int rnumber, char *side) {
  
  FILE *f;
  int i, j, done, found, Nbytes, Nline, num, NBYTES;
  char *buffer;
  double R, D;
  
  f = fopen (LONEOS_REGION_FILE, "r");
  if (f == NULL) {
    fprintf (stderr, "couldn't find region map %s\n", LONEOS_REGION_FILE);
    return (FALSE);
  }
 
  NBYTES = NBYTE_LINE * NLINES;
  ALLOCATE (buffer, char, NBYTES);
 
  found = done = FALSE;
  for (i = 0; !done && !found; i++) {
    Nbytes = fread (buffer, sizeof(char), NBYTES, f);
    if (Nbytes < 1) done = TRUE;
    Nline = Nbytes / NBYTE_LINE;
    for (j = 0; !found && (j < Nline); j++) {
      num = atof (&buffer[j*NBYTE_LINE]);
      if (num == rnumber) {
	found = TRUE;
	sscanf (&buffer[j*NBYTE_LINE], "%*d %lf %lf", &R, &D);
	fwrite (&buffer[j*NBYTE_LINE], 1, 106, stderr);
	fprintf (stderr, "\n\n%f %f\n", R, D);
	if (!strncasecmp (side, "east", 4)) {
	  R += 0.026 / cos (D);  
	  /* if the word says "east", we need to offset by 1 chip width,
	     R and D are in radians here, so 0.026 is 1.5 deg in radians */
	}
	R *= (180.0 / M_PI);
	D *= (180.0 / M_PI);
      }
    }
  }

  free (buffer);
  fclose (f);

  if (!found) {
    fprintf (stderr, "error: can't find desired region number %d\n", rnumber);
    *ra = *dec = 0;
    return (FALSE);
  }

  *ra = R;
  *dec = D;
  return (TRUE);

}
