/* @file  pmGrowthCurveGenerate.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-24 00:11:02 $
 * Copyright 2007 IfA, University of Hawaii
 */

# ifndef PM_GROWTH_CURVE_GENERATE_H
# define PM_GROWTH_CURVE_GENERATE_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

bool pmGrowthCurveGenerate (pmReadout *readout, pmPSF *psf, bool ignore, psImageMaskType maskVal, psImageMaskType mark);
pmGrowthCurve *pmGrowthCurveForPosition (psImage *image, pmPSF *psf, bool ignore, psImageMaskType maskVal, psImageMaskType markVal, float xc, float yc);
bool pmGrowthCurveGenerateFromSources (pmReadout *readout, pmPSF *psf, psArray *sources, bool INTERPOLATE_AP, psImageMaskType maskVal, psImageMaskType markVal);
pmGrowthCurve *pmGrowthCurveForSource (pmSource *source, pmPSF *psf, pmSourcePhotometryMode photMode, psImageMaskType maskVal, psImageMaskType markVal);

/// @}
# endif /* PM_GROWTH_CURVE_GENERATE_H */
