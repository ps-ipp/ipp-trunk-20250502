# include "ppSim.h"

// compare injected sources (PPSIM.SOURCES) and measured fake sources (PPSIM.FAKE.SOURCES) 
// to determine detection limits and recovered magnitude errors
// XXX this function is not used
bool ppSimDetectionLimits (pmConfig *config, pmFPAview *view) {

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // XXX do we need to ask if the analysis was performed to 1st or 2nd detection limit?
    float NSIGMA_PEAK = psMetadataLookupF32 (&status, recipe, "PEAKS_NSIGMA_LIMIT_2");

    // find the currently selected readout. 
    // we always perform photometry on the mosaiced chip
    pmReadout  *readout = pmFPAfileThisReadout (config->files, view, "PPSIM.FAKE.CHIP");
    PS_ASSERT_PTR_NON_NULL (readout, false);

    psArray *injectedSources = psMetadataLookupPtr (NULL, readout->analysis, "PPSIM.SOURCES");
    psArray *measuredSources = psMetadataLookupPtr (NULL, readout->analysis, "PPSIM.FAKE.SOURCES");
    psAssert (injectedSources->n == measuredSources->n, "mis-match between injected and measured sources");
  
    psVector *mag    = psVectorAlloc (injectedSources->n, PS_TYPE_F32);
    psVector *dmag   = psVectorAlloc (injectedSources->n, PS_TYPE_F32);
    psVector *SN     = psVectorAlloc (injectedSources->n, PS_TYPE_F32);
    psVector *detect = psVectorAlloc (injectedSources->n, PS_TYPE_BOOL);

    // first measure, for each source, if it was detected (SN > limit)
    // and the magnitude offset (dM = M_inject - M_measure)
    for (int i = 0; i < injectedSources->n; i++) {
      pmSource *injectSource = injectedSources->data[i];
      pmSource *measureSource = measuredSources->data[i];

      SN->data.F32[i] = measureSource->peak->SN;
      dmag->data.F32[i] = injectSource->psfMag - measureSource->psfMag;
      mag->data.F32[i] = injectSource->psfMag;
      detect->data.Bool[i] = (measureSource->peak->SN >= NSIGMA_PEAK);
    }

    // generate a histogram consisting of the instrumental magnitude bin, the 
    // XXX what is resolution? (user parameter?)

  return true;
}
