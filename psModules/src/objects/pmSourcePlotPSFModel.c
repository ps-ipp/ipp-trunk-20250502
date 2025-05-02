/** @file  pmSourcePlot.c
 *
 * This file contains functions to write source plots
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.14 $ $Name: not supported by cvs2svn $
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

// plot the sx, sy, sxy as vector field,
// plot the PSF measured sx, sy, sxy as vector field
// pull the sources from the config / file?
bool pmSourcePlotPSFModel (const pmFPAview *view, pmFPAfile *file, const pmConfig *config,
                           pmSourcePlotLayout *layout)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(layout, false);

    bool status;
    Graphdata graphdata;
    KapaSection section;

    psLogMsg ("psphot", 3, "creating psf model plot");

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

    int DX = 1000;
    float dx = DX / layout->nX;
    float dy = dx * layout->aspectRatio;
    int DY = dy * layout->nY;

    // XXX make the aspect-ratio match the image
    if (layout->i == 0) {
        KapaResize (kapa, DX, DY);
    }
    
    KapaClearPlots (kapa);
    KapaInitGraph (&graphdata);
    section.dx = dx / DX;
    section.dy = dy / DY;
    section.x = layout->iX * section.dx;
    section.y = layout->iY * section.dy;
    section.name = NULL;
    psStringAppend (&section.name, "a%d", layout->i);
    KapaSetSection (kapa, &section);
    psFree (section.name);

    psVector *xMNT = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);
    psVector *yMNT = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);
    psVector *xPSF = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);
    psVector *yPSF = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);
    psVector *xMIN = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);
    psVector *yMIN = psVectorAllocEmpty (2*sources->n, PS_TYPE_F32);

    // construct the plot vectors
    int nMNT = 0;
    int nPSF = 0;
    int nMIN = 0;
    dx = 0;
    dy = 0;
    float scale = 10;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (source->moments == NULL)
            continue;
        if (source->moments->SN < 25)
            continue;
        if (source->type != PM_SOURCE_TYPE_STAR)
            continue;

        pmModel *model = source->modelPSF;
        if (model == NULL)
            continue;

        psF32 *PAR = model->params->data.F32;

        psEllipseMoments moments;
        moments.x2 = source->moments->Mxx;
        moments.xy = source->moments->Mxy;
        moments.y2 = source->moments->Myy;

        // force the axis ratio to be < 20.0
        psEllipseAxes axes_mnt = psEllipseMomentsToAxes (moments, 20.0);
        psEllipseAxes axes_psf = pmPSF_ModelToAxes (PAR, model->class->useReff);

        // moments major axis
        dx = scale*axes_mnt.major*cos(axes_mnt.theta);
        dy = scale*axes_mnt.major*sin(axes_mnt.theta);
        xMNT->data.F32[nMNT] = PAR[PM_PAR_XPOS] - dx;
        yMNT->data.F32[nMNT] = PAR[PM_PAR_YPOS] - dy;
        nMNT++;
        xMNT->data.F32[nMNT] = PAR[PM_PAR_XPOS] + dx;
        yMNT->data.F32[nMNT] = PAR[PM_PAR_YPOS] + dy;
        nMNT++;

        // psf major axis
        dx = scale*axes_psf.major*cos(axes_psf.theta);
        dy = scale*axes_psf.major*sin(axes_psf.theta);
        xPSF->data.F32[nPSF] = PAR[PM_PAR_XPOS] - dx;
        yPSF->data.F32[nPSF] = PAR[PM_PAR_YPOS] - dy;
        nPSF++;
        xPSF->data.F32[nPSF] = PAR[PM_PAR_XPOS] + dx;
        yPSF->data.F32[nPSF] = PAR[PM_PAR_YPOS] + dy;
        nPSF++;

        // minor axis (to show size)
        dy = +scale*axes_psf.minor*cos(axes_psf.theta);
        dx = -scale*axes_psf.minor*sin(axes_psf.theta);
        xMIN->data.F32[nMIN] = PAR[PM_PAR_XPOS] - dx;
        yMIN->data.F32[nMIN] = PAR[PM_PAR_YPOS] - dy;
        nMIN++;
        xMIN->data.F32[nMIN] = PAR[PM_PAR_XPOS] + dx;
        yMIN->data.F32[nMIN] = PAR[PM_PAR_YPOS] + dy;
        nMIN++;

        graphdata.xmin = PS_MIN(graphdata.xmin, PAR[PM_PAR_XPOS]);
        graphdata.xmax = PS_MAX(graphdata.xmax, PAR[PM_PAR_XPOS]);
        graphdata.ymin = PS_MIN(graphdata.ymin, PAR[PM_PAR_YPOS]);
        graphdata.ymax = PS_MAX(graphdata.ymax, PAR[PM_PAR_YPOS]);
    }
    xMNT->n = yMNT->n = nMNT;
    xPSF->n = yPSF->n = nPSF;
    xMIN->n = yMIN->n = nMIN;

    float range;
    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;
    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    // XXX set the plot range to match the image
    KapaSetLimits (kapa, &graphdata);

    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "x (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "y (pixels)", KAPA_LABEL_YM);
    KapaSendLabel (kapa, "vector is major axis (scaled by 20) : black are moments, blue are psf model, red is psf minor axis", KAPA_LABEL_XP);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_PAIR_CONNECT;
    graphdata.size = 0.3;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, nMNT, &graphdata);
    KapaPlotVector (kapa, nMNT, xMNT->data.F32, "x");
    KapaPlotVector (kapa, nMNT, yMNT->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = KAPA_POINT_PAIR_CONNECT;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, nPSF, &graphdata);
    KapaPlotVector (kapa, nPSF, xPSF->data.F32, "x");
    KapaPlotVector (kapa, nPSF, yPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = KAPA_POINT_PAIR_CONNECT;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, nMIN, &graphdata);
    KapaPlotVector (kapa, nMIN, xMIN->data.F32, "x");
    KapaPlotVector (kapa, nMIN, yMIN->data.F32, "y");

    if (layout->i == layout->nTotal - 1) {
        psLogMsg ("psphot", 3, "saving plot to %s", file->filename);
        KapaPNG (kapa, file->filename);
	KapaClearPlots (kapa);
    }

    psFree (xMNT);
    psFree (yMNT);
    psFree (xPSF);
    psFree (yPSF);
    psFree (xMIN);
    psFree (yMIN);

    return true;
}

# else

    bool pmSourcePlotPSFModel (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout)
{
    psLogMsg ("psphot", 3, "skipping psf model plot");
    return true;
}

# endif
