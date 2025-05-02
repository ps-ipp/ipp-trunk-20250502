# include "imregister.h"

/* see note at end of file */
double get_fwhm (char *filename) {

  FILE *f;
  char line[1024];
  double *fwhm, *mag, *Mag, *FWHM, *Nfwhm, flags;
  double F, value, Mlim, Vmode, Fmin, Fmax;
  int i, N, n, Nwant, bin, mode, Nmode;

  /* get sextract file (has fwhm info) */
  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't open file.sdat\n");
    return (-1.0);
  }

  n = 0;
  N = 1000;
  ALLOCATE (fwhm, double, N);
  ALLOCATE (mag, double, N);
  
  /* load in data from file */
  while (scan_line (f, line) != EOF) {
    dparse (&flags, 2, line);
    if (flags > 0) continue;
    dparse (&mag[n], 4, line);
    if (mag[n] > 0) continue;
    dparse (&fwhm[n], 1, line);
    n++;
    if (n == N) {
      N += 1000;
      REALLOCATE (fwhm, double, N);
      REALLOCATE (mag, double, N);
    }
  }    
  if (n == 0) { return (-1.0); }

  /* find the mag threshold */
  ALLOCATE (Mag, double, n);
  memcpy (Mag, mag, n*sizeof(double));
  dsort (Mag, n);
  Nwant = MIN (MAX (0.25*n, 10), n-1);
  Mlim = Mag[Nwant];

  /* create subset of fwhm with mag < Mlim */
  ALLOCATE (FWHM, double, n);
  ALLOCATE (Nfwhm, double, 1024);
  bzero (FWHM, n*sizeof(double));
  bzero (Nfwhm, 1024*sizeof(double));
  
  N = 0;
  for (i = 0; i < n; i++) {
    if (mag[i] > Mlim) continue;
    FWHM[N] = fwhm[i];
    N++;

    /* accumulate mag histogram */
    bin = fwhm[i]*10.0;
    if (bin < 0) continue;
    if (bin > 999) continue;
    Nfwhm[bin] ++;
  }

  /* find the mode (bin size = 0.1 pixels) */
  mode = 0; 
  Nmode = Nfwhm[mode];
  for (i = 0; i < 1000; i++) {
    if (Nfwhm[i] > Nmode) {
      mode = i;
      Nmode = Nfwhm[i];
    }
  }
  Vmode = mode * 0.1;
  Fmin = Vmode - 0.2;
  Fmax = Vmode + 0.2;

  /* average bins within 0.2 of mode */
  F = 0;
  n = 0;
  for (i = 0; i < N; i++) {
    if (FWHM[i] < Fmin) continue;
    if (FWHM[i] > Fmax) continue;
    n++;
    F+=FWHM[i];
  }
  if (n > 3) {
    value = F / n;
    return (value);
  }

  /* if we don't get the answer from the above, resort to median */
  dsort (FWHM, N);
  F = 0;
  n = 0;
  for (i = 0.25*N; i < 0.75*N; i++) {
    n++;
    F+=FWHM[i];
  }
  if (n > 0) {
    value = F / n;
  } else {
    value = -1;
  }
  return (value);

}

/* 

   calculate the fwhm from the data in the given file.
   The file contains output from sextractor (or other program, perhaps).
   The columns that matter are: 
     1) fwhm (pixels)
     2) flags
     4) mag

   this function determines the magnitude range for the file,
   selects stars in the upper 25% of that magnitude range (by number),
   rejecting bad stars (flags > 0) along the way
   
   the resulting list of fwhms are examined for the mode, in 0.1 pixel bins.
   stars with fwhm within 0.2 pix of that mode are averaged to find the
   best fwhm value.

   in the database, a value of 0 means the measurement has not been tried.
   a value of -1 means the measurement failed

*/
