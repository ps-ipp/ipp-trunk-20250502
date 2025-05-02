# include "imregister.h"
# include "photreg.h"

off_t *match_criteria (PhotPars *photdata, off_t Nphotdata, off_t *Nmatch) {

  off_t i, j;
  off_t N, NMATCH;
  off_t *match;
  off_t reject;

  /* create selection index */
  N = 0;
  NMATCH = 1000;
  ALLOCATE (match, off_t, NMATCH);

  /* find entries that matches criteria */
  for (i = 0; i < Nphotdata; i++) {
    for (j = 0, reject = TRUE; reject && (j < criteria.Ntimes); j++) {
      reject = (photdata[i].tstop < criteria.tstart[j]) || (photdata[i].tstart > criteria.tstop[j]);
    }
    if (criteria.Ntimes && reject) continue;
    if (criteria.PhotCodeSelect && (photdata[i].photcode != criteria.photcode)) continue;
    if (criteria.LabelSelect && strcmp (photdata[i].label, criteria.Label)) continue;

    match[N] = i;
    N ++;
    if (N == NMATCH) {
      NMATCH += 1000;
      REALLOCATE (match, off_t, NMATCH);
    }
  }
  *Nmatch = N;
  return (match);
}
