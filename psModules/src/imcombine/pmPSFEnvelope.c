#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmReadoutFake.h"
#include "pmPSFEnvelope.h"
#include "pmStackVisual.h"


// #define TESTING                         // Enable test output
// #define PEAK_NORM                       // Normalise peaks?
#define PEAK_FLUX 1.0e4                 // Peak flux for each source
#define SKY_VALUE 0.0e0                 // Sky value for fake image
#define VARIANCE_VAL 3.0                // Variance for image
#define VARIANCE_FACTOR 10.0            // Factor to multiply image by to get variance
#define PSF_STATS PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV // Statistics options for measuring PSF
#define SOURCE_FIT_ITERATIONS 100       // Number of iterations for source fitting
#define MODEL_MASK (PM_MODEL_STATUS_NONCONVERGE | PM_MODEL_STATUS_OFFIMAGE | \
                    PM_MODEL_STATUS_BADARGS | PM_MODEL_STATUS_LIMITS) // Mask to apply to models


// XXX To do:
//
// * PSF variation when only a portion of the image is present (e.g., the edge of an FPA overlapping a
// skycell) may mean a disastrously weird PSF in the missing regions.  To counter this, get a region of
// validity for each PSF (perhaps from an associated mask, or have the user work it out), and taper the PSF
// when outside this region (perhaps multiply the peak flux by a Gaussian whose arguments are the distance
// from the valid region, and a width that the user supplies).


// We deliberately do not include the calculation of and storing of residuals (data - model) for the PSF
// model, because (1) there is no code in psModules to do this, and we're not going to implement it here; and
// (2) this is intended to generate "nice" or "ideal" PSFs to feed into pmSubtraction (PSF matching code), so
// any residuals will hopefully be dealt with by that.


pmPSF *pmPSFEnvelope(int numCols, int numRows, // Size of original image
                     const psArray *inputs, // Input PSF models
                     int instances, // Number of instances per dimension
                     int radius,        // Radius of each PSF
                     const char *modelName,// Name of PSF model to use
                     int xOrder, int yOrder, // Order for PSF variation fit
                     psImageMaskType maskVal
                     )
{
    PS_ASSERT_INT_POSITIVE(numCols, NULL);
    PS_ASSERT_INT_POSITIVE(numRows, NULL);
    PS_ASSERT_ARRAY_NON_NULL(inputs, NULL);
    PS_ASSERT_INT_POSITIVE(instances, NULL);
    PS_ASSERT_INT_POSITIVE(radius, NULL);
    PS_ASSERT_STRING_NON_EMPTY(modelName, NULL);
    PS_ASSERT_INT_NONNEGATIVE(xOrder, NULL);
    PS_ASSERT_INT_NONNEGATIVE(yOrder, NULL);

    float xOrigSpacing = (float)(numCols - 2 * radius) / (float)(instances - 1); // Spacing between instances
    float yOrigSpacing = (float)(numRows - 2 * radius) / (float)(instances - 1); // Spacing between instances
    int fakeSpacing = 2 * radius + 1;   // Spacing between instances (x and y) in the fake image
    int fakeSize = instances * fakeSpacing; // Size of fake image

    // Generate list of fake sources (instances of the PSF)
    int numFakes = PS_SQR(instances);   // Number of fake sources
    psArray *fakes = psArrayAlloc(numFakes); // Fake sources
    psVector *xOffset = psVectorAlloc(numFakes, PS_TYPE_S32); // X offset from fake position to image
    psVector *yOffset = psVectorAlloc(numFakes, PS_TYPE_S32); // Y offset from fake position to image
    for (int j = 0, index = 0; j < instances; j++) {
        float yOrig = j * yOrigSpacing + radius; // Source position in original image
        float yFake = j * fakeSpacing + radius; // Position in fake image
        int dy = yFake - yOrig;         // Difference between fake and original position

        for (int i = 0; i < instances; i++, index++) {
            float xOrig = i * xOrigSpacing + radius; // Source position in original image
            float xFake = i * fakeSpacing + radius; // Position in fake image
            int dx = xFake - xOrig;     // Difference between fake and original position

            pmSource *fake = pmSourceAlloc(); // Fake source
            fake->peak = pmPeakAlloc(xFake - dx, yFake - dy, PEAK_FLUX, PM_PEAK_LONE);
            fake->type = PM_SOURCE_TYPE_STAR;
            fake->psfMag = -2.5 * log10(PEAK_FLUX);

            psTrace("psModules.imcombine", 5, "Source %d: %.2f,%.2f\n",
                    index, xOrig, yOrig);

            fakes->data[index] = fake;
            xOffset->data.S32[index] = dx;
            yOffset->data.S32[index] = dy;
        }
    }

    // Generate fake images with each PSF, and take the envelope
    psImage *envelope = psImageAlloc(fakeSize, fakeSize, PS_TYPE_F32); // Image with envelope of PSFs
    psImageInit(envelope, SKY_VALUE);
    pmReadout *fakeRO = pmReadoutAlloc(NULL); // Fake readout
    float maxRadius = 0.0;              // Maximum radius for sources
    psVector *numbers = psVectorAlloc(numFakes, PS_TYPE_S32); // Number of detections for each source
    psVectorInit(numbers, 0);
    for (int i = 0; i < inputs->n; i++) {
        pmPSF *psf = inputs->data[i];   // PSF of interest
        if (!psf) {
            continue;
        }

        if (psTraceGetLevel("psModules.imcombine") >= 1) {
            psString string = NULL;     // String with values
            psStringAppend(&string, "PSF %d: ", i);
            float x = numCols / 2.0, y = numRows / 2.0; // Coordinates of interest
            for (int j = 4; j < psf->params->n; j++) {
                pmTrend2D *trend = psf->params->data[j]; // Trend of interest
                double val = pmTrend2DEval(trend, x, y);
                double err;
                switch (trend->mode) {
                  case PM_TREND_POLY_ORD:
                  case PM_TREND_POLY_CHEB:
                    err = NAN;
                    break;
                  case PM_TREND_MAP:
                    err = psImageUnbinPixel(x, y, trend->map->error, trend->map->binning);
                    break;
                  default:
                    psAbort("Unsupported mode: %x", trend->mode);
                }
                psStringAppend(&string, "%lf %lf   ", val, err);
            }
            psTrace("psModules.imcombine", 1, "%s\n", string);
            psFree(string);
        }

        // Test PSF
        {
            bool goodPSF = false;       // Is there a PSF that we can use?
            int xNum = PS_MAX(psf->trendNx, 1), yNum = PS_MAX(psf->trendNy, 1); // Number of positions to check
            for (int j = 0; j < yNum && !goodPSF; j++) {
                float y = ((float)j + 0.5) / (float)yNum * numRows; // Position on image
                for (int i = 0; i < xNum && !goodPSF; i++) {
                    float x = ((float)i + 0.5) / (float)xNum * numCols; // Position on image
                    pmModelClassSetLimits(PM_MODEL_LIMITS_IGNORE);
                    pmModel *model = pmModelFromPSFforXY(psf, x, y, PEAK_FLUX); // Test model
                    if (!model) {
                        continue;
                    }
                    model->class->modelSetLimits(PM_MODEL_LIMITS_MODERATE);
                    bool limits = true; // Model within limits?
                    for (int j = 0; j < model->params->n && limits; j++) {
                        if (!model->class->modelLimits(PS_MINIMIZE_PARAM_MIN, j, model->params->data.F32, NULL) ||
                            !model->class->modelLimits(PS_MINIMIZE_PARAM_MAX, j, model->params->data.F32, NULL)) {
                            limits = false;
                        }
                    }
                    psFree(model);
                    if (limits) {
                        goodPSF = true;
                    }
                }
            }
            if (!goodPSF) {
                psWarning("PSF %d is completely bad --- not including in envelope calculation.", i);
                continue;
            }
        }

        pmResiduals *resid = psf->residuals;// PSF residuals
        psf->residuals = NULL;
        pmModelClassSetLimits(PM_MODEL_LIMITS_MODERATE);
	psLogMsg("psModules",PS_LOG_INFO,"Matching Input %d",i);
#define CIRCULARIZE true
        if (!pmReadoutFakeFromSources(fakeRO, fakeSize, fakeSize, fakes, 0, xOffset, yOffset, psf,
                                      NAN, radius, CIRCULARIZE, false)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to generate fake readout.");
            psFree(envelope);
            psFree(yOffset);
            psFree(xOffset);
            psFree(fakes);
            psFree(numbers);
            psf->residuals = resid;
            return NULL;
        }
        psf->residuals = resid;

        // Need to renormalise sources so they all have the same peak.  You would think they do have the same
        // peak already, but it seems that the residual map messes things up by adding extra flux
        for (int j = 0; j < numFakes; j++) {
            pmSource *source = fakes->data[j]; // Fake source
            float x = source->peak->xf + xOffset->data.S32[j]; // x coordinate of source
            float y = source->peak->yf + yOffset->data.S32[j]; // y coordinate of source

#ifdef PEAK_NORM
            // Perhaps I'm being paranoid, but specify a range to check
            int uMax = PS_MIN(x + radius, numCols - 1), uMin = PS_MAX(x - radius, 0);
            int vMax = PS_MIN(y + radius, numRows - 1), vMin = PS_MAX(y - radius, 0);

            double flux = -INFINITY;    // Peak flux
            for (int v = vMin; v <= vMax; v++) {
                for (int u = uMin; u <= uMax; u++) {
                    if (fakeRO->image->data.F32[v][u] > flux) {
                        flux = fakeRO->image->data.F32[v][u];
                    }
                }
            }
            if (!isfinite(flux) || flux < 0) {
                continue;
            }
            float norm = PEAK_FLUX / flux; // Normalisation for source
#endif
            psRegion region = psRegionSet(x - radius, x + radius, y - radius, y + radius); // PSF region
            psImage *subImage = psImageSubset(fakeRO->image, region); // Subimage of fake PSF
            psImage *subEnv = psImageSubset(envelope, region); // Subimage of envelope
#ifdef PEAK_NORM
            psBinaryOp(subImage, subImage, "*", psScalarAlloc(norm, PS_TYPE_F32));
#endif
            psBinaryOp(subEnv, subEnv, "MAX", subImage);
            psFree(subImage);
            psFree(subEnv);

            // Get the radius
            pmModel *model = pmModelFromPSFforXY(psf, source->peak->xf, source->peak->yf, PEAK_FLUX); // Model for source
            if (!model || (model->flags & MODEL_MASK)) {
                continue;
            }
            float srcRadius = model->class->modelRadius(model->params, PS_SQR(VARIANCE_VAL)); // Radius for source
            psFree(model);
            if (srcRadius == 0) {
                continue;
            }
            if (srcRadius > maxRadius) {
                maxRadius = srcRadius;
            }

            // If we got this far, the source is decent
            numbers->data.S32[j]++;
        }

#ifdef TESTING
        {
            // Write out the PSF field
            psString name = NULL;
            psStringAppend(&name, "psf_field_%03d.fits", i);
            psFits *fits = psFitsOpen(name, "w");
            pmStackVisualPlotTestImage(fakeRO->image, name);
            psFitsWriteImage(fits, NULL, fakeRO->image, 0, NULL);
            psFitsClose(fits);
            psFree(name);
        }
#endif

    }
    psFree(fakeRO);

#ifdef TESTING
    {
        // Write out the envelope
        psFits *fits = psFitsOpen("psf_field_envelope.fits", "w");
        pmStackVisualPlotTestImage(envelope, "psf_field_envelope.fits");
        psFitsWriteImage(fits, NULL, envelope, 0, NULL);
        psFitsClose(fits);
    }
#endif

    // Put the fake sources onto a full-size image
    psArray *goodFakes = psArrayAllocEmpty(numFakes); // Good fake sources
    pmReadout *readout = pmReadoutAlloc(NULL); // Readout to contain envelope pixels
    readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImageInit(readout->image, 0.0);
    for (int i = 0; i < numFakes; i++) {
        pmSource *source = fakes->data[i]; // Fake source
        if (numbers->data.S32[i] > 0) {
            psArrayAdd(goodFakes, goodFakes->n, source);
        }

        // Position of source on fake image
        int xFake = source->peak->x + xOffset->data.S32[i];
        int yFake = source->peak->y + yOffset->data.S32[i];
        psRegion region = psRegionSet(xFake - radius, xFake + radius,
                                      yFake - radius, yFake + radius); // PSF region
        psImage *subImage = psImageSubset(envelope, region); // Subimage of fake PSF

        // Position of source on "real" image
        int x0 = source->peak->x - radius;
        int y0 = source->peak->y - radius;

        if (!psImageOverlaySection(readout->image, subImage, x0, y0, "=")) {
            psError(PS_ERR_UNKNOWN, false, "Unable to overlay PSF");
            psFree(subImage);
            psFree(readout);
            psFree(xOffset);
            psFree(yOffset);
            psFree(fakes);
            psFree(numbers);
            return NULL;
        }
        psFree(subImage);
    }
    psFree(xOffset);
    psFree(yOffset);
    psFree(envelope);
    psFree(numbers);

    psFree(fakes);
    fakes = goodFakes;
    numFakes = fakes->n;

    if (numFakes == 0) {
        psError(PS_ERR_UNKNOWN, false, "No fake sources are suitable for PSF fitting.");
        psFree(fakes);
        psFree(readout);
        return false;
    }

    // XXX Setting the variance seems to be an art
    // Can't set it too high so that pixels are rejected as insignificant
    // Can't set it too low so that it's hard to get to the minimum
    // Have also tried:
    // *** readout->variance = (psImage*)psBinaryOp(NULL, readout->image, "*", readout->image);
    // *** readout->variance = (psImage*)psBinaryOp(NULL, readout->image, "*", psScalarAlloc(VARIANCE_FACTOR, PS_TYPE_F32));
    readout->variance = (psImage*)psBinaryOp(NULL, readout->image, "+",
                                             psScalarAlloc(VARIANCE_VAL, PS_TYPE_F32));
    readout->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    psImageInit(readout->mask, 0);

    if (maxRadius > radius) {
        maxRadius = radius;
    }

#ifdef TESTING
    {
        // Write out the envelope
        psFits *fits = psFitsOpen("psf_field_full.fits", "w");
        pmStackVisualPlotTestImage(readout->image, "psf_field_full.fits");
        psFitsWriteImage(fits, NULL, readout->image, 0, NULL);
        psFitsClose(fits);
    }
#endif

    // Reset the sources to point to the new pixels, and measure the moments in preparation for PSF fitting
    int numMoments = 0;                 // Number of moments measured
    for (int i = 0; i < numFakes; i++) {
        pmSource *source = fakes->data[i]; // Fake source
        float x = source->peak->xf;     // x coordinates of source
        float y = source->peak->yf;     // y coordinates of source

        psFree(source->pixels);
        psFree(source->variance);
        psFree(source->maskView);
        psFree(source->maskObj);
        source->pixels = NULL;
        source->variance = NULL;
        source->maskView = NULL;
        source->maskObj = NULL;

        if (!pmSourceDefinePixels(source, readout, x, y, radius)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to define pixels for source.");
            psFree(readout);
            psFree(fakes);
            return NULL;
        }

        // measure the source moments: tophat windowing, no pixel S/N cutoff
        if (!pmSourceMoments(source, maxRadius, 0.25*maxRadius, 0.0, 0.0, maskVal)) {
            // Can't do anything about it; limp along as best we can
            psErrorClear();
            continue;
        }
        numMoments++;
    }

    if (numMoments == 0) {
        psError(PS_ERR_UNKNOWN, true, "Unable to measure moments for sources.");
        psFree(fakes);
        psFree(readout);
        return NULL;
    }

    // Don't assume Poisson errors
    pmPSFOptions *options = pmPSFOptionsAlloc(); // Options for fitting a PSF
    options->poissonErrorsPhotLMM = true;
    options->poissonErrorsPhotLin = false;
    options->poissonErrorsParams = true;
    options->stats = psStatsAlloc(PSF_STATS);
    options->fitRadius = maxRadius;
    options->apRadius = maxRadius; // XXX need to decide if aperture mags need a different radius
    options->psfTrendMode = PM_TREND_MAP;
    options->psfTrendNx = xOrder;
    options->psfTrendNy = yOrder;
    options->psfFieldNx = numCols;
    options->psfFieldNy = numRows;
    options->psfFieldXo = 0;
    options->psfFieldYo = 0;
    options->chiFluxTrend = false;      // All sources have similar flux, so fitting a trend often fails

    // options which modify the behavior of the model fitting
    options->fitOptions                = pmSourceFitOptionsAlloc();
    options->fitOptions->nIter         = SOURCE_FIT_ITERATIONS;
    options->fitOptions->minTol        = 0.01;
    options->fitOptions->maxTol        = 1.00;
    options->fitOptions->poissonErrors = true;
    options->fitOptions->weight        = VARIANCE_VAL;
    options->fitOptions->mode          = PM_SOURCE_FIT_PSF;

    pmModelClassSetLimits(PM_MODEL_LIMITS_STRICT); // Important for getting a good stack target PSF

    pmPSFtry *try = pmPSFtryModel(fakes, modelName, options, 0, 0xff);
    psFree(options);
    if (!try) {
        psError(PS_ERR_UNKNOWN, false, "Unable to fit PSF model to PSF envelope.");
        psFree(readout);
        psFree(fakes);
        return NULL;
    }

    pmPSF *psf = psMemIncrRefCounter(try->psf); // Output PSF
    psFree(try);

    if (psTraceGetLevel("psModules.imcombine") >= 1) {
        psString string = NULL;     // String with values
        psStringAppend(&string, "Envelope PSF: ");
        float x = numCols / 2.0, y = numRows / 2.0; // Coordinates of interest
        for (int j = 4; j < psf->params->n; j++) {
            pmTrend2D *trend = psf->params->data[j]; // Trend of interest
            double val = pmTrend2DEval(trend, x, y);
            double err;
            switch (trend->mode) {
              case PM_TREND_POLY_ORD:
              case PM_TREND_POLY_CHEB:
                err = NAN;
                break;
              case PM_TREND_MAP:
                err = psImageUnbinPixel(x, y, trend->map->error, trend->map->binning);
                break;
              default:
                psAbort("Unsupported mode: %x", trend->mode);
            }
            psStringAppend(&string, "%lf %lf   ", val, err);
        }
        psTrace("psModules.imcombine", 1, "%s\n", string);
        psFree(string);
    }

#ifdef TESTING
    {
        // Need to translate peak flux --> integrated flux
        pmModel *fakeModel = pmModelFromPSFforXY(psf, (float)numCols / 2.0, (float)numRows / 2.0,
                                                 1.0); // Fake model, with central intensity of 1.0
        psAssert (fakeModel, "failed to generate model: should this be an error or not?");
        float flux0 = fakeModel->modelFlux(fakeModel->params); // Flux for central intensity of 1.0
        for (int i = 0; i < numFakes; i++) {
            pmSource *source = fakes->data[i]; // Fake source
            source->psfMag -= 2.5 * log10(flux0);
        }

        pmReadout *generated = pmReadoutAlloc(NULL); // Generated image
        pmReadoutFakeFromSources(generated, numCols, numRows, fakes, 0, NULL, NULL, psf, NAN, radius,
                                 false, true);
        {
            psFits *fits = psFitsOpen("psf_field_model.fits", "w");
            pmStackVisualPlotTestImage(generated->image, "psf_field_model.fits");
            psFitsWriteImage(fits, NULL, generated->image, 0, NULL);
            psFitsClose(fits);
        }
        psBinaryOp(generated->image, generated->image, "-", readout->image);
        {
            psFits *fits = psFitsOpen("psf_field_resid.fits", "w");
            pmStackVisualPlotTestImage(generated->image, "psf_field_resid.fits");
            psFitsWriteImage(fits, NULL, generated->image, 0, NULL);
            psFitsClose(fits);
        }
        psFree(generated);
    }
#endif

    psFree(fakes);
    psFree(readout);

    return psf;
}
