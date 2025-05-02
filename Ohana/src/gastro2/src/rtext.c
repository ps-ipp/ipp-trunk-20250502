# include "gastro2.h"
/* by necesity hard wired */
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000

StarData *rtext (FILE *f, int *nstars) {

  char *buffer;
  int i, N, nbytes, Ninstar, Nstars, NSTARS;
  double dmag, type;
  StarData *stars;

  NSTARS = *nstars;
  ALLOCATE (stars, StarData, MAX (NSTARS, 1));

  N = Nstars = 0;
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR));
  
  while ((nbytes = fread (buffer, 1, (BLOCK*BYTES_STAR), f)) != 0) {
    Ninstar = nbytes / BYTES_STAR;
    for (i = 0; i < Ninstar; i++, Nstars++) {
      dparse (&stars[N].X, 1, &buffer[i*BYTES_STAR]);
      dparse (&stars[N].Y, 2, &buffer[i*BYTES_STAR]);
      dparse (&stars[N].M, 3, &buffer[i*BYTES_STAR]);
      dparse (&dmag,       4, &buffer[i*BYTES_STAR]);
      dparse (&type,       5, &buffer[i*BYTES_STAR]);

      /* hardwired dophot exclusions should eventually be encapsulated elsewhere */
      if ((type == 4) || (type == 6) || (type == 5) || (type == 9)) continue;
      if ((MAX_ERROR > 0) && (dmag > 1000*MAX_ERROR)) continue;
      stars[N].dM   = 0.001 * dmag;
      stars[N].type = type;
      N++;
    }
  }
  free (buffer);
 
  if (Nstars != NSTARS) {
    fprintf (stderr, "WARNING: only read %d of %d stars\n", Nstars, NSTARS);
  }
  if (N < 5) { 
    fprintf (stderr, "ERROR: too few stars for reliable solution, only %d\n", N);
    exit (1);
  }
  *nstars = N;
  return (stars);
}
