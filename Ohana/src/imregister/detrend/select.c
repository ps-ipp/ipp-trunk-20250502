# include "imregister.h"
# include "detrend.h"

Match SelectEntry (DetReg *image, off_t Nimage, Match *list, off_t Nlist, Criteria *crit) {

  /* force a single selection */

  /* at this point, all external criteria for the given detrend images
     are equivalently good (ie, date range, filters, type, etc).  the 
     selection here is based on other options: 
     1) Norder         - user assigned value to force selection
     2) tstart         - start valid time
     3) Nentry         - version number
     4) MatchNumber    - method to force selection 
  */

  off_t i, j, entry, order, matchnum;
  off_t nmatch, Nmatch;
  unsigned int tstart;
  Match *match, *tmatch, answer;

  Nmatch = Nlist;
  match = list;

  /* select all images with the same, maximum Norder */
  nmatch = 0;
  ALLOCATE (tmatch, Match, Nmatch);
  order = image[match[0].image].Norder;
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    order = MAX (order, image[i].Norder);
  }
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    if (order <= image[i].Norder) {
      tmatch[nmatch] = match[j];
      nmatch ++;
    }
  }
  match = tmatch;
  Nmatch = nmatch;

  /* select all images with the same, maximum tstart */
  nmatch = 0;
  ALLOCATE (tmatch, Match, Nmatch);
  tstart = image[match[0].image].tstart;
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    tstart = MAX (tstart, image[i].tstart);
  }
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    if (tstart <= image[i].tstart) {
      tmatch[nmatch] = match[j];
      nmatch ++;
    }
  }
  free (match);
  match = tmatch;
  Nmatch = nmatch;

  /* select all images with the same, maximum Nentry */
  nmatch = 0;
  ALLOCATE (tmatch, Match, Nmatch);
  entry = image[match[0].image].Nentry;
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    entry = MAX (entry, image[i].Nentry);
  }
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    if (entry <= image[i].Nentry) {
      tmatch[nmatch] = match[j];
      nmatch ++;
    }
  }
  free (match);
  match = tmatch;
  Nmatch = nmatch;

  /* if there are multiple matches, select MatchNumber entry 
     0-Nmatch-1 , or -1 -> -Nmatch (saturate at ends) */
  matchnum = crit[match[0].crit].MatchNumber;
  if (matchnum < 0) matchnum += Nmatch;
  matchnum = MAX (0, MIN (Nmatch - 1, matchnum));

  answer = match[matchnum];
  free (match);
  return (answer);
}
