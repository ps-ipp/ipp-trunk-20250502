# include "psphotInternal.h"

bool psphotIsophotal (pmSource *source, psMetadata *recipe, psImageMaskType maskVal) {

  assert (source->extpars);
  assert (source->extpars->profile);
  assert (source->extpars->profile->radius);
  assert (source->extpars->profile->flux);

  bool status;

  psVector *radius = source->extpars->profile->radius;
  psVector *flux = source->extpars->profile->flux;

  // flux at which to measure isophotal parameters
  // XXX ISOPHOTAL_FLUX should be specified in mags, need the zero point to get counts/sec
  float ISOPHOT_FLUX = psMetadataLookupF32 (&status, recipe, "ISOPHOTAL_FLUX");
  assert (status);

  // find the first bin below the flux level and the last above the level
  // XXX can this be done faster with bisection?
  // XXX do I need to worry about crazy outliers?
  // XXX should i be smoothing or fitting the curve?
  int firstBelow = -1;
  int lastAbove = -1;
  for (int i = 0; i < flux->n; i++) {
    if (flux->data.F32[i] > ISOPHOT_FLUX) lastAbove = i;
    if ((firstBelow < 0) && (flux->data.F32[i] < ISOPHOT_FLUX)) firstBelow = i;
  }
  // if we don't go out far enough, we have a problem...
  if (lastAbove == flux->n - 1) {
    psTrace ("psphot", 5, "did not go out far enough to reach isophotal magnitude");
    // XXX raise a flag ?
    return false;
  }
  if (firstBelow < 0) {
    psTrace ("psphot", 5, "did not go out far enough to bound isophotal magnitude: error unmeasured");
    // XXX raise a flag ?
    lastAbove = firstBelow;
    return false;
  }

  // need to examine pixels in this vicinity
  float isophotalFluxFirst = 0;
  float isophotalFluxLast = 0;
  for (int i = 0; i <= PS_MAX(firstBelow, lastAbove); i++) {
    if (i <= firstBelow) {
      isophotalFluxFirst += flux->data.F32[i];
    }
    if (i <= lastAbove) {
      isophotalFluxLast += flux->data.F32[i];
    }
  }
  float isophotalFlux    = 0.5*(isophotalFluxLast + isophotalFluxFirst);
  float isophotalFluxErr = 0.5*fabs(isophotalFluxLast - isophotalFluxFirst);

  float isophotalRad     = 0.5*(radius->data.F32[firstBelow] + radius->data.F32[lastAbove]);
  float isophotalRadErr  = 0.5*fabs(radius->data.F32[firstBelow] - radius->data.F32[lastAbove]);

  if (!source->extpars->isophot) {
    source->extpars->isophot = pmSourceIsophotalValuesAlloc ();
  }

  // these are uncalibrated: instrumental mags and pixel units
  source->extpars->isophot->mag    = -2.5*log10(isophotalFlux);
  source->extpars->isophot->magErr = isophotalFluxErr / isophotalFlux;

  source->extpars->isophot->rad    = isophotalRad;
  source->extpars->isophot->radErr = isophotalRadErr;

  psTrace ("psphot", 5, "Isophot flux:%f +/- %f @ %f +/- %f for %f, %f\n",
           source->extpars->isophot->mag, source->extpars->isophot->magErr,
           source->extpars->isophot->rad, source->extpars->isophot->radErr,
           source->peak->xf, source->peak->yf);

  return true;

}
