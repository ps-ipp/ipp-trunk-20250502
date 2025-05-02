# include "psphotInternal.h"

// This is only used by psphotModelTest.c
bool psphotFitSet (pmSource *source, pmModel *oneModel, char *fitset, pmSourceFitMode mode, psImageMaskType maskVal) {

    double x, y, Io;

    FILE *f = fopen (fitset, "r");
    if (f == NULL) return false;

    psArray *modelSet = psArrayAllocEmpty (16);

    while (fscanf (f, "%lf %lf %lf", &x, &y, &Io) == 3) {
        pmModel *model = pmModelAlloc (oneModel->type);

        for (psS32 i = 0; i < model->params->n; i++) {
            model->params->data.F32[i] = oneModel->params->data.F32[i];
            model->dparams->data.F32[i] = oneModel->dparams->data.F32[i];
        }
        model->params->data.F32[1] = Io;
        model->params->data.F32[2] = x;
        model->params->data.F32[3] = y;
        psArrayAdd (modelSet, 16, model);
    }

    // Define source fitting parameters for extended source fits
    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->mode          = PM_SOURCE_FIT_EXT;
    fitOptions->covarFactor   = 1.0;
    // XXX for now, use the defaults for the rest:
    // fitOptions->nIter         = fitIter;
    // fitOptions->tol           = fitTol;
    // fitOptions->poissonErrors = poisson;
    // fitOptions->weight        = PS_SQR(skySig);

    // XXX pmSourceFitSet must cache the modelFlux?
    pmSourceFitSet (source, modelSet, fitOptions, maskVal);

    // write out positive object
    psphotSaveImage (NULL, source->pixels, "object.fits");

    // subtract object, leave local sky
    for (int i = 0; i < modelSet->n; i++) {
        pmModel *model = modelSet->data[i];
        pmModelSub (source->pixels, source->maskObj, model, PM_MODEL_OP_FULL, maskVal);

        fprintf (stderr, "output parameters (obj %d):\n", i);
        for (int n = 0; n < model->params->n; n++) {
            fprintf (stderr, "%d : %f\n", n, model->params->data.F32[n]);
        }
    }

    // write out
    psphotSaveImage (NULL, source->pixels, "resid.fits");
    psphotSaveImage (NULL, source->maskObj, "mask.fits");
    return true;
}

