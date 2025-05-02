#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
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

#include "pmSourceGroups.h"
#include "pmReadoutFake.h"

// XXX this is now hard-wired in pmModelParamsToAxes
// #define MAX_AXIS_RATIO 20.0             // Maximum axis ratio for PSF model

#define MODEL_MASK (PM_MODEL_STATUS_NONCONVERGE | PM_MODEL_STATUS_OFFIMAGE | \
                    PM_MODEL_STATUS_BADARGS | PM_MODEL_STATUS_LIMITS) // Mask to apply to models


static bool threaded = false;           // Running threaded?




// Given an object model, circularise it by setting the axes to be identical
static bool circulariseModel(pmModel *model // Model to circularise
    )
{
    assert(model);

    psF32 *params = model->params->data.F32; // Model parameters
    psEllipseAxes axes = pmPSF_ModelToAxes(params, model->class->useReff); // Ellipse axes
    // Curiously, the minor axis can be larger than the major axis, so need to check.
    if (axes.major >= axes.minor) {
        axes.minor = axes.major;
    } else {
        axes.major = axes.minor;
    }
    return pmPSF_AxesToModel(params, axes, model->class->useReff);
}

/// Generate fake sources on a readout
static bool readoutFake(pmReadout *readout, // Readout of interest
                        const pmSourceGroups *groups, // Source groups
                        const psVector *x,        // x coordinates
                        const psVector *y,        // y coordinates
                        const psVector *mag,      // Magnitudes
                        const psVector *xOffset,  // Offsets in x
                        const psVector *yOffset,  // Offsets in y
                        const pmPSF *psf,         // PSF
                        float minFlux,            // Minimum flux
                        float radius,             // Minimum radius
                        bool circularise,         // Circularise PSF?
                        bool normalisePeak,       // Normalise sources for peak?
                        int groupIndex,           // Group index
                        int cellIndex             // Cell index
                        )
{
    psArray *cells = groups->groups->data[groupIndex]; // Cells in group
    psVector *cellSources = cells->data[cellIndex];    // Sources in cell

    for (int i = 0; i < cellSources->n; i++) {
        int index = cellSources->data.S32[i];                       // Index for source of interest
        float flux = powf(10.0, -0.4 * mag->data.F32[index]);       // Flux of source
        float xSrc = x->data.F32[index], ySrc = y->data.F32[index]; // Coordinates of source

        if (normalisePeak) {
            // Normalise flux
            pmModel *normModel = pmModelFromPSFforXY(psf, xSrc, ySrc, 1.0); // Model for normalisation
            if (!normModel || (normModel->flags & MODEL_MASK)) {
                psFree(normModel);
                continue;
            }
            // check that all params are valid:
            bool validParams = true;
            for (int j = 0; validParams && (j < normModel->params->n); j++) {
                switch (j) {
                  case PM_PAR_SKY:
                  case PM_PAR_I0:
                  case PM_PAR_XPOS:
                  case PM_PAR_YPOS:
                    continue;
                  default:
                    if (!isfinite(normModel->params->data.F32[j])) {
                        validParams = false;
                    }
                }
            }
            if (!validParams) {
                psFree(normModel);
                continue;
            }
            if (circularise && !circulariseModel(normModel)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to circularise PSF model.");
                psFree(normModel);
                return false;
            }

            flux /= normModel->class->modelFlux(normModel->params);
            psFree(normModel);
        }

        pmModel *fakeModel = pmModelFromPSFforXY(psf, xSrc, ySrc, flux);
        if (!fakeModel || (fakeModel->flags & MODEL_MASK)) {
            psFree(fakeModel);
            continue;
        }
        // check that all params are valid:
        bool validParams = true;
        for (int j = 0; validParams && (j < fakeModel->params->n); j++) {
            switch (j) {
              case PM_PAR_SKY:
              case PM_PAR_I0:
              case PM_PAR_XPOS:
              case PM_PAR_YPOS:
                continue;
              default:
                if (!isfinite(fakeModel->params->data.F32[j])) {
                    validParams = false;
                }
            }
        }
        if (!validParams) {
            psFree(fakeModel);
            continue;
        }
        if (circularise && !circulariseModel(fakeModel)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to circularise PSF model.");
            psFree(fakeModel);
            return false;
        }
	
        psTrace("psModules.camera", 10, "Adding source at %f,%f with flux %f\n",
                fakeModel->params->data.F32[PM_PAR_XPOS], fakeModel->params->data.F32[PM_PAR_YPOS],
                fakeModel->params->data.F32[PM_PAR_I0]);

        pmSource *fakeSource = pmSourceAlloc(); // Fake source to generate
        fakeSource->peak = pmPeakAlloc(xSrc, ySrc, fakeModel->params->data.F32[PM_PAR_I0], PM_PEAK_LONE);
        float fakeRadius = 1.0;         // Radius of fake source
        if (isfinite(minFlux)) {
            fakeRadius = PS_MAX(fakeRadius, fakeModel->class->modelRadius(fakeModel->params, minFlux));
        }
        if (radius > 0) {
            fakeRadius = PS_MAX(fakeRadius, radius);
        }

        if (xOffset) {
            if (!pmSourceDefinePixels(fakeSource, readout, xSrc + xOffset->data.S32[index],
                                      ySrc + yOffset->data.S32[index], fakeRadius)) {
                psErrorClear();
                continue;
            }
            if (!pmModelAddWithOffset(fakeSource->pixels, NULL, fakeModel, PM_MODEL_OP_FULL, 0,
                                      xOffset->data.S32[index], yOffset->data.S32[index])) {
                psErrorClear();
                continue;
            }
        } else {
            if (!pmSourceDefinePixels(fakeSource, readout, xSrc, ySrc, fakeRadius)) {
                psErrorClear();
                continue;
            }
            if (!pmModelAdd(fakeSource->pixels, NULL, fakeModel, PM_MODEL_OP_FULL, 0)) {
                psErrorClear();
                continue;
            }
        }
        psFree(fakeSource);
        psFree(fakeModel);
    }

    return true;
}

/// Thread job for readoutFake()
static bool readoutFakeThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments

    pmReadout *readout = args->data[0];     // Readout of interest
    const pmSourceGroups *groups = args->data[1]; // Source groups
    const psVector *x = args->data[2];        // x coordinates
    const psVector *y = args->data[3];        // y coordinates
    const psVector *mag = args->data[4];      // Magnitudes
    const psVector *xOffset = args->data[5];  // Offsets in x
    const psVector *yOffset = args->data[6];  // Offsets in y
    const pmPSF *psf = args->data[7];         // PSF
    float minFlux = PS_SCALAR_VALUE(args->data[8], F32); // Minimum flux
    float radius = PS_SCALAR_VALUE(args->data[9], S32);  // Minimum radius - typecast to float from S32 outside of PS_SCALAR_VALUE otherwise sets 0.0 
    bool circularise = PS_SCALAR_VALUE(args->data[10], U8); // Circularise PSF?
    bool normalisePeak = PS_SCALAR_VALUE(args->data[11], U8); // Normalise for peak?
    int groupIndex = PS_SCALAR_VALUE(args->data[12], S32); // Group index
    int cellIndex = PS_SCALAR_VALUE(args->data[13], S32);  // Cell index

    return readoutFake(readout, groups, x, y, mag, xOffset, yOffset, psf, minFlux, radius, circularise,
                       normalisePeak, groupIndex, cellIndex);
}


bool pmReadoutFakeThreads(bool new)
{
    bool old = threaded;                // Old status, to return

    if (!old && new) {
        threaded = true;

        {
            psThreadTask *task = psThreadTaskAlloc("PSMODULES_READOUT_FAKE", 14);
            task->function = &readoutFakeThread;
            psThreadTaskAdd(task);
            psFree(task);
        }

    } else if (old && !new) {
        threaded = false;
        psThreadTaskRemove("PSMODULES_READOUT_FAKE");
    }

    return old;
}


bool pmReadoutFakeFromVectors(pmReadout *readout, int numCols, int numRows,
                              const psVector *x, const psVector *y, const psVector *mag,
                              const psVector *xOffset, const psVector *yOffset,
                              const pmPSF *psf, float minFlux, int radius,
                              bool circularise, bool normalisePeak)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_INT_LARGER_THAN(numCols, 0, false);
    PS_ASSERT_INT_LARGER_THAN(numRows, 0, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(y, x, false);
    PS_ASSERT_VECTOR_NON_NULL(mag, false);
    PS_ASSERT_VECTOR_TYPE(mag, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(mag, x, false);
    long numSources = x->n;              // Number of sources
    if (xOffset || yOffset) {
        PS_ASSERT_VECTOR_NON_NULL(xOffset, false);
        PS_ASSERT_VECTOR_NON_NULL(yOffset, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(xOffset, yOffset, false);
        PS_ASSERT_VECTOR_TYPE(xOffset, PS_TYPE_S32, false);
        PS_ASSERT_VECTOR_TYPE(yOffset, PS_TYPE_S32, false);
        if (xOffset->n != numSources) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Number of offset vectors (%ld) and sources (%ld) doesn't match",
                    xOffset->n, numSources);
            return false;
        }
    }
    PS_ASSERT_PTR_NON_NULL(psf, false);

    readout->image = psImageRecycle(readout->image, numCols, numRows, PS_TYPE_F32);
    psImageInit(readout->image, 0);

    int numThreads = threaded ? psThreadPoolSize() : 0; // Number of threads
    pmSourceGroups *groups = pmSourceGroupsFromVectors(readout, x, y, numThreads); // Groups of sources
    if (!groups) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate source groups");
        return false;
    }

    if (threaded) {
        for (int i = 0; i < groups->groups->n; i++) {
            psArray *cells = groups->groups->data[i]; // Cell with sources
            for (int j = 0; j < cells->n; j++) {
                psThreadJob *job = psThreadJobAlloc("PSMODULES_READOUT_FAKE");
                psArray *args = job->args;
                psArrayAdd(args, 1, readout);
                psArrayAdd(args, 1, groups);
                // Casting away const to add to array
                psArrayAdd(args, 1, (psVector*)x);
                psArrayAdd(args, 1, (psVector*)y);
                psArrayAdd(args, 1, (psVector*)mag);
                psArrayAdd(args, 1, (psVector*)xOffset);
                psArrayAdd(args, 1, (psVector*)yOffset);
                psArrayAdd(args, 1, (pmPSF*)psf);
                PS_ARRAY_ADD_SCALAR(args, minFlux, PS_TYPE_F32);
                PS_ARRAY_ADD_SCALAR(args, radius, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(args, circularise, PS_TYPE_U8);
                PS_ARRAY_ADD_SCALAR(args, normalisePeak, PS_TYPE_U8);
                PS_ARRAY_ADD_SCALAR(args, i, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(args, j, PS_TYPE_S32);

                if (!psThreadJobAddPending(job)) {
                    psFree(groups);
                    return false;
                }
            }
            if (!psThreadPoolWait(true, true)) {
                psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
                psFree(groups);
                return false;
            }
        }
    } else if (!readoutFake(readout, groups, x, y, mag, xOffset, yOffset, psf, minFlux, radius, circularise,
                            normalisePeak, 0, 0)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake sources on readout");
        psFree(groups);
        return false;
    }

    psFree(groups);

// Set a concept value
#define CONCEPT_SET_S32(CONCEPTS, NAME, OLD, NEW) { \
        psMetadataItem *item = psMetadataLookup(CONCEPTS, NAME); \
        psAssert(item->type == PS_DATA_S32, "Incorrect type: %x", item->type); \
        if (item->data.S32 == OLD) { \
            item->data.S32 = NEW; \
        } \
    }

    if (readout->parent) {
        CONCEPT_SET_S32(readout->parent->concepts, "CELL.XPARITY", 0, 1);
        CONCEPT_SET_S32(readout->parent->concepts, "CELL.YPARITY", 0, 1);
        CONCEPT_SET_S32(readout->parent->concepts, "CELL.XBIN", 0, 1);
        CONCEPT_SET_S32(readout->parent->concepts, "CELL.YBIN", 0, 1);
    }

    return true;

}


bool pmReadoutFakeFromSources(pmReadout *readout, int numCols, int numRows, const psArray *sources,
                              pmSourceMode sourceMask, const psVector *xOffset, const psVector *yOffset,
                              const pmPSF *psf, float minFlux, int radius,
                              bool circularise, bool normalisePeak)
{
    PS_ASSERT_ARRAY_NON_NULL(sources, false);

    int numSources = sources->n;          // Number of stars
    psVector *x = psVectorAllocEmpty(numSources, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty(numSources, PS_TYPE_F32);
    psVector *mag = psVectorAllocEmpty(numSources, PS_TYPE_F32);

    int numGood = 0;                    // Number of good sources
    for (int i = 0; i < numSources; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source) {
            continue;
        }
        if (source->mode & sourceMask) {
            continue;
        }
        if (!isfinite(source->psfMag)) {
            continue;
        }
        float xSrc, ySrc;                     // Coordinates of source
        if (source->modelPSF) {
            xSrc = source->modelPSF->params->data.F32[PM_PAR_XPOS];
            ySrc = source->modelPSF->params->data.F32[PM_PAR_YPOS];
        } else {
            xSrc = source->peak->xf;
            ySrc = source->peak->yf;
        }

        x->data.F32[numGood] = xSrc;
        y->data.F32[numGood] = ySrc;
        mag->data.F32[numGood] = source->psfMag;
        numGood++;
    }
    x->n = numGood;
    y->n = numGood;
    mag->n = numGood;

    bool status = pmReadoutFakeFromVectors(readout, numCols, numRows, x, y, mag, xOffset, yOffset, psf,
                                           minFlux, radius, circularise, normalisePeak);
    psFree(x);
    psFree(y);
    psFree(mag);

    return status;
}
