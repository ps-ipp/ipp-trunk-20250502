#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

# define SKY_NOISE 10.0
# define READ_NOISE 0.0

bool fitModels (psRandom *seed, float flux, float radius, float sigma);
bool fitModelFlux (psRandom *seed, float flux, float radius, float sigma);
bool fitModelFluxOne (psRandom *seed, float flux, float radius, float sigma);

// tests to check accuracy of fitted models for a range of fit radii, sigma, and flux.
// we generate a fake source, then fit the model to it.  the difference or fractional
// difference between the true and fitted parameters is saved.
// we run these tests 200 times each an examine the resulting distribution of deviations.
// the tests fail if the stdevs are more than 2x the expected stdev based on poisson noise
// ** is 2x too generous?
int main (void)
{
    pmModelClassInit ();
    // pmSourceFitModelInit (15, 0.01, 1.0, true);

    // psTraceSetLevel ("psModules.objects.pmSourceFitModel", 10);
    // psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);

    plan_tests(240);

    // build a gauss-deviate vector (mean = 0.0, sigma = 1.0)
    psRandom *seed = psRandomAllocSpecific (PS_RANDOM_TAUS, 0);

    // try a single model
    // fitModelFluxOne (seed, 10000.0, 10.0, 2.0);
    // return exit_status();

    static float radius[] = {3.0, 5.0, 7.0, 10.0, 15.0, 25.0};
    static float sigma[] = {1.0, 1.5, 2.0};
    static float flux[] = {10000.0, 3000.0, 1000.0, 300.0, 100.0, 30.0, 10.0};

    // drop-dead simple: should always work
    bool status = fitModels (seed, 10000.0, 10.0, 2.0);
    skip_start (!status, 240, "*** BASIC MODEL FITTING FAILS! *** : skipping related tests");
    // exit (1);

    for (int i = 0; i < sizeof(sigma)/sizeof(float); i++) {
        for (int j = 0; j < sizeof(radius)/sizeof(float); j++) {
            for (int k = 0; k < sizeof(flux)/sizeof(float); k++) {
                fitModels (seed, flux[k], radius[j], sigma[i]);
            }
        }
    }

    skip_end();
    return exit_status();
}

static psVector *par1 = NULL;
static psVector *par2 = NULL;
static psVector *par3 = NULL;
static psVector *par4 = NULL;
static psVector *par5 = NULL;

# define NMODELS 200
bool fitModels (psRandom *seed, float flux, float radius, float sigma)
{

    psMemId id = psMemGetId();

    diag("test model fit - flux: %f, radius: %f, sigma: %f", flux, radius, sigma);

    par1 = psVectorAllocEmpty (NMODELS, PS_TYPE_F32);
    par2 = psVectorAllocEmpty (NMODELS, PS_TYPE_F32);
    par3 = psVectorAllocEmpty (NMODELS, PS_TYPE_F32);
    par4 = psVectorAllocEmpty (NMODELS, PS_TYPE_F32);
    par5 = psVectorAllocEmpty (NMODELS, PS_TYPE_F32);

    for (int i = 0; i < NMODELS; i++) {
        fitModelFlux (seed, flux, radius, sigma);
    }

    float signal = 2*M_PI*sigma*sigma*flux;
    float noise = sqrt(signal + 4*M_PI*sigma*sigma*(SKY_NOISE*SKY_NOISE + PS_SQR(READ_NOISE)));
    float dMag = noise / signal;
    float dPos = sigma * dMag;
    diag ("signal: %f, noise: %f, dMag: %f, dPos: %f", signal, noise, dMag, dPos);

    bool status = (par1->n == NMODELS);
    ok (status, "all %d tests passed", NMODELS);

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
    psVectorStats (stats, par1, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 1.5), "Io ref/fit stdev: %e : %e sigma",  stats->sampleStdev, stats->sampleStdev/dMag);
    psVectorStats (stats, par2, NULL, NULL, 0);
    ok ((stats->sampleStdev/dPos < 1.5), "Xo ref-fit stdev: %e : %e sigma", stats->sampleStdev, stats->sampleStdev/dPos);
    psVectorStats (stats, par3, NULL, NULL, 0);
    ok ((stats->sampleStdev/dPos < 1.5), "Yo ref-fit stdev: %e : %e sigma", stats->sampleStdev, stats->sampleStdev/dPos);
    psVectorStats (stats, par4, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 1.5), "Sx ref/fit stdev: %e : %e sigma", stats->sampleStdev, stats->sampleStdev/dMag);
    psVectorStats (stats, par5, NULL, NULL, 0);
    ok ((stats->sampleStdev/dMag < 1.5), "Sy ref/fit stdev: %e : %e sigma", stats->sampleStdev, stats->sampleStdev/dMag);

    psFree (par1);
    psFree (par2);
    psFree (par3);
    psFree (par4);
    psFree (par5);
    psFree (stats);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
    return status;
}

bool fitModelFlux (psRandom *seed, float flux, float radius, float sigma)
{

  psImageMaskType maskVal = 0x01;

    psVector *rnd = psVectorAlloc (1000, PS_TYPE_F32);
    for (int i = 0; i < rnd->n; i++) {
        rnd->data.F32[i] = psRandomGaussian (seed);
    }

    // construct a model
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

    source->pixels   = psImageAlloc (100, 100, PS_TYPE_F32);
    source->variance = psImageAlloc (100, 100, PS_TYPE_F32);
    source->maskObj  = psImageAlloc (100, 100, PS_TYPE_IMAGE_MASK);
    psImageInit (source->pixels, 0.0);
    psImageInit (source->variance, 0.0);
    psImageInit (source->maskObj, 0);

    // create an image with the model, and add noise: gain is 1, subtracted sky is 100, readnoise is 5
    pmModelAdd (source->pixels, source->maskObj, source->modelEXT, PM_MODEL_OP_FULL, maskVal);
    int npix = 0;
    for (int j = 0; j < source->pixels->numRows; j++) {
        for (int i = 0; i < source->pixels->numCols; i++) {
            float flux = source->pixels->data.F32[j][i];
            float var = flux + PS_SQR(SKY_NOISE) + PS_SQR(READ_NOISE);
            source->pixels->data.F32[j][i] += rnd->data.F32[npix]*sqrt(var);
            source->variance->data.F32[j][i] = var;
            npix ++;
            if (npix == rnd->n)
                npix = 0;
        }
    }

    // psFits *fits = psFitsOpen ("test.fits", "w");
    // psFitsWriteImage (fits, NULL, source->pixels, 0, NULL);
    // psFitsClose (fits);

    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->mode          = PM_SOURCE_FIT_PSF;
    fitOptions->covarFactor   = 1.0;

    // save the original model, modify params
    pmModel *guess = pmModelCopy (source->modelEXT);
    if (fitOptions->mode == PM_SOURCE_FIT_PSF) {
      guess->params->data.F32[1] *= 0.9;
      guess->params->data.F32[2] += 1.0;
      guess->params->data.F32[3] -= 1.0;
    } 
    if (fitOptions->mode == PM_SOURCE_FIT_EXT) {
      guess->params->data.F32[1] *= 0.9;
      guess->params->data.F32[4] *= 0.9;
      guess->params->data.F32[5] *= 0.9;
    }

    // use maskVal to exclude pixels outside the circle
    psImageKeepCircle (source->maskObj, 50, 50, radius, "OR", maskVal);

    bool status = pmSourceFitModel (source, guess, fitOptions, maskVal);
    // fprintf (stderr, "Io: %8.1f %8.1f\n", source->modelEXT->params->data.F32[1], guess->params->data.F32[1]);
    if (!status) {
        psFree (rnd);
        psFree (source);
        psFree (guess);
        return false;
    }
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(maskVal));

    // par1->data.F32[par1->n] = (guess->params->data.F32[1]);
    par1->data.F32[par1->n] = (guess->params->data.F32[1] / source->modelEXT->params->data.F32[1]);
    par2->data.F32[par2->n] = (source->modelEXT->params->data.F32[2] - guess->params->data.F32[2]);
    par3->data.F32[par3->n] = (source->modelEXT->params->data.F32[3] - guess->params->data.F32[3]);
    par4->data.F32[par4->n] = (source->modelEXT->params->data.F32[4] / guess->params->data.F32[4]);
    par5->data.F32[par5->n] = (source->modelEXT->params->data.F32[5] / guess->params->data.F32[5]);

    psVectorExtend (par1, 100, 1);
    psVectorExtend (par2, 100, 1);
    psVectorExtend (par3, 100, 1);
    psVectorExtend (par4, 100, 1);
    psVectorExtend (par5, 100, 1);

    psFree (rnd);
    psFree (source);
    psFree (guess);
    psFree (fitOptions);

    return true;
}

bool fitModelFluxOne (psRandom *seed, float flux, float radius, float sigma)
{

  psImageMaskType maskVal = 0x01;

  psVector *rnd = psVectorAlloc (1000, PS_TYPE_F32);
  for (int i = 0; i < rnd->n; i++) {
    rnd->data.F32[i] = psRandomGaussian (seed);
  }

  // construct a model
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

  source->pixels   = psImageAlloc (100, 100, PS_TYPE_F32);
  source->variance = psImageAlloc (100, 100, PS_TYPE_F32);
  source->maskObj  = psImageAlloc (100, 100, PS_TYPE_IMAGE_MASK);
  psImageInit (source->pixels, 0.0);
  psImageInit (source->variance, 0.0);
  psImageInit (source->maskObj, 0);

  // create an image with the model, and add noise: gain is 1, subtracted sky is 100, readnoise is 5
  pmModelAdd (source->pixels, source->maskObj, source->modelEXT, PM_MODEL_OP_FULL, maskVal);
  int npix = 0;
  for (int j = 0; j < source->pixels->numRows; j++) {
    for (int i = 0; i < source->pixels->numCols; i++) {
      float flux = source->pixels->data.F32[j][i];
      float var = flux + 100 + PS_SQR(5);
      source->pixels->data.F32[j][i] += rnd->data.F32[npix]*sqrt(var);
      source->variance->data.F32[j][i] = var;
      npix ++;
      if (npix == rnd->n)
	npix = 0;
    }
  }

  // psFits *fits = psFitsOpen ("test.fits", "w");
  // psFitsWriteImage (fits, NULL, source->pixels, 0, NULL);
  // psFitsClose (fits);

  // EXT vs PSF fitting:
  // EXT fits Io, Sxx, Sxy, Syy
  // PSF fits Io, Xo, Yo
  pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
  fitOptions->mode = PM_SOURCE_FIT_EXT;
  // fitOptions->mode = PM_SOURCE_FIT_PSF;
  fitOptions->covarFactor   = 1.0;

  // save the original model, modify params
  pmModel *guess = pmModelCopy (source->modelEXT);
  if (fitOptions->mode == PM_SOURCE_FIT_PSF) {
    guess->params->data.F32[1] *= 0.9;
    guess->params->data.F32[2] += 1.0;
    guess->params->data.F32[3] -= 1.0;
  } 
  if (fitOptions->mode == PM_SOURCE_FIT_EXT) {
    guess->params->data.F32[1] *= 0.9;
    guess->params->data.F32[4] *= 0.9;
    guess->params->data.F32[5] *= 0.9;
  }

  // use maskVal to exclude pixels outside the circle
  psImageKeepCircle (source->maskObj, 50, 50, radius, "OR", maskVal);

  // I need to use psTrace to get verbosity from the fit...
  bool status = pmSourceFitModel (source, guess, fitOptions, maskVal);
  if (!status) {
    psFree (rnd);
    psFree (source);
    psFree (guess);
    psFree (fitOptions);
    fprintf (stderr, "failed\n");
    return false;
  }
  psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(maskVal));

  psFree (rnd);
  psFree (source);
  psFree (guess);
  psFree (fitOptions);

  return true;
}
