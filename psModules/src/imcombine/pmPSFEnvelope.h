#ifndef PM_PSF_ENVELOPE_H
#define PM_PSF_ENVELOPE_H

#include <pslib.h>
#include <pmMoments.h>
#include <pmResiduals.h>
#include <pmGrowthCurve.h>
#include <pmTrend2D.h>
#include <pmPSF.h>

/// Generate a PSF which is the envelope of an array of PSFs
///
/// Generates multiple instances of the PSFs (distributed over an image)
pmPSF *pmPSFEnvelope(int numCols, int numRows, // Size of original image
                     const psArray *inputs, // Input PSF models
                     int instances,     // Number of instances per dimension
                     int radius,        // Radius of each PSF
                     const char *modelName, // Name of PSF model to use
                     int xOrder, int yOrder, // Order for PSF variation
		     psImageMaskType maskVal
    );

#endif
