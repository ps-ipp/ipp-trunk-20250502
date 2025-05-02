/* @file  pmSourcePlots.h
 * @brief functions to create plots illustrating source properties
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-10 01:09:20 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_SOURCE_PLOTS_H
#define PM_SOURCE_PLOTS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct
{
    int nX;
    int nY;
    int nTotal;
    int iX;
    int iY;
    int i;
    float aspectRatio;
}
pmSourcePlotLayout;

// typedef bool (*pmSourcePlotFunction)(pmConfig *config, pmFPAview *view, pmSourcePlotLayout *layout);

pmSourcePlotLayout *pmSourcePlotLayoutAlloc(void);
bool psMemCheckSourcePlotLayout(psPtr ptr);

bool pmFPAviewWriteSourcePlot(const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmFPAWriteSourcePlot (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout);
bool pmChipWriteSourcePlot (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout);
bool pmSourcePlotPSFModel (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout);
bool pmSourcePlotMoments (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout);
bool pmSourcePlotApResid (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout);

/// @}
#endif // PM_SOURCE_PLOTS_H
