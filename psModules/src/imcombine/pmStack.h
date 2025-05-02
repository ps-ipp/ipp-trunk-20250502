/* @file  pmStack.h
 *
 * This file will perform image combination of several images of the
 * same field, produce a list of questionable pixels, then tag some
 * of those pixels as defects.
 *
 * @author Paul Price, IfA
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-13 23:52:14 $
 *
 * Copyright 2004-2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_STACK_H
#define PM_STACK_H

#include <pslib.h>
#include <pmHDU.h>
#include <pmFPA.h>

/// @addtogroup imcombine Image Combinations
/// @{


/// Container for input image
typedef struct {
    pmReadout *readout;                 ///< Warped readout (sky cell)
    psPixels *reject;                   ///< Pixels to reject
    psPixels *inspect;                  ///< Pixels to inspect
    float weight;                       ///< Relative weighting for image
    float exp;                          ///< Exposure time
    float addVariance;                  ///< Additional variance when rejecting
} pmStackData;

/// Constructor
pmStackData *pmStackDataAlloc(pmReadout *readout, ///< Warped readout (sky cell)
                              float weight, ///< Weight to apply
                              float exp,    ///< Exposure time
                              float addVariance ///< Additional variance when rejecting
    );
/// Stack input images simply

bool pmStackSimpleMedianCombine(pmReadout *combined, ///< Combined readout (output)
				psArray *input       ///< Input array of pmStackData
				);

bool pmStackCombineByPercentile(
    pmReadout *combined,
    pmReadout *expmaps,
    psArray *input,
    psF64 rejectFraction,
    int nminpix,		     ///< Minimum number input per pixel to combine
    psImageMaskType badMaskBits, 	// treat these bits as 'bad'
    psImageMaskType suspectMaskBits,	// treat these bits as 'suspect'
    psImageMaskType blankMaskBits       // use this mask value for pixels missing input data (distinguish between Ninput = 0 and Ngood = 0?)
  );

/// Stack input images
bool pmStackCombine(pmReadout *combined,///< Combined readout (output)
                    pmReadout *expmaps, ///< Exposure maps (output)
                    psArray *input,     ///< Input array of pmStackData
                    psImageMaskType maskVal, ///< Mask value of bad pixels
                    psImageMaskType suspect, ///< Mask value of suspect pixels
                    psImageMaskType bad,     ///< Mask value to give rejected pixels
                    int kernelSize,     ///< Half-size of the convolution kernel
                    float iter,         ///< Number of iterations per input
                    float rej,          ///< Rejection limit (standard deviations)
                    float sys,          ///< Relative systematic error
                    float discard,      ///< Fraction of values to discard for Olympic weighted mean
                    bool useVariance,   ///< Use variance values for rejection?
                    bool safe,          ///< Play safe with small numbers of input pixels (mask if N <= 2)?
		    int nminpix,        ///< Minimum number input per pixel to combine
                    bool rejectInspect  ///< Reject pixels instead of marking them for inspection?
    );

/// @}
#endif
