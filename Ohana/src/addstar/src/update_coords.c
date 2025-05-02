# include "addstar.h"

/* find the average and scatter for R and D - no outlier rejection */
void update_coords (Average *average, Measure *measure, off_t *next) {

  off_t i, m, Npt;
  double R, D, r, d;
  double r2, d2, dR2, dD2;

  Npt = r = d = r2 = d2 = 0;

  if (average[0].Nmeasure < 2) return;

  /* find the average & sum-square (does not use reference coordinates) */
  m = average[0].measureOffset;  /* first measurement of this star */
  for (i = 0; i < average[0].Nmeasure; i++) {
    if (measure[m].t == 0) {
      m = next[m];
      continue;
    }
    R = measure[m].R;
    D = measure[m].D;
    r += R;
    d += D;
    r2 += R*R;
    d2 += D*D;
    m = next[m];
    Npt ++;
  }
  if (Npt < 1) return;

  /* apply average offset */
  r = r / Npt;  /* these are corrections in 1/100 arcsec to RA and DEC */
  d = d / Npt;
  average[0].R = r;
  average[0].D = d;
  m = average[0].measureOffset;  /* first measurement of this star */

  /* measure scatter, if possible */
  if (Npt < 2) return;

  dR2 = r2 / Npt - r*r;
  dD2 = d2 / Npt - d*d;
  average[0].ChiSqAve = 3600.0*sqrt (dD2 + dR2 / SQ(cos(d*RAD_DEG)));
  /* ChiSqAve is supposed to be a chisq */

  return;
}
