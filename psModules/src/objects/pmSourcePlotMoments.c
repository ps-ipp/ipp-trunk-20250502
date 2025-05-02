/** @file  pmSourcePlot.c
 *
 * This file contains functions to write source plots
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2006 IfA, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmDetections.h"
#include "pmSourcePlots.h"
#include "pmKapaPlots.h"

// this variable is defined in psmodules.h if ohana-config is found
# if (HAVE_KAPA)
# include <kapa.h>

// plot the sx, sy moments plane (faint and bright sources)
bool pmSourcePlotMoments (const pmFPAview *view, pmFPAfile *file, const pmConfig *config,
                          pmSourcePlotLayout *layout)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(layout, false);

    bool status;
    Graphdata graphdata;
    KapaSection section;

    psLogMsg ("psphot", 3, "creating moments plot");

    // find the currently selected readout
    pmReadout  *readout = pmFPAfileThisReadout (config->files, view, "PSPHOT.INPUT");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (detections == NULL) return false;

    psArray *sources = detections->allSources;
    if (sources == NULL) return false;

    int kapa = pmKapaOpen (false);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    // moments plot subplots are square
    int DX = 1000;
    int dx = DX / layout->nX;
    int dy = dx;
    int DY = dy * layout->nY;

    // XXX make the aspect-ratio match the image
    if (layout->i == 0) {
        KapaResize (kapa, DX, DY);
    }

    KapaClearPlots (kapa);
    KapaInitGraph (&graphdata);
    section.dx = dx / (float) DX;
    section.dy = dy / (float) DY;
    section.x = layout->iX * section.dx;
    section.y = layout->iY * section.dy;
    section.name = NULL;
    psStringAppend (&section.name, "a%d", layout->i);
    KapaSetSection (kapa, &section);
    psFree (section.name);

    // examine sources to set data range
    graphdata.xmin = -0.05;
    graphdata.ymin = -0.05;
    graphdata.xmax = +4.05;
    graphdata.ymax = +4.05;
    KapaSetLimits (kapa, &graphdata);

    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "&ss&h_x| (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "&ss&h_y| (pixels)", KAPA_LABEL_YM);

    psVector *xBright = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yBright = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *xFaint  = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yFaint  = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    // construct the vectors
    int nB = 0;
    int nF = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (source->moments == NULL)
            continue;

        xFaint->data.F32[nF] = source->moments->Mxx;
        yFaint->data.F32[nF] = source->moments->Myy;
        nF++;

        // XXX make this a user-defined cutoff
        if (source->moments->SN < 50)
            continue;

        xBright->data.F32[nB] = source->moments->Mxx;
        yBright->data.F32[nB] = source->moments->Myy;
        nB++;
    }
    xFaint->n = nF;
    yFaint->n = nF;

    xBright->n = nB;
    yBright->n = nB;

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.3;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, nF, &graphdata);
    KapaPlotVector (kapa, nF, xFaint->data.F32, "x");
    KapaPlotVector (kapa, nF, yFaint->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, nB, &graphdata);
    KapaPlotVector (kapa, nB, xBright->data.F32, "x");
    KapaPlotVector (kapa, nB, yBright->data.F32, "y");

    if (layout->i == layout->nTotal - 1) {
        psLogMsg ("psphot", 3, "saving plot to %s", file->filename);
        KapaPNG (kapa, file->filename);
	KapaClearPlots (kapa);
    }

    psFree (xBright);
    psFree (yBright);
    psFree (xFaint);
    psFree (yFaint);

    return true;
}


# else

    bool pmSourcePlotMoments (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout)
{
    psLogMsg ("psphot", 3, "skipping moments plot");
    return true;
}

# endif
