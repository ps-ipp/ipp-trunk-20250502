/** @file  psImageMap.c
 *
 *  @brief Functions define a 2d coarse representation of a finer 2D field
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 00:00:21 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IMAGE_MAP_H
#define PS_IMAGE_MAP_H

/// @addtogroup ImageOps Image Operations
/// @{

#include <psStats.h>
#include <psImage.h>
#include <psImageBinning.h>
#include <psVector.h>

// a structure to describe the 2D variations of some quantity as a function of position the
// variation is represented as a psImage which covers the field also represented as a psImage.
// the map image pixels are superpixels of the field image.  values in the field are determined
// by interpolating the map image.
typedef struct {
    psStats *stats;
    psImage *map;
    psImage *error;
    int col0, row0;                     // Column and row offsets from the original image
    int numCols, numRows;               // Size of original image
    psImageBinning *binning;
    int nBad;
    int nPoor;
    int nGood;
    psStatsOptions singleMean, singleStdev;  // Statistics for mean and stdev when there's a single pixel
} psImageMap;

// Assertion for psImageMap
#define PS_ASSERT_IMAGE_MAP_NON_NULL(MAP, RVAL) \
if (!(MAP) || !(MAP)->binning || !(MAP)->map || !(MAP)->error) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Image map %s or its contents is NULL.", #MAP); \
    return RVAL; \
} \
PS_ASSERT_IMAGE_NON_NULL((MAP)->map, RVAL); \
PS_ASSERT_IMAGE_NON_NULL((MAP)->error, RVAL); \
PS_ASSERT_IMAGE_TYPE((MAP)->map, PS_TYPE_F32, RVAL); \
PS_ASSERT_IMAGE_TYPE((MAP)->error, PS_TYPE_F32, RVAL); \
PS_ASSERT_IMAGES_SIZE_EQUAL((MAP)->map, (MAP)->error, RVAL);

// Assertion for statistics in psImageMap
#define PS_ASSERT_IMAGE_MAP_STATS(MAP, RVAL) \
if (!(MAP)->stats) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Image map %s statistics is unset.", #MAP); \
    return RVAL; \
}


psImageMap *psImageMapAlloc(const psImage *field, psImageBinning *binning, psStats *stats) PS_ATTR_MALLOC;
bool psMemCheckImageMap(psPtr ptr);

psImageMap *psImageMapNoImageAlloc(psImageBinning *binning, psStats *stats) PS_ATTR_MALLOC;

bool psImageMapModifyScale(psImageMap *map, int nXruff, int nYruff);

// generate a psImageMap (or NULL) with the given number of superpixels in X and Y
bool psImageMapGenerate (psImageMap *map, const psVector *x, const psVector *y, const psVector *f, const psVector *df, float badFrac);

bool psImageMapGenerateScale (psImageMap *map, const psVector *x, const psVector *y, const psVector *f, const psVector *df, float badFrac);

// apply the psImageMap to the given coordinate (fine image pixels)
double psImageMapEval (const psImageMap *map, float x, float y);

// apply the psImageMap to the given coordinate vectors (fine image pixels)
psVector *psImageMapEvalVector (const psImageMap *map, const psVector *mask, psMaskType maskValue, const psVector *x, const psVector *y);

bool psImageMapCleanup (psImageMap *map);

/// @}
#endif // #ifndef PS_IMAGE_MAP_H
