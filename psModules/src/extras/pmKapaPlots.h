/* @file  pmKapaPlots.h
 * @brief functions to make plots with the external program 'kapa'
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_KAPA_PLOTS_H
#define PM_KAPA_PLOTS_H

/// @addtogroup Extras Miscellaneous Funtions
/// @{

// move to psLib or psModules
int pmKapaOpen (bool showWindow);
bool pmKapaClose(void);
bool pmKapaPlotVectorPair (psVector *xVec, psVector *yVec);

# if (HAVE_KAPA)
# include <kapa.h>

// yes, this is an absurd name...
bool pmKapaPlotVectorPair_AutoLimits_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec);
bool pmKapaPlotVectorTriple_AutoLimits_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing);
# else

bool pmKapaPlotVectorPair_AutoLimits_OpenGraph (int kapa, void *graphdata, psVector *xVec, psVector *yVec);
bool pmKapaPlotVectorTriple_AutoLimits_OpenGraph (int kapa, void *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing);
# endif

/// @}
#endif // PM_KAPA_PLOTS_H
