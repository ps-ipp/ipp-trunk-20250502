# include "addstar.h"
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000

Stars *ReadStarsTEXT (FILE *f, unsigned int *nstars) {

  int j, N, Nextra, Ninstar, Nskip, Nbytes, nbytes;
  int done, itmp;
  char *buffer, *c, *c2;
  double tmp, fx, fy, df;
  double ZeroPt;
  Stars *stars;
  
  ZeroPt = GetZeroPoint();

  /* load in stars by blocks of 1000 */
  N = 0;
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR) + 1);
  buffer[BLOCK*BYTES_STAR] = 0;
  Nextra = 0;

  ALLOCATE (stars, Stars, *nstars);

  while (N < *nstars) {
    /* load next data block */
    Nbytes = BYTES_STAR * BLOCK - Nextra;
    nbytes = fread (&buffer[Nextra], 1, Nbytes, f);
    if (nbytes == 0) {
      *nstars = N;
      return (stars);
    }
    nbytes += Nextra;

    /* check line-by-line integrity */
    c = buffer;
    done = FALSE;
    while ((c < buffer + nbytes) && (!done)) { 
      for (c2 = c; *c2 == '\n'; c2++);
      if (c2 > c) { /* extra return chars */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	memset (buffer + nbytes, 0, Nskip);
	if (VERBOSE) fprintf (stderr, "deleted %d extra return chars\n", Nskip);
      }
      c2 = strchr (c, '\n');
      if (c2 == (char *) NULL) {
	done = TRUE;	
	continue;
      }
      c2++;
      if ((c2 - c) != BYTES_STAR) { /* bad line, delete it */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	memset (buffer + nbytes, 0, Nskip);
	if (VERBOSE) fprintf (stderr, "deleted line, %d extra chars\n", Nskip);
      } else {
	c = c2;
      }
    }

    /* extract data for stars */
    Ninstar = nbytes / BYTES_STAR;
    Nextra = nbytes % BYTES_STAR;
    for (j = 0; (j < Ninstar) && (N < *nstars); j++, N++) {
      InitStar (&stars[N]);
      fparse (&stars[N].measure.Xccd,  1, &buffer[j*BYTES_STAR]);
      fparse (&stars[N].measure.Yccd,  2, &buffer[j*BYTES_STAR]);
      fparse (&stars[N].measure.M,  3, &buffer[j*BYTES_STAR]);
      if ((stars[N].measure.M > ZeroPt) || isnan(stars[N].measure.M)) {
	stars[N].measure.M = NAN;
      }

      /* cmp files carry dM in millimags */
      dparse (&tmp, 4, &buffer[j*BYTES_STAR]);
      stars[N].measure.dM = 0.001*tmp;

      // the dophot type information get pushed into the upper 2 bytes of photFlags
      dparse (&tmp,         5, &buffer[j*BYTES_STAR]);
      itmp = tmp;
      stars[N].measure.photFlags = (itmp << 16);

      // XXX I've removed the Mgal field from the measure.d table, and am using Map
      // instead.  DVO has not to date been used to track and study objects which are
      // extended, but it is about to.  Related to this, I have created the concept of two
      // extended source attribute tables, to carry the information being measured by the
      // IPP.

      // dparse (&stars[N].Mgal, 7, &buffer[j*BYTES_STAR]);
      fparse (&stars[N].measure.Map,  8, &buffer[j*BYTES_STAR]);
      dparse (&fx,   9, &buffer[j*BYTES_STAR]);
      dparse (&fy,  10, &buffer[j*BYTES_STAR]);
      dparse (&df,  11, &buffer[j*BYTES_STAR]);

      stars[N].measure.FWx   = ToShortPixels (fx);
      stars[N].measure.FWy   = ToShortPixels (fy);
      stars[N].measure.theta = ToShortDegrees (df);
    }
  }
  *nstars = N;
  return (stars);
}
