# include "ppSim.h"

// XXX add bounds to the inputs?
bool ppSimMergeReadouts (pmConfig *config, pmFPAview *view) {

    // bool mdok;

    // if we have an input image, we need to add the synthetic data on top of it below
    // XXX we may potentially use this input file to skip missing elements
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PPSIM.INPUT");
    if (!input) return true;

    pmReadout *inReadout = pmFPAviewThisReadout (view, input->fpa);
    if (!inReadout) return true;

    if (!inReadout->variance) {
	if (!pmReadoutGenerateVariance(inReadout, NULL, true)) {
        psError (PS_ERR_UNKNOWN, false, "trouble creating variance");
        return false;
      }
    }

    // output must exist or we made a programming error
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PPSIM.OUTPUT"); // Output file
    assert(output);

    // XXX require outReadout?
    pmReadout *outReadout = pmFPAviewThisReadout (view, output->fpa);
    if (!outReadout) return true;

    psImage *inSignal = inReadout->image;
    psImage *inVariance = inReadout->variance;

    psImage *outSignal = outReadout->image;
    psImage *outVariance = outReadout->variance;

    assert (inSignal->numRows == outSignal->numRows);
    assert (inSignal->numCols == outSignal->numCols);

    for (int y = 0; y < inSignal->numRows; y++) {
        for (int x = 0; x < inSignal->numCols; x++) {
            outSignal->data.F32[y][x] += inSignal->data.F32[y][x];
            outVariance->data.F32[y][x] += inVariance->data.F32[y][x];
        }
    }
    return true;
}

