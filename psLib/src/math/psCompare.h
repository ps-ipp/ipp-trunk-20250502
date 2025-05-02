/* @file psCompare.h
 * @brief Comparison functions for sorting routines
 *
 * @author Robert Daniel DeSonia, MHPCC
 *
 * @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-09-28 21:02:23 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_COMPARE_H
#define PS_COMPARE_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psType.h"

/** A comparison function for sorting elements that are pointers to data,
 *  e.g., for psList of pointers to numeric values.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
typedef int (*psComparePtrFunc) (
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** A comparison function for sorting.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
typedef int (*psCompareFunc) (
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);

/** Compare function of psS8 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS8Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS16 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS16Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS32 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS64 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU8 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU8Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU16 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU16Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU32 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU64 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psF32 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareF32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psF64 data.  For use with psListSort.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareF64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS8 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS8Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS16 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS16Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS32 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS64 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU8 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU8Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU16 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU16Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU32 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or lessg than the second.
 */
int psCompareDescendingU32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psU64 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or lessg than the second.
 */
int psCompareDescendingU64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psF32 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or lessg than the second.
 */
int psCompareDescendingF32Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psF64 data.  For use with psListSort for descending ordering.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or lessg than the second.
 */
int psCompareDescendingF64Ptr(
    const void **a,                    ///< first comparison target
    const void **b                     ///< second comparison target
);

/** Compare function of psS8 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS8(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS16 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS16(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareS64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU8 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU8(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU16 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU16(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareU64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psF32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareF32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psF64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively less
 *                   than, equal to, or greater than the second.
 */
int psCompareF64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS8 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS8(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS16 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS16(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psS64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingS64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU8 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU8(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU16 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU16(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psU64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingU64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psF32 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingF32(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);


/** Compare function of psF64 data.
 *
 *  @return int      an integer less than, equal to, or greater than zero if
 *                   the first argument is considered to be respectively greater
 *                   than, equal to, or less than the second.
 */
int psCompareDescendingF64(
    const void *a,                     ///< first comparison target
    const void *b                      ///< second comparison target
);

/// @}
#endif  // #ifndef PS_COMPARE_H
