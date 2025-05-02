# include "psphotInternal.h"

bool psphotAnnuli (pmSource *source, psMetadata *recipe, psImageMaskType maskVal) {

  assert (source->extpars);
  assert (source->extpars->profile);
  assert (source->extpars->profile->radius);
  assert (source->extpars->profile->flux);

  bool status;

  psVector *radius = source->extpars->profile->radius;
  psVector *variance = source->extpars->profile->variance;
  psVector *flux = source->extpars->profile->flux;

  // XXX how do I define the radii?  we can put a vector in the recipe...
  // radialBins defines the bounds or start and stop (we can skip some that way...
  psVector *radialBinsLower = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.LOWER");
  psVector *radialBinsUpper = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.UPPER");
  assert (radialBinsLower->n == radialBinsUpper->n);

  psVector *fluxValues = psVectorAlloc (radialBinsLower->n, PS_TYPE_F32);
  psVector *fluxSquare = psVectorAlloc (radialBinsLower->n, PS_TYPE_F32);
  psVector *fluxVariance = psVectorAlloc (radialBinsLower->n, PS_TYPE_F32);
  psVector *pixelCount = psVectorAlloc (radialBinsLower->n, PS_TYPE_F32);
  psVectorInit (fluxValues, 0.0);
  psVectorInit (fluxSquare, 0.0);
  psVectorInit (fluxVariance, 0.0);
  psVectorInit (pixelCount, 0.0);

  // XXX this code assumes the radii are in pixels.  convert from arcsec with plate scale
  // XXX assume the annulii above are not overlapping?  much faster...
  // XXX this might be must faster in the reverse order: loop over annulii and use disection to
  // skip to the start of the annulus.
  for (int i = 0; i < flux->n; i++) {
    for (int j = 0; j < radialBinsLower->n; j++) {
      if (radius->data.F32[i] < radialBinsLower->data.F32[j]) continue;
      if (radius->data.F32[i] > radialBinsUpper->data.F32[j]) continue;
      fluxValues->data.F32[j] += flux->data.F32[i];
      fluxSquare->data.F32[j] += PS_SQR(flux->data.F32[i]);
      fluxVariance->data.F32[j] += variance->data.F32[i];
      pixelCount->data.F32[j] += 1.0;
    }
  }

  for (int j = 0; j < radialBinsLower->n; j++) {
    fluxValues->data.F32[j] /= pixelCount->data.F32[j];
    fluxSquare->data.F32[j] /= pixelCount->data.F32[j];
    fluxSquare->data.F32[j] -= PS_SQR(fluxValues->data.F32[j]);
  }

  source->extpars->annuli = pmSourceAnnuliAlloc ();
  source->extpars->annuli->flux    = fluxValues;
  source->extpars->annuli->fluxErr = fluxVariance;
  source->extpars->annuli->fluxVar = fluxSquare;

  psFree (pixelCount);

  return true;
}

