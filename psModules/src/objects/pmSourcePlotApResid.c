/** @file  pmSourcePlot.c
 *
 *  Plot the Aperture Mag - Fitted Mag residuals
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *  Copyright 2006 IfA, University of Hawaii
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
bool pmSourcePlotApResid (const pmFPAview *view, pmFPAfile *file, const pmConfig *config,
                          pmSourcePlotLayout *layout)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(layout, false);

    bool status;
    Graphdata graphdata;
    KapaSection section;

    psLogMsg ("psphot", 3, "creating ap mag - psf mag plot");

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

    psVector *x = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    graphdata.xmin = +32.0;
    graphdata.xmax = -32.0;
    graphdata.ymin = +32.0;
    graphdata.ymax = -32.0;

    // construct the plot vectors
    int n = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
	if (!source) continue;
        if (source->type != PM_SOURCE_TYPE_STAR) continue;
	if (!isfinite (source->apMag)) continue;
	if (!isfinite (source->psfMag)) continue;

        x->data.F32[n] = source->psfMag;
        y->data.F32[n] = source->apMag - source->psfMag;
        graphdata.xmin = PS_MIN(graphdata.xmin, x->data.F32[n]);
        graphdata.xmax = PS_MAX(graphdata.xmax, x->data.F32[n]);
        graphdata.ymin = PS_MIN(graphdata.ymin, y->data.F32[n]);
        graphdata.ymax = PS_MAX(graphdata.ymax, y->data.F32[n]);

        n++;
    }
    x->n = y->n = n;

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
    KapaSendLabel (kapa, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "Ap Mag - PSF Mag", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, n, &graphdata);
    KapaPlotVector (kapa, n, x->data.F32, "x");
    KapaPlotVector (kapa, n, y->data.F32, "y");

    if (layout->i == layout->nTotal - 1) {
        psLogMsg ("psphot", 3, "saving plot to %s", file->filename);
        KapaPNG (kapa, file->filename);
	KapaClearPlots (kapa);
    }

    psFree (x);
    psFree (y);
    return true;
}

# else

bool pmSourcePlotApResid (const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout)
{
    psLogMsg ("psphot", 3, "skipping ap-mag resid plot");
    return true;
}

# endif
