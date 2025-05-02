# include "ppSim.h"

// XXX this function is not used
bool ppSimDetections (psImage *significance, psMetadata *recipe, psArray *sources) {
    psAssert (sources, "programming error: ppSimDetections passed NULL sources");

    bool status;

    // for each source, measure the significance of the peak at the given coordinate
    // where does this get stored?

    // XXX need to get the effective Area from the PSF sigma
    float SIGMA_SMTH  = psMetadataLookupF32 (&status, recipe, "SIGMA_SMOOTH"); 
    psAssert (status, "SIGMA_SMOOTH missing: call psphotSignificanceImage first"); 
    // float effArea = 4.0*M_PI*PS_SQR(SIGMA_SMTH);

    int row0 = significance->row0;
    int col0 = significance->col0;

    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	pmPeak *peak = source->peak;
	psAssert (peak, "peak is not defined for the source");

	peak->detValue = significance->data.F32[peak->y-row0][peak->x-col0];
    }
    return true;
}
