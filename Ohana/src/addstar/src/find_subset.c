# include "addstar.h"

/* find stars within this region */
Stars **find_subset (SkyRegion *region, Stars *stars, unsigned int Nstars, unsigned int *Nsubset) {

  int i, N, NSUBSET;
  Stars **subset;
  double RA0, RA1, DEC0, DEC1;

  NSUBSET = 1000;
  ALLOCATE (subset, Stars *, NSUBSET);

  RA0  = region[0].Rmin;
  RA1  = region[0].Rmax;
  DEC0 = region[0].Dmin;
  DEC1 = region[0].Dmax;

  if (VERBOSE) fprintf (stderr, "subset for %f - %f, %f - %f\n", RA0, RA1, DEC0, DEC1);

  /* find stars within ra,dec region */
  for (i = N = 0; i < Nstars; i++) {
    if (stars[i].average.R <  RA0)  continue;
    if (stars[i].average.R >= RA1)  continue;
    if (stars[i].average.D <  DEC0) continue;
    if (stars[i].average.D >= DEC1) continue;

    subset[N] = &stars[i];
    N++;
    if (N == NSUBSET - 1) {
      NSUBSET += 1000;
      REALLOCATE (subset, Stars *, NSUBSET);
    }
  }
  *Nsubset = N;
  return (subset);
}
