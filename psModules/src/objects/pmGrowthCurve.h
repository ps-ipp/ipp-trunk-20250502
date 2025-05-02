/* @file  pmGrowthCurve.h
 * @brief functions to manipulate the curve-of-growth data
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-10 01:09:20 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_GROWTH_CURVE_H
# define PM_GROWTH_CURVE_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct
{
    psVector *radius;
    psVector *apMag;
    psF32 refRadius;
    psF32 maxRadius;
    psF32 fitMag;
    psF32 refMag;
    psF32 apRef;   // apMag[refRadius]
    psF32 apLoss;  // fitMag - apRef
    int refBin;
}
pmGrowthCurve;

bool psMemCheckGrowthCurve(psPtr ptr);

pmGrowthCurve *pmGrowthCurveAlloc (psF32 minRadius, psF32 maxRadius, psF32 refRadius);
psF32 pmGrowthCurveCorrect (pmGrowthCurve *growth, psF32 radius);

/// @}
# endif /* PM_GROWTH_CURVE_H */
