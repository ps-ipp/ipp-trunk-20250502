# include "relastro.h"
/* the Measure carries the instantaneous mean position at the epoch t */ 

// average & secfilt no longer used since R and D are now in measure

double getMeanR (MeasureTiny *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  double ra;

  // old: ra = average[0].R - measure[0].dR / 3600.0;
  if (!measure) return NAN;
  ra = measure[0].R;

  return (ra);
}

double getMeanD (MeasureTiny *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  double dec;

  // old: dec = average[0].D - measure[0].dD / 3600.0;
  if (!measure) return NAN;
  dec = measure[0].D;

  return (dec);
}

int setMeanR (double ra_fit, MeasureTiny *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  // old: measure[0].dR += (ra_fit - average[0].R) * 3600.0;
  if (!measure) return FALSE;
  measure[0].R = ra_fit;

  return (TRUE);
}

int setMeanD (double dec_fit, MeasureTiny *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  // old: measure[0].dD += (dec_fit - average[0].D) * 3600.0;
  if (!measure) return FALSE;
  measure[0].D = dec_fit;

  return (TRUE);
}

double getMeanR_Big (Measure *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  double ra;

  // old: ra = average[0].R - measure[0].dR / 3600.0;
  if (!measure) return NAN;
  ra = measure[0].R;

  return (ra);
}

double getMeanD_Big (Measure *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  double dec;

  // old: dec = average[0].D - measure[0].dD / 3600.0;
  if (!measure) return NAN;
  dec = measure[0].D;

  return (dec);
}

int setMeanR_Big (double ra_fit, Measure *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  // old: measure[0].dR += (ra_fit - average[0].R) * 3600.0;
  if (!measure) return TRUE;
  measure[0].R = ra_fit;

  return (TRUE);
}

int setMeanD_Big (double dec_fit, Measure *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  // measure[0].dD += (dec_fit - average[0].D) * 3600.0;
  if (!measure) return TRUE;
  measure[0].D = dec_fit;

  return (TRUE);
}

  /* possible corrections to mean ra:

  - proper-motion and parallax
  - abberation
  - precession and nutation, etc
  - refraction
  - DCR

  */

