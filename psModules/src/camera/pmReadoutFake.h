#ifndef PM_READOUT_FAKE_H
#define PM_READOUT_FAKE_H

// #include <pslib.h>
// #include <pmHDU.h>
// #include <pmFPA.h>
// 
// #include <pmMoments.h>
// #include <pmResiduals.h>
// #include <pmGrowthCurve.h>
// #include <pmTrend2D.h>
// #include <pmPSF.h>
// #include <pmSourceMasks.h>

/// Set threading
///
/// Returns old threading state
bool pmReadoutFakeThreads(
    bool new                            // New threading state
    );

/// Generate a fake readout from vectors
bool pmReadoutFakeFromVectors(pmReadout *readout, ///< Output readout
                              int numCols, int numRows, ///< Dimension of image
                              const psVector *x, const psVector *y, ///< Source coordinates
                              const psVector *mag, ///< Source magnitudes
                              const psVector *xOffset, ///< x offsets for sources (source -> img), or NULL
                              const psVector *yOffset, ///< y offsets for sources (source -> img), or NULL
                              const pmPSF *psf, ///< PSF for sources
                              float minFlux, ///< Minimum flux to bother about; for setting source radius
                              int radius, ///< Fixed radius for sources
                              bool circularise, ///< Circularise PSF model?
                              bool normalisePeak ///< Normalise the peak value?
    );

/// Generate a fake readout from an array of sources
bool pmReadoutFakeFromSources(pmReadout *readout, ///< Output readout
                              int numCols, int numRows, ///< Dimension of image
                              const psArray *sources, ///< Array of pmSource
                              pmSourceMode sourceMask, ///< Mask for sources
                              const psVector *xOffset, ///< x offsets for sources (source -> img), or NULL
                              const psVector *yOffset, ///< y offsets for sources (source -> img), or NULL
                              const pmPSF *psf, ///< PSF for sources
                              float minFlux, ///< Minimum flux to bother about; for setting source radius
                              int radius, ///< Fixed radius for sources
                              bool circularise, ///< Circularise PSF model?
                              bool normalise ///< Normalise the peak value?
    );

#endif
