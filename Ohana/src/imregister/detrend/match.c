# include "imregister.h"
# include "detrend.h"

Match *MatchCriteria (DetReg *image, off_t Nimage, off_t *nmatch) {

  off_t i;
  off_t Nmatch, NMATCH;
  Match result, *match;

  /* find entries that matches criteria */
  Nmatch = 0;
  NMATCH = 100;
  ALLOCATE (match, Match, NMATCH);

  /* find set of images that matches criteria */
  for (i = 0; i < Nimage; i++) {
    result = CheckCriteria (&image[i]);
    if (result.state == MATCH_NONE) continue;

    result.image = i;
    match[Nmatch] = result;
    Nmatch ++;
    if (Nmatch == NMATCH) {
      NMATCH += 100;
      REALLOCATE (match, Match, NMATCH);
    }
  }

  *nmatch = Nmatch;
  return (match);
  
}
