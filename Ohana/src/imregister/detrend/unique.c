# include "imregister.h"
# include "detrend.h"

/* define a list of best images which have unique properties */ 
Match *UniqueSubset (DetReg *image, off_t Nimage, Match *match, off_t *nmatch) {

  int found;
  off_t Nsubset, Nunique, Ncrit, NCRIT;
  off_t i, j, N, Nmatch;
  Criteria *crit;
  Match *subset, *unique, *local;

  Nmatch = *nmatch;
  ALLOCATE (local, Match, Nmatch);

  /* create a set of complete criteria derived from the images */

  Ncrit = 0;
  NCRIT = 10;
  ALLOCATE (crit, Criteria, NCRIT);
  bzero (crit, (NCRIT - Ncrit)*sizeof(Criteria));

  for (i = 0; i < Nmatch; i++) {
    
    local[i] = match[i];
    N = match[i].image;
    if (N == -1) continue;

    found = FALSE;
    for (j = 0; (j < Ncrit) && !found; j++) {
      if (!cmp_crit (&crit[j], &image[N])) continue;
      found = TRUE;
      local[i].crit  = j;
    }      
    if (found) continue;
    local[i].crit  = Ncrit;
    set_crit (&crit[Ncrit], &image[N]);
    Ncrit ++;
    if (Ncrit == NCRIT) {
      NCRIT += 10;
      REALLOCATE (crit, Criteria, NCRIT);
      bzero (&crit[Ncrit], (NCRIT - Ncrit)*sizeof(Criteria));
    }
  }
      
  /* we now have a set of criteria (crit, Ncrit) which describe the matched images
     and a set of local matches (local) which link images to crit */

  ALLOCATE (subset, Match, Nmatch);
  ALLOCATE (unique, Match, Nmatch);

  Nunique = 0;
  for (i = 0; i < Ncrit; i++) {

    /* extract the subset of local matches which are associated with crit[i] */
    Nsubset= 0;
    for (j = 0; j < Nmatch; j++) {
      if (local[j].crit  != i) continue;
      subset[Nsubset] = local[j];
      Nsubset ++;
    }

    /* subset, Nsubset has a unique list of matched images */
    unique[Nunique] = SelectEntry (image, Nimage, subset, Nsubset, crit);
    Nunique ++;
  }
  free (subset);
  free (local);
  free (match);

  match = unique;
  *nmatch = Nunique;

  return (match);
}


/* 
   return list of all images that have the same value of:
   criteria = tstart, tstop, filter, ccd, type, exptime
*/


int cmp_crit (Criteria *crit, DetReg *image) {

  /* image and criteria need to match on:
     tstart, tstop, filter, ccd, type, exptime
  */
  
  if (crit[0].Filter  != image[0].filter ) return (FALSE);
  if (crit[0].CCD     != image[0].ccd    ) return (FALSE);
  if (crit[0].Type    != image[0].type   ) return (FALSE);
  /* if (crit[0].Exptime != image[0].exptime) return (FALSE); */

  if (crit[0].tstart  > image[0].tstop) return (FALSE);
  if (crit[0].tstop   < image[0].tstart) return (FALSE);

  /* we have overlapping time ranges.  set crit[0] to have the 
     maximum of these time ranges */

  crit[0].tstop  = MIN (crit[0].tstop,  image[0].tstop);
  crit[0].tstart = MAX (crit[0].tstart, image[0].tstart);

  return (TRUE);

}

int set_crit (Criteria *crit, DetReg *image) {

  /* image and criteria need to match on:
     tstart, tstop, filter, ccd, type, exptime
  */

  crit[0].tstart  = image[0].tstart;
  crit[0].tstop   = image[0].tstop;  
  crit[0].Filter  = image[0].filter; 
  crit[0].CCD     = image[0].ccd;    
  crit[0].Type    = image[0].type;   
  crit[0].Exptime = image[0].exptime;

  return (TRUE);

}

