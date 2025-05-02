#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <pslib.h>

#include "pmFlatNormalize.h"

// XXX this function should take the abstract mask names and set bad values in a more precise way

// I'm not sure that many many iterations are required, but rather suspect that the system converges within a
// few with absolutely no trouble (it *is* over-constrained).  For this reason, I'm putting the maximum number
// of iterations and tolerance as preset values.
#define MAXITER 10                      // Maximum number of iterations
#define TOLERANCE 1e-3                  // Minimum tolerance for convergance


bool pmFlatNormalize(psVector **expFluxesPtr, psVector **chipGainsPtr, const psImage *bgMatrix)
{
    PS_ASSERT_PTR_NON_NULL(bgMatrix, false);
    PS_ASSERT_IMAGE_NON_NULL(bgMatrix, false);

    int numExps = bgMatrix->numRows; // Number of exposures
    int numChips = bgMatrix->numCols; // Number of chips with which each exposure is made

    psVector *expFluxes;                // Dereferenced version of expFluxesPtr
    if (expFluxesPtr) {
        if (*expFluxesPtr) {
            PS_ASSERT_VECTOR_TYPE(*expFluxesPtr, PS_TYPE_F32, false);
            PS_ASSERT_VECTOR_SIZE(*expFluxesPtr, (long)numExps, false);
        } else {
            *expFluxesPtr = psVectorAlloc(numExps, PS_TYPE_F32);
        }
        expFluxes = psMemIncrRefCounter(*expFluxesPtr);
    } else {
        expFluxes = psVectorAlloc(numExps, PS_TYPE_F32);
    }

    psVector *chipGains;                // Dereferenced version of chipGainsPtr
    if (chipGainsPtr) {
        if (*chipGainsPtr) {
            PS_ASSERT_VECTOR_TYPE(*chipGainsPtr, PS_TYPE_F32, false);
            PS_ASSERT_VECTOR_SIZE(*chipGainsPtr, (long)numChips, false);
        } else {
            *chipGainsPtr = psVectorAlloc(numChips, PS_TYPE_F32);
            psVectorInit(*chipGainsPtr, 1.0);
        }
        chipGains = psMemIncrRefCounter(*chipGainsPtr);
    } else {
        chipGains = psVectorAlloc(numChips, PS_TYPE_F32);
        psVectorInit(chipGains, 1.0);
    }

    // Take the logarithms
    psImage *flux = psImageCopy(NULL, bgMatrix, PS_TYPE_F32); // Copy of the input flux levels matrix
    psImage *fluxMask = psImageAlloc(numChips, numExps, PS_TYPE_IMAGE_MASK); // Mask for bad measurements
    psImageInit(fluxMask, 0);
    psVector *gainMask = psVectorAlloc(numChips, PS_TYPE_VECTOR_MASK); // Mask for bad gains
    psVectorInit(gainMask, 0);
    psVector *expMask = psVectorAlloc(numExps, PS_TYPE_VECTOR_MASK); // Mask for bad exposures
    psVectorInit(expMask, 0);
    for (int i = 0; i < numChips; i++) {
        // Note: the input gains are in e/ADU; we want to work with ADU/e (bg [ADU] = g [ADU/e] * f [e])
        // Hence the minus sign
        if (isfinite(chipGains->data.F32[i]) && chipGains->data.F32[i] > 0) {
            chipGains->data.F32[i] = -logf(chipGains->data.F32[i]);
        } else {
            chipGains->data.F32[i] = 0.0; // Take a wild guess, gain ~ 1 e/ADU
        }

        for (int j = 0; j < numExps; j++) {
            if (isfinite(flux->data.F32[j][i]) && flux->data.F32[j][i] > 0) {
                flux->data.F32[j][i] = logf(flux->data.F32[j][i]);
            } else {
                // Blank out this measurement
                fluxMask->data.PS_TYPE_IMAGE_MASK_DATA[j][i] = 1;
                flux->data.F32[j][i] = NAN;
            }
        }
    }

    // Not really sure that we need to iterate, but here we go anyway...

    float diff = INFINITY;              // Difference from previous iteration
    psVector *oldExpFluxes = NULL;      // The fluxes in the previous iteration
    psVector *oldChipGains = NULL;      // Chip gains in the previous iteration
    for (int iter = 0; iter < MAXITER && diff > TOLERANCE; iter++) {
        // Improve on the exposure fluxes
        int numFluxes = 0;              // Number of fluxes
        for (int i = 0; i < numExps; i++) {
            if (expMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                psTrace("psModules.detrend", 7, "Flux for exposure %d is masked.\n", i);
                continue;
            }
            numFluxes++;
            float sum = 0.0;            // Sum of F_ij - G_j
            int number = 0;             // Number of chips contributing
            for (int j = 0; j < numChips; j++) {
                if (!gainMask->data.PS_TYPE_VECTOR_MASK_DATA[j] && !fluxMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
                    sum += flux->data.F32[i][j] - chipGains->data.F32[j];
                    number++;
                }
            }
            if (number > 0) {
                expFluxes->data.F32[i] = sum / (float)number;
            } else {
                expMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
                expFluxes->data.F32[i] = NAN;
            }
            psTrace("psModules.detrend", 7, "Flux for exposure %d is %lf\n", i, expf(expFluxes->data.F32[i]));
        }

        // Improve on the gains
        float meanGain = 0.0;           // Mean gain
        int numGains = 0;               // Number of gains
        for (int i = 0; i < numChips; i++) {
            if (gainMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                continue;
            }
            float sum = 0.0;           // Sum of F_ji - S_j
            int number = 0;             // Numer of sources contributing
            for (int j = 0; j < numExps; j++) {
                if (!fluxMask->data.PS_TYPE_IMAGE_MASK_DATA[j][i]) {
                    sum += flux->data.F32[j][i] - expFluxes->data.F32[j];
                    number++;
                }
            }
            if (number > 0) {
                chipGains->data.F32[i] = sum / (float)number;
            } else {
                gainMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
                chipGains->data.F32[i] = NAN;
            }
            psTrace("psModules.detrend", 7, "Gain for chip %d is %lf\n", i, expf(-chipGains->data.F32[i]));
            meanGain += expf(chipGains->data.F32[i]);
            numGains++;
        }

        // Normalise the mean gain to unity, and measure the difference
        meanGain /= (float)numGains;
        meanGain = logf(meanGain);
        if (iter > 0) {
            diff = 0.0;
            for (int i = 0; i < numChips; i++) {
                if (gainMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                    continue;
                }
                chipGains->data.F32[i] -= meanGain;
                diff += abs((chipGains->data.F32[i] - oldChipGains->data.F32[i]) / chipGains->data.F32[i]);
            }
            for (int i = 0; i < numExps; i++) {
                if (expMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                    continue;
                }
                diff += abs((expFluxes->data.F32[i] - oldExpFluxes->data.F32[i]) / expFluxes->data.F32[i]);
            }
        }

        psTrace("psModules.detrend", 2, "Iteration %d: difference is %e\n", iter, diff);

        // Copy the new to the old
        oldChipGains = psVectorCopy(oldChipGains, chipGains, PS_TYPE_F32);
        oldExpFluxes = psVectorCopy(oldExpFluxes, expFluxes, PS_TYPE_F32);
    }
    psFree(flux);
    psFree(fluxMask);
    psFree(oldChipGains);
    psFree(oldExpFluxes);

    // Un-log the vectors
    for (int i = 0; i < numChips; i++) {
        if (!gainMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            chipGains->data.F32[i] = expf(chipGains->data.F32[i]);
        }
    }
    for (int i = 0; i < numExps; i++) {
        if (!expMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            expFluxes->data.F32[i] = expf(expFluxes->data.F32[i]);
        }
    }
    psFree(gainMask);
    psFree(expMask);

    psFree(chipGains);
    psFree(expFluxes);

    return (diff < TOLERANCE); // Did we converge?
}
