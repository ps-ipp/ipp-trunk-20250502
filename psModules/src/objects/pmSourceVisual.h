/* @file  pmKapaPlots.h
 * @brief functions to make plots with the external program 'kapa'
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.1.16.1 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-19 17:59:50 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_SOURCE_VISUAL_H
#define PM_SOURCE_VISUAL_H

bool pmSourceVisualClose(void);

/// @addtogroup Extras Miscellaneous Funtions
/// @{

bool pmSourceVisualPSFModelResid (pmTrend2D *trend, psVector *x, psVector *y, psVector *param, psVector *mask);
bool pmSourceVisualPlotPSFMetric (pmPSFtry *try);
bool pmSourceVisualPlotPSFMetricSubpix (pmPSFtry *try);

bool pmSourceVisualShowModelFit (pmSource *source);
bool pmSourceVisualShowModelFits (pmPSF *psf, psArray *sources, psImageMaskType maskVal);

/// @}
#endif // PM_KAPA_PLOTS_H
