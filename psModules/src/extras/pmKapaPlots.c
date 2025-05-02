/** @file  pmAstrometryWCS.c
 *
 *  @brief functions to convert FITS WCS keywords to / from pmFPA structures
 *
 *  @ingroup Astrometry
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-06-20 02:20:07 $
 *
 *  Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "pmKapaPlots.h"

// the top portion of this file defines plotting functions which use kapa for plotting.
// if kapa is not available, these functions are defined in the bottom portion as stubs
// which perform NOP and return false (XXX should this be 'true' ??)

# if (HAVE_KAPA)

    // XXX not thread safe (perhaps not needed)
    // to make this thread safe, we could check the thread ID and tie to it
    static int kapa_fd = -1;

int pmKapaOpen (bool showWindow)
{
    char kapa[64];

    if (showWindow) {
        strcpy (kapa, "kapa");
    } else {
        strcpy (kapa, "kapa -noX");
    }

    if (kapa_fd == -1) {
        // kapa_fd = KapaOpen (kapa, "psphot");
        kapa_fd = KapaOpenNamedSocket (kapa, "psphot");
    }
    return kapa_fd;
}

bool pmKapaClose (void)
{

    if (kapa_fd == -1)
        return true;
    KapaClose (kapa_fd);
    kapa_fd = -1;
    return true;
}

bool pmKapaPlotVectorPair_AutoLimits_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec)
{

    // set limits based on data values
    graphdata->xmin = +FLT_MAX;
    graphdata->xmax = -FLT_MAX;
    graphdata->ymin = +FLT_MAX;
    graphdata->ymax = +FLT_MAX;
    for (int i = 0; i < xVec->n; i++) {
        graphdata->xmin = PS_MIN (graphdata->xmin, xVec->data.F32[i]);
        graphdata->xmax = PS_MAX (graphdata->xmax, xVec->data.F32[i]);
        graphdata->ymin = PS_MIN (graphdata->ymin, yVec->data.F32[i]);
        graphdata->ymax = PS_MAX (graphdata->ymax, yVec->data.F32[i]);
    }
    // add 5% to range
    float range;

    range = graphdata->xmax - graphdata->xmin;
    graphdata->xmax += 0.05*range;
    graphdata->xmin -= 0.05*range;

    range = graphdata->ymax - graphdata->ymin;
    graphdata->ymax += 0.05*range;
    graphdata->ymin -= 0.05*range;

    KapaSetLimits (kapa, graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, graphdata);

    KapaPrepPlot (kapa, xVec->n, graphdata);
    KapaPlotVector (kapa, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa, yVec->n, yVec->data.F32, "y");
    return true;
}

bool pmKapaPlotVectorPair (psVector *xVec, psVector *yVec)
{

    Graphdata graphdata;

    int kapa = pmKapaOpen (true);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    if (xVec->n != yVec->n)
        return false;

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa);

    // set limits based on data values
    graphdata.xmin = +FLT_MAX;
    graphdata.xmax = -FLT_MAX;
    graphdata.ymin = +FLT_MAX;
    graphdata.ymax = -FLT_MAX;
    for (int i = 0; i < xVec->n; i++) {
        graphdata.xmin = PS_MIN (graphdata.xmin, xVec->data.F32[i]);
        graphdata.xmax = PS_MAX (graphdata.xmax, xVec->data.F32[i]);
        graphdata.ymin = PS_MIN (graphdata.ymin, yVec->data.F32[i]);
        graphdata.ymax = PS_MAX (graphdata.ymax, yVec->data.F32[i]);
    }
    // add 5% to range
    float range;

    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;

    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    KapaSetLimits (kapa, &graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa, xVec->n, &graphdata);
    KapaPlotVector (kapa, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa, yVec->n, yVec->data.F32, "y");

    return true;
}

bool pmKapaPlotVectorTriple_AutoLimits_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing)
{

    // set limits based on data values
    graphdata->xmin = +FLT_MAX;
    graphdata->xmax = -FLT_MAX;
    graphdata->ymin = +FLT_MAX;
    graphdata->ymax = -FLT_MAX;
    float zmin = +FLT_MAX;
    float zmax = -FLT_MAX;
    for (int i = 0; i < xVec->n; i++) {
        graphdata->xmin = PS_MIN (graphdata->xmin, xVec->data.F32[i]);
        graphdata->xmax = PS_MAX (graphdata->xmax, xVec->data.F32[i]);
        graphdata->ymin = PS_MIN (graphdata->ymin, yVec->data.F32[i]);
        graphdata->ymax = PS_MAX (graphdata->ymax, yVec->data.F32[i]);
        zmin = PS_MIN (zmin, zVec->data.F32[i]);
        zmax = PS_MAX (zmax, zVec->data.F32[i]);
    }

    // add 5% to range
    float range;

    psVector *zScale = psVectorAlloc (zVec->n, PS_DATA_F32);

    range = zmax - zmin;
    if (range == 0.0) {
        psVectorInit (zScale, 1.0);
    } else {
        for (int i = 0; i < zVec->n; i++) {
            if (increasing) {
                zScale->data.F32[i] = PS_MIN (1.5, PS_MAX(0.05, 1.5*(zVec->data.F32[i] - zmin)/range));
            } else {
                zScale->data.F32[i] = PS_MIN (1.5, PS_MAX(0.05, 1.5*(zmax - zVec->data.F32[i])/range));
            }
        }
    }

    range = graphdata->xmax - graphdata->xmin;
    graphdata->xmax += 0.05*range;
    graphdata->xmin -= 0.05*range;

    range = graphdata->ymax - graphdata->ymin;
    graphdata->ymax += 0.05*range;
    graphdata->ymin -= 0.05*range;

    KapaSetLimits (kapa, graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, graphdata);

    // the point size will be scaled from the z vector
    graphdata->size = -1;
    KapaPrepPlot (kapa, xVec->n, graphdata);
    KapaPlotVector (kapa, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa, yVec->n, yVec->data.F32, "y");
    KapaPlotVector (kapa, zVec->n, zScale->data.F32, "z");
    psFree (zScale);
    return true;
}

# else
    # include "pmKapaPlots.h"

    int pmKapaOpen (bool showWindow)
{
    return -1;
}

bool pmKapaClose ()
{
    return true;
}

bool pmKapaPlotVectorPair (psVector *xVec, psVector *yVec)
{
    return false;
}

bool pmKapaPlotVectorPair_AutoLimits_OpenGraph (int kapa, void *graphdata, psVector *xVec, psVector *yVec)
{
    return false;
}

bool pmKapaPlotVectorTriple_AutoLimits_OpenGraph (int kapa, void *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing)
{
    return false;
}
# endif
