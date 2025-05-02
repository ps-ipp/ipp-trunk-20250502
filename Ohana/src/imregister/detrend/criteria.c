# include "imregister.h"
# include "detrend.h"

Match CheckCriteria (DetReg *image) {

  Match match;
  int i, close, crit;

  match.state = MATCH_NONE;
  match.crit  = 0;
  match.image = -1;

  crit = 0;
  close = FALSE;
  for (i = 0; (i < Ncriteria) && (match.state == MATCH_NONE); i++) {

    if (criteria[i].CCDSelect) {
      if (image[0].mode == M_MEF) goto valid_ccd;
      if (image[0].mode == M_MODES) goto valid_ccd;
      if (image[0].ccd  == criteria[i].CCD) goto valid_ccd;
      continue;
    }
  valid_ccd:

    if (criteria[i].TypeSelect   && (image[0].type            != criteria[i].Type))   continue;
    if (criteria[i].ModeSelect   && (image[0].mode            != criteria[i].Mode))   continue;
    if (criteria[i].FilterSelect && (image[0].filter          != criteria[i].Filter)) continue;

    if (criteria[i].EntrySelect  && (image[0].Nentry          != criteria[i].Entry))  continue;
    if (criteria[i].LabelSelect  && (strcasecmp (image[0].label, criteria[i].Label))) continue;
    if (criteria[i].NameSelect   && (strstr (image[0].filename, criteria[i].Name) == (char *) NULL)) continue;

    /* looking for the best 'close' match: minimum |dt| */
    if (criteria[i].TimeSelect   && (image[0].tstart           > criteria[i].tstop))  {
      close = TRUE;
      crit  = i;
      continue;
    } 
    if (criteria[i].TimeSelect   && (image[0].tstop            < criteria[i].tstart)) {
      close = TRUE;
      crit  = i;
      continue;
    } 
    match.crit  = i;
    match.state = MATCH_PERFECT;
  }

  if ((match.state == MATCH_NONE) && close) {
    match.crit  = crit;
    match.state = MATCH_CLOSE;
  }    
  return (match);

}

/* 
   the image only needs to match one criterion
   close only counts for TimeSelect
   multiple criteria can exist for Range, CCD, Filter
*/

Match *ExptimeCriteria (DetReg *image, off_t Nimage, Match *match, off_t *nmatch) {

  off_t i, j, Nmatch, Nnew, NNEW, entry;
  unsigned long dtime;
  float Chi, ChiMin;
  Match *new;

  Nmatch = *nmatch;

  Nnew = 0;
  NNEW = 100;
  ALLOCATE (new, Match, NNEW);

  for (i = 0; i < Ncriteria; i++) {

    /* find min Chi value */
    ChiMin = FLT_MAX;
    for (j = 0; j < Nmatch; j++) {
      if (match[j].crit != i) continue;
      if (!criteria[i].ExptimeSelect) {
	new[Nnew] = match[j];
	Nnew ++;
	if (Nnew == NNEW) {
	  NNEW += 100;
	  REALLOCATE (new, Match, NNEW);
	}
	continue;
      }

      entry = match[j].image;

      dtime = 0;
      if (criteria[i].TimeSelect) {
	if (criteria[i].tstart > image[entry].tstop) 
	  dtime = criteria[i].tstart - image[entry].tstop;
	if (criteria[i].tstop < image[entry].tstart) 
	  dtime = image[entry].tstart - criteria[i].tstop;
      }
      Chi = dtime / 864.0 + abs (criteria[i].Exptime - image[entry].exptime);
      ChiMin = MIN (Chi, ChiMin);
    }

    /* select entries with Chi <= ChiMin */
    for (j = 0; j < Nmatch; j++) {
      if (match[j].crit != i) continue;
      if (!criteria[i].ExptimeSelect) continue;
      entry = match[j].image;
      
      dtime = 0;
      if (criteria[i].TimeSelect) {
	if (criteria[i].tstart > image[entry].tstop) 
	  dtime = criteria[i].tstart - image[entry].tstop;
	if (criteria[i].tstop < image[entry].tstart) 
	  dtime = image[entry].tstart - criteria[i].tstop;
      }
      Chi = dtime / 864.0 + abs (criteria[i].Exptime - image[entry].exptime);
      if (Chi <= ChiMin) {
	new[Nnew] = match[j];
	Nnew ++;
	if (Nnew == NNEW) {
	  NNEW += 100;
	  REALLOCATE (new, Match, NNEW);
	}
      }
    }
  }

  free (match);
  *nmatch = Nnew;
  return (new);
}

Match *CloseCriteria (DetReg *image, off_t Nimage, Match *match, off_t *nmatch) {

  off_t i, j, Nmatch, Nnew, NNEW, Ngood, entry;
  unsigned long dmin, dtime;
  Match *new;

  Nmatch = *nmatch;

  Nnew = 0;
  NNEW = 100;
  ALLOCATE (new, Match, NNEW);

  for (i = 0; i < Ncriteria; i++) {

    Ngood = 0;

    /* first include only perfect matches */
    for (j = 0; j < Nmatch; j++) {
      if (match[j].crit  != i) continue;
      if (match[j].state != MATCH_PERFECT) continue;

      Ngood ++;
      new[Nnew] = match[j];
      Nnew ++;
      if (Nnew == NNEW) {
	NNEW += 100;
	REALLOCATE (new, Match, NNEW);
      }	
    } 

    if (Ngood) continue;
    if (!output.Close) continue;
    if (!criteria[i].TimeSelect) continue;

    /* find closest time */
    dmin = 0xffffffff;
    for (j = 0; j < Nmatch; j++) {
      if (match[j].crit  != i) continue;
      entry = match[j].image;
      dtime = (image[entry].tstop < criteria[i].tstart) ?  criteria[i].tstart - image[entry].tstop : image[entry].tstart - criteria[i].tstop;
      dmin = MIN (dmin, dtime);
    }

    /* select images with dtime == dmin */
    for (j = 0; j < Nmatch; j++) {
      if (match[j].crit  != i) continue;
      entry = match[j].image;
      dtime = (image[entry].tstop < criteria[i].tstart) ?  criteria[i].tstart - image[entry].tstop : image[entry].tstart - criteria[i].tstop;
      if (dtime <= dmin) {
	new[Nnew] = match[j];
	Nnew ++;
	if (Nnew == NNEW) {
	  NNEW += 100;
	  REALLOCATE (new, Match, NNEW);
	}	
      }
    }
  }

  free (match);
  *nmatch = Nnew;
  return (new);
}





/* CheckCriteria returns two pieces of information:
   match state (MATCH_NONE, MATCH_CLOSE, MATCH_PERFECT)
   match entry (i)
*/
   
/* match[] contains all potentially valid matches.
   if they are darks, there will be several with similar values for detdata[i].tstop
   we want to minimize:
   (tzero - tstop) and abs (ExpTime - detdata[i].exptime) 

   Chi = (tzero - tstop) / 864 + abs (ExpTime - detdata[i].exptime)

   if (tzero - tstop) = 2 days, this term contributes 200.  
*/


/* CloseCriteria returns all images which are either a perfect match
   or which are close and have the same minimum time distance */
