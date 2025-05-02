#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

bool fitModels (psRandom *seed, float flux, float radius, float sigma);
bool fitModelFlux (psRandom *seed, float flux, float radius, float sigma);
bool printDev (float *src, float *par, int Npar, bool absolute);

int main (void)
{
    pmModelGroupInit ();
    pmSourceFitModelInit (15, 0.01, 1.0, true);

    float flux = 10000;
    float sigma = 2.0;
    float radius = 10.0;

    plan_tests(240);

    // build a gauss-deviate vector (mean = 0.0, sigma = 1.0)
    psRandom *seed = psRandomAllocSpecific (PS_RANDOM_TAUS, 0);

    // noise vector to noise up the image
    psVector *rnd = psVectorAlloc (1000, PS_TYPE_F32);
    for (int i = 0; i < rnd->n; i++) {
        rnd->data.F32[i] = psRandomGaussian (seed);
    }

    // construct a GAUSS model
    pmSource *source = pmSourceAlloc ();
    source->moments = pmMomentsAlloc ();

    pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
    source->modelEXT = pmModelAlloc (type);

    source->modelEXT->params->data.F32[0] = 0;
    source->modelEXT->params->data.F32[1] = flux;
    source->modelEXT->params->data.F32[2] = 50;
    source->modelEXT->params->data.F32[3] = 50;
    source->modelEXT->params->data.F32[4] = 2.0*sqrt(sigma);
    source->modelEXT->params->data.F32[5] = 2.0*sqrt(sigma);
    source->modelEXT->params->data.F32[6] = 0;

    source->pixels = psImageAlloc (100, 100, PS_TYPE_F32);
    source->weight = psImageAlloc (100, 100, PS_TYPE_F32);
    source->mask   = psImageAlloc (100, 100, PS_TYPE_U8);
    psImageInit (source->pixels, 0.0);
    psImageInit (source->weight, 0.0);
    psImageInit (source->mask, 0);

    // create an image with the model, and add noise: gain is 1, subtracted sky is 100, readnoise is 5
    pmModelAdd (source->pixels, source->mask, source->modelEXT, PM_MODEL_OP_FULL);
    int npix = 0;
    for (int j = 0; j < source->pixels->numRows; j++) {
        for (int i = 0; i < source->pixels->numCols; i++) {
            float flux = source->pixels->data.F32[j][i];
            float var = flux + 100 + PS_SQR(5);
            source->pixels->data.F32[j][i] += rnd->data.F32[npix]*sqrt(var);
            source->weight->data.F32[j][i] = var;
            npix ++;
            if (npix == rnd->n)
                npix = 0;
        }
    }

    // fit with a PGAUSS model
    pmModel *guess;

    type = pmModelClassGetType ("PS_MODEL_PGAUSS");
    guess = pmModelAlloc (type);

    guess->params->data.F32[0] = 0;
    guess->params->data.F32[1] = flux;
    guess->params->data.F32[2] = 50;
    guess->params->data.F32[3] = 50;
    guess->params->data.F32[4] = 2.0*sqrt(sigma);
    guess->params->data.F32[5] = 2.0*sqrt(sigma);
    guess->params->data.F32[6] = 0;
    // modify guess
    guess->params->data.F32[1] *= 0.9;
    guess->params->data.F32[2] += 1.0;
    guess->params->data.F32[3] -= 1.0;
    guess->params->data.F32[4] *= 0.9;
    guess->params->data.F32[5] *= 0.9;

    psImageKeepCircle (source->mask, 50, 50, radius, "OR", PM_MASK_MARK);
    pmSourceFitModel (source, guess, PM_SOURCE_FIT_EXT);
    psImageKeepCircle (source->mask, 50, 50, radius, "AND", PS_NOT_U8(PM_MASK_MARK));

    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 1, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 2, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 3, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 4, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 5, false);

    psImageKeepCircle (source->mask, 50, 50, radius, "OR", PM_MASK_MARK);
    pmSourceFitModel (source, guess, PM_SOURCE_FIT_PSF);
    psImageKeepCircle (source->mask, 50, 50, radius, "AND", PS_NOT_U8(PM_MASK_MARK));

    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 1, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 2, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 3, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 4, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 5, false);

    // muck up the Sx, Sy terms a little : how does this affect the dparams?
    float Sx = guess->params->data.F32[4];
    float Sy = guess->params->data.F32[5];
    guess->params->data.F32[4] = 0.95*Sx;
    guess->params->data.F32[5] = 0.95*Sy;

    psImageKeepCircle (source->mask, 50, 50, radius, "OR", PM_MASK_MARK);
    pmSourceFitModel (source, guess, PM_SOURCE_FIT_PSF);
    psImageKeepCircle (source->mask, 50, 50, radius, "AND", PS_NOT_U8(PM_MASK_MARK));

    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 1, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 2, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 3, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 4, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 5, false);

    // muck up the Sx, Sy terms a little : how does this affect the dparams?
    guess->params->data.F32[4] = 0.99*Sx;
    guess->params->data.F32[5] = 0.99*Sy;

    psImageKeepCircle (source->mask, 50, 50, radius, "OR", PM_MASK_MARK);
    pmSourceFitModel (source, guess, PM_SOURCE_FIT_PSF);
    psImageKeepCircle (source->mask, 50, 50, radius, "AND", PS_NOT_U8(PM_MASK_MARK));

    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 1, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 2, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 3, true);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 4, false);
    printDev (source->modelEXT->params->data.F32, guess->params->data.F32, 5, false);

    psFree (rnd);
    psFree (source);
    psFree (guess);

    return true;
}

bool printDev (float *src, float *fit, int Npar, bool absolute)
{
    float dev;
    if (absolute) {
        dev = (src[Npar]-fit[Npar]);
        fprintf (stderr, "par %d : %f vs %f : abso dev %f\n", Npar, src[Npar], fit[Npar], dev);
    } else {
        dev = (src[Npar]/fit[Npar]);
        fprintf (stderr, "par %d : %f vs %f : frac dev %f\n", Npar, src[Npar], fit[Npar], dev);
    }
    return true;
}

# if (0)
    int main (void)
{
    pmModelGroupInit ();
    pmSourceFitModelInit (15, 0.01, 1.0, true);

    plan_tests(240);

    // build a gauss-deviate vector (mean = 0.0, sigma = 1.0)
    psRandom *seed = psRandomAllocSpecific (PS_RANDOM_TAUS, 0);

    static float radius[] = {3.0, 5.0, 7.0, 10.0, 15.0, 25.0};
    static float sigma[] = {1.0, 1.5, 2.0};
    static float flux[] = {10000.0, 3000.0, 1000.0, 300.0, 100.0, 30.0, 10.0};

    for (int i = 0; i < sizeof(sigma)/sizeof(float); i++) {
        for (int j = 0; j < sizeof(radius)/sizeof(float); j++) {
            for (int k = 0; k < sizeof(flux)/sizeof(float); k++) {
                fitModels (seed, flux[k], radius[j], sigma[i]);
            }
        }
    }

    return exit_status();
}

static psVector *par1 = NULL;
static psVector *par2 = NULL;
static psVector *par3 = NULL;
static psVector *par4 = NULL;
static psVector *par5 = NULL;

bool fitModels (psRandom *seed, float flux, float radius, float sigma)
{

    psMemId id = psMemGetId();

    diag("test model fit - flux: %f, radius: %f, sigma: %f", flux, radius, sigma);

    par1 = psVectorAllocEmpty (200, PS_TYPE_F32);
    par2 = psVectorAllocEmpty (200, PS_TYPE_F32);
    par3 = psVectorAllocEmpty (200, PS_TYPE_F32);
    par4 = psVectorAllocEmpty (200, PS_TYPE_F32);
    par5 = psVectorAllocEmpty (200, PS_TYPE_F32);

    for (int i = 0; i < 200; i++) {
        fitModelFlux (seed, flux, radius, sigma);
    }

    float signal = 2*M_PI*sigma*sigma*flux;
    float noise = sqrt(signal + 4*M_PI*sigma*sigma*(100 + PS_SQR(5)));
    float dMag = noise / signal;
    float dPos = sigma * dMag;
    diag ("signal: %f, noise: %f, dMag: %f, dPos: %f", signal, noise, dMag, dPos);

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
    psVectorStats (stats, par1, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 2.0), "Io ref/fit stdev: %f : %f sigma", stats->sampleStdev, stats->sampleStdev/dMag);
    psVectorStats (stats, par2, NULL, NULL, 0);
    ok ((stats->sampleStdev/dPos < 2.0), "Xo ref/fit stdev: %f : %f sigma", stats->sampleStdev, stats->sampleStdev/dPos);
    psVectorStats (stats, par3, NULL, NULL, 0);
    ok ((stats->sampleStdev/dPos < 2.0), "Yo ref/fit stdev: %f : %f sigma", stats->sampleStdev, stats->sampleStdev/dPos);
    psVectorStats (stats, par4, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 2.0), "Sx ref/fit stdev: %f : %f sigma", stats->sampleStdev, stats->sampleStdev/dMag);
    psVectorStats (stats, par5, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 2.0), "Sy ref/fit stdev: %f : %f sigma", stats->sampleStdev, stats->sampleStdev/dMag);

    psFree (par1);
    psFree (par2);
    psFree (par3);
    psFree (par4);
    psFree (par5);
    psFree (stats);

    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return true;
}

# endif
