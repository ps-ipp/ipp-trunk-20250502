/* @file  pmTrend2D.h
 *
 * functions to represent ways of modeling a 2D trend
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_TREND_2D_H
# define PM_TREND_2D_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef enum {
    PM_TREND_NONE,
    PM_TREND_POLY_ORD,
    PM_TREND_POLY_CHEB,
    PM_TREND_MAP,
} pmTrend2DMode;

typedef struct {
    psPolynomial2D *poly;
    psImageMap *map;
    psStats *stats;                     // Statistics for clipped fitting
    psStatsOptions singleMean, singleStdev; // Staistics for mean and stdev when there's a single pixel
    pmTrend2DMode mode; // POLY_ORD, POLY_CHEB, MAP
} pmTrend2D;

// Assertion for pmTrend2D
#define PM_ASSERT_TREND2D_NON_NULL(TREND, RVAL) \
if (!(TREND)) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Trend %s is NULL", #TREND); \
    return RVAL; \
} \
if ((TREND)->mode == PM_TREND_MAP) { \
    PS_ASSERT_IMAGE_MAP_NON_NULL((TREND)->map, RVAL); \
} else if ((TREND)->mode == PM_TREND_POLY_ORD || (TREND)->mode == PM_TREND_POLY_CHEB) { \
    PS_ASSERT_POLY_NON_NULL((TREND)->poly, RVAL); \
} else if ((TREND)->mode != PM_TREND_NONE) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unknown trend mode for %s: %x", #TREND, (TREND)->mode); \
    return RVAL; \
}

#define PM_ASSERT_TREND2D_STATS(TREND, RVAL) \
if (!(TREND)->stats) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Trend %s statistics is NULL", #TREND); \
    return RVAL; \
}


// allocate a pmTrend2D structure tied to an image dimensions.  nXtrend,nYtrend is the order for the polynomials, max number of grid cells for
// psImageMap
pmTrend2D *pmTrend2DAlloc(pmTrend2DMode mode,
                          psImage *image,
                          int nXtrend, int nYtrend,
                          psStats *stats
);

bool psMemCheckTrend2D(psPtr ptr
    );

pmTrend2D *pmTrend2DNoImageAlloc(pmTrend2DMode mode,
                                 psImageBinning *binning,
                                 psStats *stats
    );

// allocate a pmTrend2D tied to an abstract field with size nXfield,nYfield
pmTrend2D *pmTrend2DFieldAlloc(pmTrend2DMode mode,
                               int nXfield, int nYfield,
                               int nXtrend, int nYtrend,
                               psStats *stats
    );

bool pmTrend2DFit(bool *goodFit,
		  pmTrend2D *trend,
                  psVector *mask,       // Warning: mask is modified!
                  psVectorMaskType maskVal,
                  const psVector *x,
                  const psVector *y,
                  const psVector *f,
                  const psVector *df
    );

double pmTrend2DEval(const pmTrend2D *trend,
                     float x, float y
    );
psVector *pmTrend2DEvalVector(const pmTrend2D *trend, psVector *mask, psVectorMaskType maskValue, 
                              const psVector *x, const psVector *y
    );

psString pmTrend2DModeToString(pmTrend2DMode mode);
pmTrend2DMode pmTrend2DModeFromString(psString name);

bool pmTrend2DPrintMap (pmTrend2D *trend);

/// @}
# endif
