# include "gastro2.h"

/* mag range -5 - 35, dmag = 0.25, Nbin = 160 */
# define MMIN -5
# define MMAX 35
# define dM 0.5
# define NMBIN 90

int get_luminosity_func (StarData *stars, int N, LumStats *lum) {

  int i, j, Nb, peaki, peakn;
  double mbin[NMBIN];
  double bin[NMBIN], lbin[NMBIN], rbin[NMBIN];
  double C0, C1;

  lum[0].dNdM = lum[0].Mo = 0;

  // use 'explicit_bzero' to avoid optimizations compile warnings
  // explicit_bzero (mbin, NMBIN * sizeof (double));
  memset (mbin, 0, NMBIN * sizeof (double));
  memset (bin, 0, NMBIN * sizeof (double));

  /* sum histogram */
  for (i = 0; i < N; i++) {
    if (stars[i].M < MMIN) continue;
    if (stars[i].M > MMAX) continue;

    j = (stars[i].M - MMIN) / dM;
    j = MIN (MAX (j, 0), (NMBIN - 1));
    mbin[j] ++;
  }

  /* find peak bin */
  peaki = 0;
  peakn = mbin[0];
  for (i = 0; i < NMBIN; i++) {
    if (mbin[i] > peakn) {
      peaki = i;
      peakn = mbin[i];
    }
  }

  /* select filled bins */
  for (Nb = i = 0; i < peaki; i++) {
    if (mbin[i] > 0) {
      bin[Nb]  = i * dM + MMIN;
      lbin[Nb] = log10 (mbin[i]);
      Nb++;
    }
  }
  Nb = MAX(1, Nb);

  /* find max & min mag bins */
  lum[0].Mmin = bin[0];
  lum[0].Mmax = bin[Nb-1];

  if (Nb < 4) { return (FALSE); }
  fit_lum_bin (bin, lbin, Nb, &C0, &C1);
  
  /* find residuals */
  for (i = 0; i < Nb; i++) {
    rbin[i] = C0 + C1*bin[i] - lbin[i];
  }

  /* keep inner 80% */
  dsortthree (rbin, bin, lbin, Nb);
  for (j = 0, i = 0.1*Nb; i < 0.9*Nb; i++, j++) {
    bin[j]  = bin[i];
    lbin[j] = lbin[i];
  }    
  Nb = j;

  if (Nb < 4) { return (FALSE); }
  fit_lum_bin (bin, lbin, Nb, &C0, &C1);

  lum[0].dNdM = C1;
  lum[0].Mo   = C0;
  
  if (VERBOSE) fprintf (stderr, "lum stats: dNdM = %f, Mo = %f, Mmin = %f, Mmax = %f\n", 
	   lum[0].dNdM, lum[0].Mo, lum[0].Mmin, lum[0].Mmax);

  return (TRUE);

}


void fit_lum_bin (double *x, double *y, int N, double *C0, double *C1) {

  int i;
  double **c, **b;

  ALLOCATE (c, double *, 2);
  ALLOCATE (b, double *, 2);
  ALLOCATE (c[0], double, 2);
  ALLOCATE (c[1], double, 2);
  ALLOCATE (b[0], double, 1);
  ALLOCATE (b[1], double, 1);

  /* fit x, y to line */
  c[0][0] = 0; c[0][1] = 0;
  c[1][0] = 0; c[1][1] = 0;
  b[0][0] = 0; b[1][0] = 0;

  for (i = 0; i < N; i++) {
    c[0][0] += 1;
    c[0][1] += x[i];
    c[1][0] += x[i];
    c[1][1] += x[i]*x[i];
    
    b[0][0] += y[i];
    b[1][0] += y[i]*x[i];
  }
  dgaussjordan (c, b, 2, 1);
  *C0 = b[0][0];
  *C1 = b[1][0];

  free (b[0]);
  free (b[1]);
  free (c[0]);
  free (c[1]);
  free (c);
  free (b);

}
