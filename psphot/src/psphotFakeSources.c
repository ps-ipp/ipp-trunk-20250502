# include "psphotInternal.h"

psArray *psphotFakeSources () {

    // psphotUpdateHeader (header, config);

    psArray *sources = psArrayAlloc (50);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = pmSourceAlloc ();
        source->moments = pmMomentsAlloc ();
        source->moments->Mx = 10;
        source->moments->My = 10;
        source->moments->Mxx = 1;
        source->moments->Myy = 1;
        source->moments->Mxy = 0;
        source->moments->Sum = 1000;
        source->moments->Peak = 100;
        source->moments->Sky = 10;
        source->moments->nPixels = 10;

        source->peak = pmPeakAlloc (10, 10, 0, 0);
        source->type = PM_SOURCE_TYPE_STAR;

        pmModelType modelType = pmModelClassGetType ("PS_MODEL_QGAUSS");
        source->modelPSF = pmSourceModelGuess (source, modelType, 0, 0);
        sources->data[i] = source;
    }
    return sources;
}
