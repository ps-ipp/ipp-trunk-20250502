# include "imregister.h"
# include "spreg.h"

off_t *match_criteria (Spectrum *spectrum, off_t Nspectrum, off_t *Nmatch) {

  off_t i, j;
  off_t N, NMATCH;
  off_t *match;
  off_t reject;
  off_t Nfilename, Nobject, Ntelescope, Ninstrument;

  /* create selection index */
  N = 0;
  NMATCH = 1000;
  ALLOCATE (match, off_t, NMATCH);

  Nfilename = Nobject = Ntelescope = Ninstrument = 0;

  if (criteria.FilenameSelect)   Nfilename   = strlen (criteria.Filename);
  if (criteria.ObjectSelect)     Nobject     = strlen (criteria.Object);
  if (criteria.TelescopeSelect)  Ntelescope  = strlen (criteria.Telescope);
  if (criteria.InstrumentSelect) Ninstrument = strlen (criteria.Instrument);

  /* find entries that matches criteria */
  for (i = 0; i < Nspectrum; i++) {
    for (j = 0, reject = TRUE; reject && (j < criteria.Ntimes); j++) {
      reject = (spectrum[i].obstime + spectrum[i].exptime < criteria.tstart[j]) || (spectrum[i].obstime > criteria.tstop[j]);
    }
    if (criteria.Ntimes && reject) continue;
    if (criteria.ModeSelect    && (spectrum[i].mode  != criteria.Mode)) continue;
    if (criteria.StateSelect   && (spectrum[i].state != criteria.State)) continue;

    if (criteria.ExptimeSelect && (fabs (spectrum[i].exptime - criteria.Exptime) > 5.0)) continue;

    if (criteria.FilenameSelect   && (strncasecmp (spectrum[i].filename, criteria.Filename, Nfilename))) continue;
    if (criteria.ObjectSelect     && (strncasecmp (spectrum[i].objname, criteria.Object, Nobject))) continue;
    if (criteria.TelescopeSelect  && (strncasecmp (spectrum[i].telescope, criteria.Telescope, Ntelescope))) continue;
    if (criteria.InstrumentSelect && (strncasecmp (spectrum[i].instrument, criteria.Instrument, Ninstrument))) continue;

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

# if (0)

off_t *match_spectra (Spectrum *subset, off_t Nsubset, off_t *Nmatch) {
  
  off_t i, j, N, Nspectrum, Nfound;
  off_t *match;
  Spectrum *spectrum;

  spectrum = get_spectra (&Nspectrum);

  /* find matching spectra - very inefficient : sort by obstime, find those first? */
  ALLOCATE (match, off_t, Nsubset);
  for (j = 0; j < Nsubset; j++) {
    match[j] = -1;
    for (i = 0; (match[j] == -1) && (i < Nspectrum); i++) {
      if (spectrum[i].obstime > subset[j].obstime + 1) continue;
      if (spectrum[i].obstime < subset[j].obstime - 1) continue;
      match[j] = i;
    }
  }
  
  Nfound = 0;
  for (i = 0; i < Nsubset; i++) {
    /* set the new values for this spectrum */
    N = match[i];
    if (N == -1) continue;
    spectrum[N].ra   = subset[i].ra; 
    spectrum[N].dec  = subset[i].dec; 
    /* if the spectrum is MEF, these were not correctly assigned by imsort.
       this step uses the values from the split ccd spectrum */
    Nfound ++;
  }
  *Nmatch = Nfound;
  return (match);
}

# endif
