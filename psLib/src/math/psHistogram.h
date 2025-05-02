/* @file  psHistogram.h
 * @brief basic histogram functions
 *
 * This file holds the definition of the histogram data structures.  It also contains
 * prototypes for procedures which operate on those data structures.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 *
 * Copyright 2004-2005 IfA, University of Hawaii
 */

#ifndef PS_HISTOGRAM_H
#define PS_HISTOGRAM_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/******************************************************************************
    Histogram functions and data structures.
 *****************************************************************************/

/** The basic histogram structure which contains bounds and bins.
 *
 *  In this structure, the vector bounds specifies the boundaries of the
 *  histogram bins, and must of type psF32, while nums specifies the number
 *  of entries in the bin, and must of type psU32. The value of bounds.n must
 *  therefore be 1 greater than than nums.n. The two values minNum and maxNum
 *  are the number of data values which fell below the lower limit bound or
 *  above the upper limit bound, respectively.
 */
typedef struct
{
    const psVector* bounds;            ///< Bounds for the bins (type F32)
    psVector* nums;                    ///< Number in each of the bins (INT)
    int minNum;                        ///< Number below the minimum
    int maxNum;                        ///< Number above the maximum
    bool uniform;                      ///< Is it a uniform distribution?
}
psHistogram;

/** Allocator for psHistogram where the bounds of the bins are implicitly
 *  specified through simply specifying an upper and lower limit along with
 *  the size of the bins.
 *
 *  @return psHistogram*    Newly allocated psHistogram
 */
psHistogram* psHistogramAlloc(
    float lower,                       ///< Lower limit for the bins
    float upper,                       ///< Upper limit for the bins
    int n                              ///< Number of bins
) PS_ATTR_MALLOC;


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psHistogram structure, false otherwise.
 */
bool psMemCheckHistogram(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Allocator for psHistogram where the bounds of the bins are explicitly
 *  specified.
 *
 *  @return psHistogram*    Newly allocated psHistogram
 */
psHistogram* psHistogramAllocGeneric(
    const psVector* bounds             ///< Bounds for the bins
);

/** Calculate a histogram
 *
 *  The following function populates the histogram bins from the specified
 *  vector (in). It alters and returns the histogram out structure. The input
 *  vector may be of types psU8, psU16, psF32, psF64.
 *
 *  @return bool   Successful operation?
 */
bool psVectorHistogram(
    psHistogram* out,                  ///< Histogram data
    const psVector* values,            ///< Vector to analyse
    const psVector* errors,            ///< Errors
    const psVector* mask,              ///< Mask dat for input vector
    psVectorMaskType maskVal		///< Mask value
    );

/// @}
#endif // #ifndef PS_HISTOGRAM_H
