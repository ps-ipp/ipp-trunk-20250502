# include "imregister.h"
# include "imreg.h"

off_t *match_criteria (RegImage *image, off_t Nimage, off_t *Nmatch) {

  off_t i, j, Nname;
  off_t N, NMATCH;
  off_t *match;
  off_t reject;

  /* create selection index */
  N = 0;
  NMATCH = 1000;
  ALLOCATE (match, off_t, NMATCH);

  Nname = 0;
  if (criteria.NameSelect) Nname = strlen (criteria.Name);
  
  /* find entries that matches criteria */
  for (i = 0; i < Nimage; i++) {
    for (j = 0, reject = TRUE; reject && (j < criteria.Ntimes); j++) {
      reject = (image[i].obstime + image[i].exptime < criteria.tstart[j]) || (image[i].obstime > criteria.tstop[j]);
    }
    if (criteria.Ntimes && reject) continue;
    if (criteria.FilterSelect  && (strcasecmp (image[i].filter, criteria.Filter))) continue;
    if (criteria.ModeSelect    && (image[i].mode != criteria.Mode)) continue;
    if (criteria.CCDSelect     && (image[i].ccd != criteria.CCD)) continue;
    if (criteria.TypeSelect    && (image[i].type != criteria.Type)) continue;
    if (criteria.ExptimeSelect && (fabs (image[i].exptime - criteria.Exptime) > 5.0)) continue;
    if (criteria.NameSelect    && (strncasecmp (image[i].filename, criteria.Name, Nname))) continue;
    if (criteria.ProcSelect    && (criteria.Proc ^ (image[i].bias != 0.0))) continue;
    if (criteria.DistSelect    && (criteria.Dist ^ (image[i].flag && IMREG_DIST))) continue;

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

off_t *match_images (RegImage *image, off_t Nimage, RegImage *subset, off_t Nsubset, off_t *Nmatch) {
  
  off_t i, j, N, Nfound;
  off_t *match;

  /* find matching images - very inefficient : sort by obstime, find those first? */
  ALLOCATE (match, off_t, Nsubset);
  for (j = 0; j < Nsubset; j++) {
    match[j] = -1;
    for (i = 0; (match[j] == -1) && (i < Nimage); i++) {
      if (image[i].obstime > subset[j].obstime + 1) continue;
      if (image[i].obstime < subset[j].obstime - 1) continue;
      if (image[i].ccd != subset[j].ccd) continue;
      match[j] = i;
    }
  }
  
  Nfound = 0;
  for (i = 0; i < Nsubset; i++) {
    /* set the new values for this image */
    N = match[i];
    if (N == -1) continue;
    image[N].fwhm = subset[i].fwhm; 
    image[N].bias = subset[i].bias; 
    image[N].sky  = subset[i].sky; 
    image[N].ra   = subset[i].ra; 
    image[N].dec  = subset[i].dec; 
    /* if the image is MEF, these were not correctly assigned by imsort.
       this step uses the values from the split ccd image */
    Nfound ++;
  }
  *Nmatch = Nfound;
  return (match);
}
