#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAfile.h"
#include "pmAstrometryObjects.h"
#include "pmAstrometryVisual.h"
#include "pmFPAExtent.h"

#if (HAVE_KAPA)
#include <kapa.h>
#include "pmKapaPlots.h"
#include "pmVisual.h"
#include "pmVisualUtils.h"

// variables to determine when things are plotted
static bool plotGridMatch        = true;
static bool plotTweak            = true;
static bool plotRawStars         = true;
static bool plotRefStars         = false;
static bool plotLumFunc          = true;
static bool plotRemoveClumps     = true;
static bool plotOneChipFit       = true;
static bool plotFixChips         = true;
static bool plotAstromGuessCheck = true;
static bool plotMosaicMatches    = true;
static bool plotCommonScale      = true;
static bool plotMosaicOneChip    = true;

// variables to store plotting window indices
static int kapa1 = -1;
static int kapa2 = -1;
static int kapa3 = -1;

// helper prototypes
bool residPlot (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe,
                char *title);


/* Initialization Routines  */

bool pmAstromVisualClose(void)
{
    if (kapa1 != -1) KapaClose(kapa1);
    if (kapa2 != -1) KapaClose(kapa2);
    return true;
}


/* Plotting Routines */
bool pmAstromVisualPlotRawStars (psArray *rawstars, pmFPA *fpa, pmChip *chip, psMetadata *recipe)
{
    if (!plotRawStars) return true;
    if (!pmVisualTestLevel("psastro.plot1", 1)) return true;
    if (!pmVisualInitWindow (&kapa1, "psastro:plots")) return false;

    Graphdata graphdata;
    KapaSection section;

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa1);
    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    section.dx = 0.5;
    section.dy = 0.5;

    // initialize and populate plotting vectors
    bool status = false;
    float iMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MIN");
    float iMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MAX");

    psVector *xVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);

    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a0");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    // Chip coordinates
    int n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel(kapa1, "Chip", KAPA_LABEL_XP);
    KapaSendLabel(kapa1, "X", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "Y", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // Focal Plane Coordinates
    section.x = 0.5;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->FP->x;
        yVec->data.F32[n] = raw->FP->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "Focal Plane", KAPA_LABEL_XP);
    KapaSendLabel (kapa1, "L", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "M", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // Tangent Plane Coordinates
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->TP->x;
        yVec->data.F32[n] = raw->TP->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "Tangential Plane", KAPA_LABEL_XP);
    KapaSendLabel (kapa1, "P", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Q", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // sky coordinates
    section.x = 0.5;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a3");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = DEG_RAD*raw->sky->r;
        yVec->data.F32[n] = DEG_RAD*raw->sky->d;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "Sky", KAPA_LABEL_XP);
    KapaSendLabel(kapa1, "RA", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "Dec", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // flip x (East increase to left)
    SWAP (graphdata.xmin, graphdata.xmax);
    KapaSetLimits (kapa1, &graphdata);

    // plot label
    section.x = 0.0;
    section.y = 0.0;
    section.dx = 1.0;
    section.dy = 1.0;
    section.name = NULL;
    psStringAppend (&section.name, "a5");
    KapaSetSection (kapa1, &section);
    KapaSendLabel (kapa1, "Raw Star Selection - Initial Astrometry", KAPA_LABEL_XP);
    psFree (section.name);

    // pause and wait for user input:
    pmVisualAskUser(&plotRawStars);

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);
    return true;
}


bool pmAstromVisualPlotRefStars (psArray *refstars, psMetadata *recipe)
{
    if (!plotRefStars) return true;
    if (!pmVisualTestLevel("psastro.plot2", 1)) return true;
    if (!pmVisualInitWindow (&kapa1, "psastro:plots")) return false;

    Graphdata graphdata;
    KapaInitGraph (&graphdata);
    KapaClearSections (kapa1);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    // initialize and populate plot vectors
    bool status = false;
    float rMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MIN");
    float rMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MAX");

    psVector *xVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (refstars->n, PS_TYPE_F32);

    int n = 0;
    for (int i = 0; i < refstars->n; i++) {
        pmAstromObj *ref = refstars->data[i];
        if (!isfinite(ref->Mag)) continue;
        if (ref->Mag > rMagMax) continue;
        if (ref->Mag < rMagMin) continue;

        xVec->data.F32[n] = DEG_RAD*ref->sky->r;
        yVec->data.F32[n] = DEG_RAD*ref->sky->d;
        zVec->data.F32[n] = ref->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "RA", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Dec", KAPA_LABEL_YM);
    KapaSendLabel (kapa1, "Reference Stars", KAPA_LABEL_XP);
    pmVisualTriplePlot(kapa1, &graphdata, xVec, yVec, zVec, false);

    // flip x (East increase to left)
    SWAP (graphdata.xmin, graphdata.xmax);
    KapaSetLimits (kapa1, &graphdata);

    // pause and wait for user input:
    pmVisualAskUser(&plotRefStars);

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);
    return true;
}


bool pmAstromVisualPlotLuminosityFunction (psVector *lnMag,   // Log(n) for each magnitude bin
                                          psVector *Mag,     // magnitude bins
                                          pmLumFunc *lumFunc,// Fit to the reference star luminosity function
                                          pmLumFunc *rawFunc // Fit to the raw star luminoisty function
                                          )
{
    if (!plotLumFunc ) return true;
    if (!pmVisualTestLevel("psastro.plot3", 1)) return true;
    if (!pmVisualInitWindow (&kapa1, "psastro:plots")) return false;

    int colorNone = KapaColorByName ("none"); // XXX probably should be 'none'

    Graphdata graphdata;
    KapaSection section1 = {"s1", 0.0, 0.0, 1.0, 0.5, colorNone};
    KapaSection section2 = {"s2", 0.0, 0.5, 1.0, 0.5, colorNone};
    KapaSection section;
    if (rawFunc == NULL) {
        section = section1;
    } else {
        section = section2;
    }

    if (rawFunc == NULL) KapaClearPlots(kapa1);
    KapaInitGraph(&graphdata);

    // Determine Plot Limits
    pmVisualScaleGraphdata(&graphdata, Mag, lnMag, false);

    // Make a line for the fit
    float x[2] = {graphdata.xmin, graphdata.xmax};
    float y[2] = {lumFunc->offset + x[0] * lumFunc->slope,
                 lumFunc->offset + x[1] * lumFunc->slope};

    // Plot Data
    KapaSetSection(kapa1, &section);
    KapaSetLimits(kapa1, &graphdata);

    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "Magnitude", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Log(N)", KAPA_LABEL_YM);
    if (rawFunc == NULL)
        KapaSendLabel (kapa1, "Raw Star Luminosity Function", KAPA_LABEL_XP);
    else
        KapaSendLabel (kapa1,
                       "Reference Star Luminosity Function, Shifted Raw Fit, and Cutoff",
                       KAPA_LABEL_XP);
    graphdata.color = KapaColorByName("black");
    graphdata.style = KAPA_PLOT_HISTOGRAM;
    KapaPrepPlot (kapa1, lnMag->n, &graphdata);
    KapaPlotVector(kapa1, lnMag->n,   Mag->data.F32, "x");
    KapaPlotVector(kapa1, lnMag->n, lnMag->data.F32, "y");

    // Overplot fit
    graphdata.style = KAPA_PLOT_CONNECT;
    KapaPrepPlot(kapa1,2,&graphdata);
    KapaPlotVector(kapa1, 2, x, "x");
    KapaPlotVector(kapa1, 2, y, "y");

    // If rawFunc was supplied, overplot the raw star fit + cutoff
    if( rawFunc != NULL) {
        double mRef = 0.5*(lumFunc->mMin + lumFunc->mMax);
        double logRho = mRef * lumFunc->slope + lumFunc->offset;
        double mRaw = (logRho - rawFunc->offset) / rawFunc->slope;
        double deltaM = mRef - mRaw;
        double mRefMax = rawFunc->mMax + deltaM;

        float xraw[2] = {rawFunc->mMin + deltaM, rawFunc->mMax + deltaM};
        float yraw[2] = {rawFunc->offset + (rawFunc->slope) * rawFunc->mMin,
                        rawFunc->offset + (rawFunc->slope) * rawFunc->mMax};
        float x[2] = {mRefMax, mRefMax};
        float y[2] = {graphdata.ymin, graphdata.ymax};
        graphdata.color= KapaColorByName("red");
        KapaPrepPlot(kapa1, 2, &graphdata);
        KapaPlotVector(kapa1, 2, x, "x");
        KapaPlotVector(kapa1, 2, y, "y");
        KapaPrepPlot (kapa1, 2, &graphdata);
        KapaPlotVector (kapa1, 2, xraw, "x");
        KapaPlotVector (kapa1, 2, yraw, "y");

        // pause and wait for user input:
        pmVisualAskUser (&plotLumFunc);
    }
    return true;
} // end of pmAstromVisualPlotLuminosityFunction


bool pmAstromVisualPlotRemoveClumps (psArray *input, // Array containing the field stars
                                    psImage *count, // A 2D histogram of the field star distribution
                                    int scale,      // The pixel size of the histogram
                                    float limit     // The minimum numuber of stars in a bin flagged as a clump
                                    )
{
    if (!plotRemoveClumps) return true;
    if (!pmVisualTestLevel("psastro.plot4", 1)) return true;
    if (!pmVisualInitWindow (&kapa1, "psastro:plots")) return false;

    KapaSection section;
    Graphdata graphdata;

    KapaClearSections (kapa1);
    KapaInitGraph(&graphdata);
    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    section.x = 0.0;
    section.dx = 1;
    section.y = 0.0;
    section.dy = 1.0;
    section.name = NULL;
    psStringAppend( &section.name, "a0");
    KapaSetSection(kapa1, &section);
    psFree(section.name);

    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.color = KapaColorByName ("black");
    KapaClearPlots(kapa1);

    // set up plot vectors
    float Xmin = +FLT_MAX;
    float Xmax = -FLT_MAX;
    float Ymin = +FLT_MAX;
    float Ymax = -FLT_MAX;
    psVector *xVec = psVectorAlloc (input->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (input->n, PS_TYPE_F32);

    // determine boundaries for histogram bin calculation
    int n = 0;
    for (int i=0; i< input->n; i++) {
        pmAstromObj *obj = (pmAstromObj *)input->data[i];
        if (!isfinite(obj->FP->x)) continue;
        if (!isfinite(obj->FP->y)) continue;
        xVec->data.F32[n] = obj->FP->x;
        yVec->data.F32[n] = obj->FP->y;
        Xmin = PS_MIN (Xmin, xVec->data.F32[n]);
        Xmax = PS_MAX (Xmax, xVec->data.F32[n]);
        Ymin = PS_MIN (Ymin, yVec->data.F32[n]);
        Ymax = PS_MAX (Ymax, yVec->data.F32[n]);
        n++;
    }
    xVec->n = yVec->n = n;

    // plot stars
    graphdata.xmax = Xmax;
    graphdata.xmin = Xmin;
    graphdata.ymax = Ymax;
    graphdata.ymin = Ymin;
    KapaSetLimits (kapa1, &graphdata);
    KapaSetFont (kapa1, "helvetica", 14);

    KapaBox (kapa1, &graphdata);

    KapaSendLabel (kapa1, "L (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "M (pixels)", KAPA_LABEL_YM);
    KapaSendLabel (kapa1, "Regions Flagged as Clumps (Red Boxes)",
                   KAPA_LABEL_XP);

    KapaPrepPlot (kapa1, xVec->n, &graphdata);
    KapaPlotVector (kapa1, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa1, yVec->n, yVec->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.style = KAPA_PLOT_CONNECT;

    // overplot clumpy regions excluded from analysis
    for (int i = 0; i < count->numCols; i++) {
        for (int j = 0; j < count->numRows; j++) {
            if(count->data.U32[j][i] <= limit) continue; // not a clump
            float Xbot = (i - 5) * scale + Xmin;
            float Ybot = (j - 5) * scale + Ymin;
            if(Xbot < graphdata.xmin || Xbot > graphdata.xmax ||
               Ybot < graphdata.ymin || Ybot > graphdata.ymax) continue;
            float x[5] = {Xbot, Xbot + scale, Xbot + scale, Xbot, Xbot};
            float y[5] = {Ybot, Ybot, Ybot + scale, Ybot + scale, Ybot};
            KapaPrepPlot (kapa1, 5, &graphdata);
            KapaPlotVector (kapa1, 5, x, "x");
            KapaPlotVector (kapa1, 5, y, "y");
        }
    }

    // ask for user input and finish
    pmVisualAskUser (&plotRemoveClumps);
    psFree (xVec);
    psFree (yVec);

    return true;
}


bool pmAstromVisualPlotOneChipFit (psArray *rawstars, // stars detected in the image
                                  psArray *refstars, // reference stars over the same region
                                  psArray *match,    // contains which rawstars match to which refstars
                                  psMetadata *recipe // data reduction recipe
                                  )
{
    if (!plotOneChipFit) return true;
    if (!pmVisualTestLevel("psastro.plot5", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plot1")) return false;
    if (!pmVisualInitWindow(&kapa2, "psastro:plot2")) return false;
    if (!pmVisualInitWindow(&kapa3, "psastro:plot3")) return false;

    // plot the residuals
    if (!residPlot(rawstars, refstars, match, recipe, "Single Chip Fit Residuals (Chip Coordinates)")) {
        plotOneChipFit = false;
        return false;
    }

    // ask for user input and finish
    pmVisualAskUser(&plotOneChipFit);
    return true;
}


bool pmAstromVisualPlotFixChips (pmFPAfile *input, // focal plane array file
                                psVector *xOld, // old X location of chip cornerss
                                psVector *yOld // old Y location of chip corners
                                )
{
    if (!plotFixChips) return true;
    if (!pmVisualTestLevel("psastro.plot6", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plots")) return false;

    int colorNone = KapaColorByName ("none"); // XXX probably should be 'none'

    KapaSection section = {"s1", 0.0, 0.0, 1.0, 1.0, colorNone};
    Graphdata graphdata;
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa1);
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.style = KAPA_PLOT_POINTS;

    psVector *xNew = psVectorAllocEmpty (xOld->n, PS_TYPE_F32);
    psVector *yNew = psVectorAllocEmpty (yOld->n, PS_TYPE_F32);

    // copy of the code in psastroFixChips that generated xOld, yOld, but for xNew, yNew
    pmFPAview *view = pmFPAviewAlloc (0);

    pmChip *obsChip = NULL;
    while ((obsChip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

        psRegion *region = pmChipPixels(obsChip);
        psPlane ptCP, ptFP;

        ptCP.x = region->x0; ptCP.y = region->y0;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        psVectorAppend (xNew, ptFP.x);
        psVectorAppend (yNew, ptFP.y);

        ptCP.x = region->x0; ptCP.y = region->y1;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        psVectorAppend (xNew, ptFP.x);
        psVectorAppend (yNew, ptFP.y);

        ptCP.x = region->x1; ptCP.y = region->y1;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        psVectorAppend (xNew, ptFP.x);
        psVectorAppend (yNew, ptFP.y);

        ptCP.x = region->x1; ptCP.y = region->y0;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        psVectorAppend (xNew, ptFP.x);
        psVectorAppend (yNew, ptFP.y);

        psFree (region);
    }

    // set up graph
    pmVisualScaleGraphdata(&graphdata, xOld, yOld, true);
    pmVisualInitGraph(kapa1, &section, &graphdata);
    KapaSendLabel (kapa1, "L (FP)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "M (FP)", KAPA_LABEL_YM);
    KapaSendLabel (kapa1, "Chip corners before (black) and after (red) FixChips", KAPA_LABEL_XP);
    KapaPrepPlot (kapa1, xOld->n, &graphdata);
    KapaPlotVector (kapa1, xOld->n, xOld->data.F32, "x");
    KapaPlotVector (kapa1, xOld->n, yOld->data.F32, "y");

    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    graphdata.color = KapaColorByName("red");
    KapaPrepPlot (kapa1, xNew->n, &graphdata);
    KapaPlotVector (kapa1, xNew->n, xNew->data.F32, "x");
    KapaPlotVector (kapa1, xNew->n, yNew->data.F32, "y");

    pmVisualAskUser(&plotFixChips);
    psFree(xNew);
    psFree(yNew);
    psFree(view);
    return true;
}


bool pmAstromVisualPlotAstromGuessCheck (psVector *cornerPo, // P coordinates of chip corners before fitting
                                        psVector *cornerQo, // Q coordinates of chip corners before fitting
                                        psVector *cornerPn, // P coordinates of chip corners after fitting
                                        psVector *cornerQn, // Q coordinates of chip corners after fitting
                                        psVector *cornerPd, // P coordinate residuals of fit from old to new coordinates
                                        psVector *cornerQd  // Q coordinate residuals of fit from old to new coordinates
                                        )
{
    if (!plotAstromGuessCheck) return true;
    if (!pmVisualTestLevel("psastro.plot7", 1)) return true;
    if (!pmVisualInitWindow (&kapa1, "psastro:plots")) return false;

    Graphdata graphdata;
    KapaSection section;
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa1);

    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    section.dx = 0.4;
    section.dy = 0.4;

    // Old Corners
    section.x = 0.30;
    section.y = 0.50;
    section.name = NULL;
    psStringAppend (&section.name, "a0");
    KapaSetSection (kapa1, &section);
    psFree(section.name);

    pmVisualScaleGraphdata (&graphdata, cornerPo, cornerPo, true);
    KapaSetLimits (kapa1, &graphdata);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "P (Pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Q (Pixels)", KAPA_LABEL_YM);
    KapaSendLabel (kapa1,
                   "Fiducial Points in the Tangent Plane. Black: Initial Astrometry. Red: Final Astrometry",
                   KAPA_LABEL_XP);
    KapaPrepPlot (kapa1, cornerPo->n, &graphdata);
    KapaPlotVector (kapa1, cornerPo->n, cornerPo->data.F32, "x");
    KapaPlotVector (kapa1, cornerQo->n, cornerQo->data.F32, "y");

    // New Corners
    graphdata.color = KapaColorByName("red");
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 1.5;
    KapaPrepPlot (kapa1, cornerPn->n, &graphdata);
    KapaPlotVector (kapa1, cornerPn->n, cornerPn->data.F32, "x");
    KapaPlotVector (kapa1, cornerQn->n, cornerQn->data.F32, "y");

    // Residuals
    psVector *xResid = psVectorAlloc(cornerPn->n, PS_DATA_F32);
    psVector *yResid = psVectorAlloc(cornerQn->n, PS_DATA_F32);
    for(int i=0; i < cornerPn->n; i++) {
        xResid->data.F32[i] = (cornerPd->data.F32[i]);
        yResid->data.F32[i] = (cornerQd->data.F32[i]);
    }

    graphdata.color = KapaColorByName("black");
    graphdata.size=0.5;
    section.x = 0.3;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa1, &section);
    psFree(section.name);

    pmVisualScaleGraphdata (&graphdata, xResid, yResid, true);
    KapaSetLimits (kapa1, &graphdata);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "dP", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "dQ", KAPA_LABEL_YM);
    KapaSendLabel (kapa1,
                   "Residual of the Fit from the Initial Astrometry to the Final Astrometry",
                   KAPA_LABEL_XP);
    KapaPrepPlot (kapa1, cornerPd->n, &graphdata);
    KapaPlotVector (kapa1, cornerPd->n, xResid->data.F32, "x");
    KapaPlotVector (kapa1, cornerQd->n, yResid->data.F32, "y");

    psFree(xResid);
    psFree(yResid);
    pmVisualAskUser (&plotAstromGuessCheck);
    return true;
}


bool pmAstromVisualPlotCommonScale (pmFPA *fpa,         // the fpa
                                   psVector *oldScale  // the old pixel scale of each chip in the fpa
                                   )
{
    if (!plotCommonScale) return true;
    if (!pmVisualTestLevel("psastro.plot8", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plots")) return false;

    int colorNone = KapaColorByName ("none");
    KapaSection section = {"s1", 0.0, 0.0, 1.0, 1.0, colorNone};
    Graphdata graphdata;

    psPlane ptCH, ptFP;
    ptCH.x = 0;
    ptCH.y = 0;
    psVector *xVec = psVectorAlloc (oldScale->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (oldScale->n, PS_TYPE_F32);

    int nobj = 0;

    // project each chip corner to the Focal Plane
    for(int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->toFPA) { continue; }

        psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
        xVec->data.F32[nobj] = ptFP.x;
        yVec->data.F32[nobj] = ptFP.y;
        nobj++;
        if (nobj == oldScale->n) break;
    }

    // set up plot window
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa1);
    KapaSetSection (kapa1, &section);
    KapaSetFont (kapa1, "helvetica", 14);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, oldScale, false);
    KapaSendLabel (kapa1, "L (FP)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "M (FP)", KAPA_LABEL_YM);
    KapaSendLabel (kapa1, "Old Pixel Scale of FPA Chips (Not to Scale)", KAPA_LABEL_XP);

    pmVisualAskUser (&plotCommonScale);

    psFree(xVec);
    psFree(yVec);

    return true;
}


bool pmAstromVisualPlotMosaicOneChip (psArray *rawstars, psArray *refstars,
                                     psArray *match, psMetadata *recipe)
{
    if (!plotMosaicOneChip) return true;
    if (!pmVisualTestLevel("psastro.plot9", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plot1")) return false;
    if (!pmVisualInitWindow(&kapa2, "psastro:plot2")) return false;
    if (!pmVisualInitWindow(&kapa3, "psastro:plot3")) return false;

    // plot the residuals
    if (!residPlot(rawstars, refstars, match, recipe, "Single Chip Fit Residuals - Mosaic Mode")) {
        pmVisualSetVisual(false);
        return false;
    }

    // ask for user input and finish
    pmVisualAskUser(&plotMosaicOneChip);

    return true;
}


bool pmAstromVisualPlotMosaicMatches( psArray *rawstars, psArray *refstars,
                                    psArray *match, int iteration,
                                    psMetadata *recipe)
{
    if (!plotMosaicMatches) return true;
    if (!pmVisualTestLevel("psastro.plot10", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plot1")) return false;
    if (!pmVisualInitWindow(&kapa2, "psastro:plot2")) return false;
    if (!pmVisualInitWindow(&kapa3, "psastro:plot3")) return false;

    char title[60];
    sprintf(title, "Matches found during psastroMosaicSetMatch iteration %d", iteration);

    if (!residPlot(rawstars, refstars, match, recipe, title)){
        pmVisualSetVisual(false);
        return false;
    }

    // ask for user input
    pmVisualAskUser (&plotMosaicMatches);
    return true;
}


bool pmAstromVisualPlotGridMatch (const psArray *raw,
                                  const psArray *ref,
                                  psImage *gridNP,
                                  double offsetX,
                                  double offsetY,
                                  double maxOffpix,
                                  double Scale,
                                  double Offset)
{
    if (!plotGridMatch) return true;
    if (!pmVisualTestLevel("psastro.plot11", 1)) return true;
    if (!pmVisualInitWindow(&kapa1, "psastro:plots")) return false;

    int colorNone = KapaColorByName ("none");
    KapaSection section  = {"s1", 0.00, 0.00, 0.75, 0.75, colorNone};
    KapaSection sectionY = {"s2", 0.75, 0.00, 0.25, 0.75, colorNone};
    KapaSection sectionX = {"s3", 0.00, 0.75, 0.75, 0.25, colorNone};

    Graphdata graphdata;
    int nplot = raw->n * ref->n;                      // number of points to plot
    psVector *dXplot = psVectorAlloc (nplot, PS_TYPE_F32); // x data points
    psVector *dYplot = psVectorAlloc (nplot, PS_TYPE_F32); // y data points

    pmAstromObj *ob1; pmAstromObj *ob2;               // shortcuts to the data in raw and ref
    psU32 **NP = gridNP->data.U32;                    // shortcut to the gridNP data
    float vertHistSlice[gridNP->numRows];             // vertical histogram slice through peak
    float horizHistSlice[gridNP->numCols];            // horizontal histogram slice through peak
    float horizontalIndices[gridNP->numCols];         // the horizontal offset corresponding to each bin of horizHistSlice
    float verticalIndices[gridNP->numRows];           // the vertical offset corresponding to each bin of vertHistSlice
    int maxHorizontalSlice = 0;                       // peak value of horizHistSlice
    int maxVerticalSlice = 0;                         // peak value of vertHistSlice
    int peakXbin = (int) (offsetX / Scale + Offset);  // X bin index of peak
    int peakYbin = (int) (offsetY / Scale + Offset);  // Y bin index of peak

    // psVector *vertHistSlice = psVectorAlloc (gridNP->numRows, PS_TYPE_F32);

    // set up plot information
    KapaClearPlots(kapa1);
    KapaInitGraph(&graphdata);
    KapaSetSection(kapa1, &section);

    graphdata.xmin = -1.0 * maxOffpix;
    graphdata.xmax =  1.0 * maxOffpix;
    graphdata.ymin = -1.0 * maxOffpix;
    graphdata.ymax =  1.0 * maxOffpix;
    KapaSetLimits(kapa1, &graphdata);

    KapaSetFont(kapa1, "helvetica", 14);
    KapaBox(kapa1, &graphdata);
    KapaSendLabel (kapa1, "X offset (FP)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Y offset (FP)", KAPA_LABEL_YM);
    KapaSendLabel (kapa1, "pmAstromGridAngle residuals. Box: Correlation Peak.",
                   KAPA_LABEL_XP);
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.4;
    graphdata.color = KapaColorByName ("black");

    // calculate the plot points
    float dX, dY;
    for (int i = 0; i < raw->n; i++) {
        ob1 = (pmAstromObj *)raw->data[i];
        for (int j = 0; j < ref->n; j++) {
            ob2 = (pmAstromObj *)ref->data[j];
            dX = ob1->FP->x - ob2->FP->x;
            dY = ob1->FP->y - ob2->FP->y;
            dXplot->data.F32[(i * ref->n) + j] = dX;
            dYplot->data.F32[(i * ref->n) + j] = dY;
        }
    }

    // calculate the points for the profiles
    for (int i = 0; i < gridNP->numRows; i++) {
        vertHistSlice[i] = NP[i][peakXbin];
        verticalIndices[i] = (i - Offset) * Scale;
        if (vertHistSlice[i] > maxVerticalSlice) {
            maxVerticalSlice = vertHistSlice[i];
        }
    }
    for (int i = 0; i < gridNP->numCols; i++) {
        horizHistSlice[i] = NP[peakYbin][i];
        horizontalIndices[i] = (i - Offset) * Scale;
        if (horizHistSlice[i] > maxHorizontalSlice) {
            maxHorizontalSlice = horizHistSlice[i];
        }
    }

    // Plot the offsets
    KapaPrepPlot(kapa1, nplot, &graphdata);
    KapaPlotVector (kapa1, nplot, dXplot->data.F32, "x");
    KapaPlotVector (kapa1, nplot, dYplot->data.F32, "y");

    // Overplot bounding box, peak of distribution
    float xbound[5] = { -maxOffpix, maxOffpix, maxOffpix, -maxOffpix, -maxOffpix};
    float ybound[5] = { -maxOffpix, -maxOffpix, maxOffpix, maxOffpix, -maxOffpix};
    float xbin[5] = {offsetX - 0.5 * Scale, offsetX + 0.5 * Scale,
                     offsetX + 0.5 * Scale, offsetX - 0.5 * Scale,
                     offsetX - 0.5 * Scale};
    float ybin[5] = {offsetY - 0.5 * Scale, offsetY - 0.5 * Scale,
                     offsetY + 0.5 * Scale, offsetY + 0.5 * Scale,
                     offsetY - 0.5 * Scale};
    graphdata.color = KapaColorByName("red");
    graphdata.style = KAPA_PLOT_CONNECT;
    graphdata.size = 1.0;
    KapaPrepPlot(kapa1, 5, &graphdata);
    KapaPlotVector (kapa1, 5, xbound, "x");
    KapaPlotVector (kapa1, 5, ybound, "y");
    KapaPrepPlot(kapa1, 5, &graphdata);
    KapaPlotVector (kapa1, 5, xbin, "x");
    KapaPlotVector (kapa1, 5, ybin, "y");

    // plot X profile
    KapaSetSection(kapa1, &sectionX);
    graphdata.color = KapaColorByName("black");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    graphdata.style = KAPA_PLOT_HISTOGRAM;
    graphdata.ymin = 0;
    graphdata.ymax = maxHorizontalSlice + 0.5;
    strcpy (graphdata.labels, "0200");
    KapaSetLimits(kapa1, &graphdata);

    KapaBox(kapa1, &graphdata);
    KapaPrepPlot(kapa1, gridNP->numCols, &graphdata);
    KapaPlotVector (kapa1, gridNP->numCols, horizontalIndices, "x");
    KapaPlotVector (kapa1, gridNP->numCols, horizHistSlice, "y");

    float xslice[2] = {offsetX - Scale / 2., offsetX - Scale / 2.};
    float yslice[2] = {-5, 100};
    graphdata.style = KAPA_PLOT_CONNECT;
    graphdata.color = KapaColorByName("red");
    KapaPrepPlot(kapa1, 2, &graphdata);
    KapaPlotVector (kapa1, 2, xslice, "x");
    KapaPlotVector (kapa1, 2, yslice, "y");

    // plot Y profile
    KapaSetSection(kapa1, &sectionY);
    graphdata.color = KapaColorByName("black");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    graphdata.style = KAPA_PLOT_HISTOGRAM;
    graphdata.ymin = -maxOffpix;
    graphdata.ymax = maxOffpix;
    graphdata.xmin = -1.0 ;
    graphdata.xmax = maxVerticalSlice + 0.5;
    strcpy (graphdata.labels, "2000");
    KapaSetLimits(kapa1, &graphdata);

    KapaBox(kapa1, &graphdata);
    KapaPrepPlot(kapa1, gridNP->numRows, &graphdata);
    KapaPlotVector (kapa1, gridNP->numRows, vertHistSlice, "x");
    KapaPlotVector (kapa1, gridNP->numRows, verticalIndices, "y");

    yslice[0] = yslice[1] = offsetY - Scale / 2.;
    xslice[0] = -5; xslice[1] = 100;
    graphdata.style = KAPA_PLOT_CONNECT;
    graphdata.color = KapaColorByName("red");
    KapaPrepPlot(kapa1, 2, &graphdata);
    KapaPlotVector (kapa1, 2, xslice, "x");
    KapaPlotVector (kapa1, 2, yslice, "y");

    pmVisualAskUser(&plotGridMatch);
    psFree(dXplot);
    psFree(dYplot);
    return true;
} // end of pmAstromVisualPlotGridMatch


bool pmAstromVisualPlotGridMatchOverlay (const psArray *raw,
					 const psArray *ref,
					 const psPlane offset)
{
    if (!plotGridMatch) return true;
    if (!pmVisualTestLevel("psastro.plot12", 1)) return true;
    if (!pmVisualInitWindow(&kapa2, "psastro:plots")) return false;

    Graphdata graphdata;
    psVector *xPlot = psVectorAlloc (PS_MAX(raw->n, ref->n), PS_TYPE_F32); // x data points
    psVector *yPlot = psVectorAlloc (PS_MAX(raw->n, ref->n), PS_TYPE_F32); // y data points
    psVector *zPlot = psVectorAlloc (PS_MAX(raw->n, ref->n), PS_TYPE_F32); // y data points

    // set up plot information
    KapaClearPlots(kapa2);
    KapaInitGraph(&graphdata);

    KapaSetFont(kapa2, "helvetica", 14);
    KapaBox(kapa2, &graphdata);
    KapaSendLabel (kapa2, "X (FP)", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "Y (FP)", KAPA_LABEL_YM);
    KapaSendLabel (kapa2, "pmAstromGridAngle red: raw, black: ref.", KAPA_LABEL_XP);

    // plot the REF data.  (also calculate the plot ranges, accumulate the plot vectors)
    graphdata.xmin = +INT_MAX;
    graphdata.xmax = -INT_MAX;
    graphdata.ymin = +INT_MAX;
    graphdata.ymax = -INT_MAX;
    for (int i = 0; i < ref->n; i++) {
        pmAstromObj *obj = ref->data[i];
	graphdata.xmin = PS_MIN(graphdata.xmin, obj->FP->x);
	graphdata.xmax = PS_MAX(graphdata.xmax, obj->FP->x);
	graphdata.ymin = PS_MIN(graphdata.ymin, obj->FP->y);
	graphdata.ymax = PS_MAX(graphdata.ymax, obj->FP->y);
	xPlot->data.F32[i] = obj->FP->x + offset.x;
	yPlot->data.F32[i] = obj->FP->y + offset.y;
	zPlot->data.F32[i] = obj->Mag;
    }
    xPlot->n = yPlot->n = zPlot->n = ref->n;
    KapaSetLimits(kapa2, &graphdata);

    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN | PS_STAT_MIN  | PS_STAT_MAX );
    psVectorStats (stats, zPlot, NULL, NULL, 0);
    float range = stats->max - stats->min;
    range = PS_MAX (0.5, PS_MIN (6.0, range));
    float zero = stats->sampleMedian + 0.25*range;

    float maxZ = zPlot->data.F32[0], minZ = zPlot->data.F32[0];
    for (int i = 0; i < zPlot->n; i++) {
	maxZ = PS_MAX (maxZ, zPlot->data.F32[i]);
	minZ = PS_MIN (minZ, zPlot->data.F32[i]);
	float value = (zero - zPlot->data.F32[i]) / range;
	zPlot->data.F32[i] = PS_MAX(0.0, PS_MIN(1.0, value));
    }
    fprintf (stderr, "ref mags: %f to %f (%f median)\n", minZ, maxZ, stats->sampleMedian);

    // the point size will be scaled from the z vector
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = -1;
    graphdata.color = KapaColorByName ("black");

    KapaPrepPlot   (kapa2, xPlot->n, &graphdata);
    KapaPlotVector (kapa2, xPlot->n, xPlot->data.F32, "x");
    KapaPlotVector (kapa2, yPlot->n, yPlot->data.F32, "y");
    KapaPlotVector (kapa2, zPlot->n, zPlot->data.F32, "z");

    // plot the RAW data (keep previous limits)
    for (int i = 0; i < raw->n; i++) {
        pmAstromObj *obj = raw->data[i];
	xPlot->data.F32[i] = obj->FP->x;
	yPlot->data.F32[i] = obj->FP->y;
	zPlot->data.F32[i] = obj->Mag;
    }
    xPlot->n = yPlot->n = zPlot->n = raw->n;

    psStatsInit(stats);
    psVectorStats (stats, zPlot, NULL, NULL, 0);
    range = stats->max - stats->min;
    range = PS_MAX (0.5, PS_MIN (6.0, range));
    zero = stats->sampleMedian + 0.25*range;
    // zero = stats->sampleMedian + 1.0;
    // range = 6.0;

    maxZ = zPlot->data.F32[0], minZ = zPlot->data.F32[0];
    for (int i = 0; i < zPlot->n; i++) {
	maxZ = PS_MAX (maxZ, zPlot->data.F32[i]);
	minZ = PS_MIN (minZ, zPlot->data.F32[i]);
	float value = (zero - zPlot->data.F32[i]) / range;
	zPlot->data.F32[i] = PS_MAX(0.0, PS_MIN(1.0, value));
    }
    fprintf (stderr, "raw mags: %f to %f (%f median)\n", minZ, maxZ, stats->sampleMedian);

    // the point size will be scaled from the z vector
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.ptype = KAPA_POINT_CIRCLE_OPEN;
    graphdata.size = -1;
    graphdata.color = KapaColorByName ("red");

    KapaPrepPlot   (kapa2, xPlot->n, &graphdata);
    KapaPlotVector (kapa2, xPlot->n, xPlot->data.F32, "x");
    KapaPlotVector (kapa2, yPlot->n, yPlot->data.F32, "y");
    KapaPlotVector (kapa2, zPlot->n, zPlot->data.F32, "z");

    pmVisualAskUser(&plotGridMatch);
    psFree(xPlot);
    psFree(yPlot);
    psFree(zPlot);
    psFree(stats);
    return true;
}

bool pmAstromVisualPlotTweak (psVector *xHist, // Smoothed Horizontal cut through the histogram
                              psVector *yHist, // Smoothed Vertical cut throug the histogram
                              int xBin,        // X Bin index of the histogram peak
                              int yBin         // Y bin index of the histogram peak
    )
{
    if (!plotTweak) return true;
    if (!pmVisualTestLevel("psastro.plot13", 1)) return true;
    if (!pmVisualInitWindow(&kapa3, "psastro:plots")) return false;

    Graphdata graphdata;

    int colorNone = KapaColorByName ("none"); // XXX probably should be 'none'
    KapaSection section1 = {"s1", 0.0, 0.0, 1.0, 0.5, colorNone};
    KapaSection section2 = {"s2", 0.0, 0.5, 1.0, 0.5, colorNone};
    KapaSection section3 = {"s3", 0.0, 0.0, 1.0, 1.0, colorNone};

    psVector *xIndices = psVectorAlloc (xHist->n, PS_TYPE_F32);
    psVector *yIndices = psVectorAlloc (yHist->n, PS_TYPE_F32);

    // populate the Indices vectors
    for(int i = 0; i < xHist->n; i++) {
        xIndices->data.F32[i] = i;
    }
    for(int i = 0; i < yHist->n; i++) {
        yIndices->data.F32[i] = i;
    }

    // set up plot information
    KapaClearPlots(kapa3);
    KapaInitGraph(&graphdata);

    // plot the X histogram
    pmVisualScaleGraphdata(&graphdata, xIndices, xHist, false);
    KapaSetSection(kapa3, &section1);
    KapaSetLimits (kapa3, &graphdata);
    KapaSetFont(kapa3, "helvetica", 14);
    KapaBox(kapa3, &graphdata);
    KapaSendLabel (kapa3, "X offset Bin", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "Number of Sources", KAPA_LABEL_YM);
    KapaSendLabel (kapa3, "Horizontal Profile",
                   KAPA_LABEL_XP);
    graphdata.style = KAPA_PLOT_HISTOGRAM;
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.4;
    graphdata.color = KapaColorByName ("black");

    KapaPrepPlot (kapa3, xHist->n, &graphdata);
    KapaPlotVector (kapa3, xHist->n, xIndices->data.F32, "x");
    KapaPlotVector (kapa3, xHist->n, xHist->data.F32, "y");

    // overplot the peak
    float x[2] = {xBin, xBin};
    float y[2] = {-500, 500};
    graphdata.color = KapaColorByName ("red");
    KapaPrepPlot (kapa3, 2, &graphdata);
    KapaPlotVector (kapa3, 2, x, "x");
    KapaPlotVector (kapa3, 2, y, "y");

    // plot the Y histogram
    pmVisualScaleGraphdata(&graphdata, yIndices, yHist, false);
    KapaSetSection(kapa3, &section2);
    KapaSetLimits (kapa3, &graphdata);
    KapaSetFont(kapa3, "helvetica", 14);
    graphdata.color = KapaColorByName ("black");
    KapaBox(kapa3, &graphdata);
    KapaSendLabel (kapa3, "Y offset Bin", KAPA_LABEL_XM);
    KapaSendLabel (kapa3, "Number of Sources", KAPA_LABEL_YM);
    KapaSendLabel (kapa3, "Vertical Profile",
                   KAPA_LABEL_XP);
    graphdata.style = KAPA_PLOT_HISTOGRAM;
    graphdata.ptype = KAPA_POINT_BOX_SOLID;
    graphdata.size = 0.4;

    KapaPrepPlot (kapa3, yHist->n, &graphdata);
    KapaPlotVector (kapa3, yHist->n, yIndices->data.F32, "x");
    KapaPlotVector (kapa3, yHist->n, yHist->data.F32, "y");

    // overplot the peak
    x[0] = x[1] = yBin;
    graphdata.color = KapaColorByName ("red");
    KapaPrepPlot (kapa3, 2, &graphdata);
    KapaPlotVector (kapa3, 2, x, "x");
    KapaPlotVector (kapa3, 2, y, "y");

    // plot title
    graphdata.color = KapaColorByName("black");

    KapaSetSection( kapa3, &section3);
    KapaSendLabel (kapa3, "Tweaking the Astrometry Grid Solution. Smoothed profiles + peak location",
                   KAPA_LABEL_XP);

    pmVisualAskUser(&plotTweak);

    psFree(xIndices);
    psFree(yIndices);
    return true;
} // end of pmAstromPlotTweak


bool residPlot (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe,
                        char *title) {


    // initialize graph information
    Graphdata graphdata;
    KapaSection section;
    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa1);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CIRCLE_OPEN;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;

    section.dx = 0.4;
    section.dy = 0.4;

    // initialize and populate the plotting vectors
    bool status = false;
    float iMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MIN");
    float iMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MAX");
    float rMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MIN");
    float rMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MAX");

    psVector *xVec = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (match->n, PS_TYPE_F32);

    // X vs dX
    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a0");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    int n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "X", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "dX", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // X vs dY
    section.x = 0.5;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "X", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "dY", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // Y vs dX
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->y;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "Y", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "dX", KAPA_LABEL_YM);
    pmVisualTriplePlot (kapa1, &graphdata, xVec, yVec, zVec, false);

    // Y vs dY
    section.x = 0.5;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a3");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->y;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    KapaSendLabel (kapa1, "Y", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "dY", KAPA_LABEL_YM);
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa1, &graphdata, xVec, yVec, zVec, false);

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    section.x = 0.0;
    section.y = 0.0;
    section.dx = 0.95;
    section.dy = 0.95;
    section.name = NULL;
    psStringAppend (&section.name, "a5");
    KapaSetSection (kapa1, &section);
    KapaSendLabel (kapa1, title, KAPA_LABEL_XP);
    psFree (section.name);

    // ***************************************
    // second window

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa2);

    KapaSendLabel (kapa2, "X", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "Y", KAPA_LABEL_YM);
    KapaSendLabel (kapa2, "Chip Coordinates. Black = Raw Stars. Red = Ref Stars. Blue = Matched Stars", KAPA_LABEL_XP);

    // X vs Y by mag (ref)
    xVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    yVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    zVec = psVectorAlloc (refstars->n, PS_TYPE_F32);

    n = 0;
    for (int i = 0; i < refstars->n; i++) {
        pmAstromObj *ref = refstars->data[i];
        if (!isfinite(ref->Mag)) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = ref->chip->x;
        yVec->data.F32[n] = ref->chip->y;
        zVec->data.F32[n] = ref->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = KAPA_POINT_X;
    graphdata.style = KAPA_PLOT_POINTS;

    pmVisualTriplePlot (kapa2, &graphdata, xVec, yVec, zVec, false);

    // rescale the graph to include all points
    float xmin = graphdata.xmin;
    float ymin = graphdata.ymin;
    float xmax = graphdata.xmax;
    float ymax = graphdata.ymax;
    pmVisualScaleGraphdata(&graphdata, xVec, yVec, true);
    graphdata.xmin = PS_MIN(xmin, graphdata.xmin);
    graphdata.ymin = PS_MIN(ymin, graphdata.ymin);
    graphdata.xmax = PS_MAX(xmax, graphdata.xmax);
    graphdata.ymax = PS_MAX(ymax, graphdata.ymax);
    KapaSetLimits (kapa2, &graphdata);

    // bool plotTweak;
    // pmVisualAskUser(&plotTweak);

    // X vs Y by mag (raw)
    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    xVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    yVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    zVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    graphdata.style = KAPA_PLOT_POINTS;
    pmVisualTripleOverplot (kapa2, &graphdata, xVec, yVec, zVec, false);

    // overplot matched stars in blue
    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    xVec = psVectorAlloc (match->n, PS_TYPE_F32);
    yVec = psVectorAlloc (match->n, PS_TYPE_F32);
    zVec = psVectorAlloc (match->n, PS_TYPE_F32);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    fprintf (stderr, "plotting %d matched stars (raw = blue)\n", n);

    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.color = KapaColorByName ("blue");
    pmVisualTripleOverplot (kapa2, &graphdata, xVec, yVec, zVec, false);

    if (0) {
	graphdata.ptype = KAPA_POINT_X;
	graphdata.color = KapaColorByName ("green");
	n = 0;
	for (int i = 0; i < match->n; i++) {
	    pmAstromMatch *pair = match->data[i];
	    pmAstromObj *raw = rawstars->data[pair->raw];
	    pmAstromObj *ref = refstars->data[pair->ref];
	    if (raw->Mag < iMagMin) continue;
	    if (raw->Mag > iMagMax) continue;
	    if (ref->Mag < rMagMin) continue;
	    if (ref->Mag > rMagMax) continue;

	    xVec->data.F32[n] = ref->chip->x;
	    yVec->data.F32[n] = ref->chip->y;
	    zVec->data.F32[n] = ref->Mag;
	    n++;
	}
	xVec->n = yVec->n = zVec->n = n;
	fprintf (stderr, "plotting %d matched stars (ref = green)\n", n);
	pmVisualTripleOverplot (kapa2, &graphdata, xVec, yVec, zVec, false);
    }

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    // ***************************************
    // third window, pt1

    xVec = psVectorAlloc (match->n, PS_TYPE_F32);
    yVec = psVectorAlloc (match->n, PS_TYPE_F32);

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa3);

    // mag vs dX

    section.x  = 0.0;
    section.y  = 0.0;
    section.dx = 0.5;
    section.dy = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "s1");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->Mag;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        n++;
    }
    xVec->n = yVec->n = n;

    // rescale the graph to include all points
    pmVisualScaleGraphdata(&graphdata, xVec, yVec, true);
    KapaSetLimits (kapa3, &graphdata);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel(kapa1, "raw mag", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "dY", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = KAPA_POINT_TRIANGLE_OPEN;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.size = 2;

    KapaPrepPlot (kapa3, xVec->n, &graphdata);
    KapaPlotVector (kapa3, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa3, yVec->n, yVec->data.F32, "y");

    // ***************************************
    // third window, pt2

    section.x  = 0.5;
    section.y  = 0.0;
    section.dx = 0.5;
    section.dy = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "s2");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = ref->Mag;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        n++;
    }
    xVec->n = yVec->n = n;

    // rescale the graph to include all points
    pmVisualScaleGraphdata(&graphdata, xVec, yVec, true);
    KapaSetLimits (kapa3, &graphdata);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel(kapa1, "ref mag", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "dY", KAPA_LABEL_YM);

    KapaPrepPlot (kapa3, xVec->n, &graphdata);
    KapaPlotVector (kapa3, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa3, yVec->n, yVec->data.F32, "y");

    // ***************************************
    // third window, pt3

    section.x  = 0.0;
    section.y  = 0.5;
    section.dx = 0.5;
    section.dy = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "s3");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->Mag;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        n++;
    }
    xVec->n = yVec->n = n;

    // rescale the graph to include all points
    pmVisualScaleGraphdata(&graphdata, xVec, yVec, true);
    KapaSetLimits (kapa3, &graphdata);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel(kapa1, "raw mag", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "dX", KAPA_LABEL_YM);

    KapaPrepPlot (kapa3, xVec->n, &graphdata);
    KapaPlotVector (kapa3, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa3, yVec->n, yVec->data.F32, "y");

    // ***************************************
    // third window, pt4

    section.x  = 0.5;
    section.y  = 0.5;
    section.dx = 0.5;
    section.dy = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "s4");
    KapaSetSection (kapa3, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = ref->Mag;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        n++;
    }
    xVec->n = yVec->n = n;

    // rescale the graph to include all points
    pmVisualScaleGraphdata(&graphdata, xVec, yVec, true);
    KapaSetLimits (kapa3, &graphdata);
    KapaBox (kapa3, &graphdata);
    KapaSendLabel(kapa1, "ref mag", KAPA_LABEL_XM);
    KapaSendLabel(kapa1, "dX", KAPA_LABEL_YM);

    KapaPrepPlot (kapa3, xVec->n, &graphdata);
    KapaPlotVector (kapa3, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa3, yVec->n, yVec->data.F32, "y");

    return true;
}




# else

bool pmAstromVisualClose() { return true; }
bool pmAstromVisualPlotGridMatch (const psArray *raw, const psArray *ref, psImage *gridNP, double offsetX, double offsetY, double maxOffpix, double Scale, double Offset) { return true; }
bool pmAstromVisualPlotTweak (psVector *xHist, psVector *yHist, int xBin, int yBin) {return true;}
bool pmAstromVisualPlotLuminosityFunction (psVector *lnMag, psVector *Mag, pmLumFunc *lumFunc, pmLumFunc *rawFunc) {return true;}
bool pmAstromVisualPlotRawStars (psArray *rawstars, pmFPA *fpa, pmChip *chip, psMetadata *recipe) {return true;}
bool pmAstromVisualPlotRefStars (psArray *refstars, psMetadata *recipe) {return true;}
bool pmAstromVisualPlotRemoveClumps (psArray *input, psImage *count, int scale, float limit) {return true;}
bool pmAstromVisualPlotFixChips (pmFPAfile *input, psVector *xOld, psVector *yOld) {return true;}
bool pmAstromVisualPlotOneChipFit (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe) {return true;}
bool pmAstromVisualPlotAstromGuessCheck (psVector *cornerPo, psVector *cornerQo, psVector *cornerPn, psVector *cornerQn, psVector *cornerPd, psVector *cornerQd) {return true;}
bool pmAstromVisualPlotMosaicOneChip (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe) {return true;}
bool pmAstromVisualPlotCommonScale (pmFPA *fpa, psVector *oldScale) {return true;}
bool pmAstromVisualPlotMosaicMatches (psArray *rawstars, psArray *refstars, psArray *match, int iteration, psMetadata *recipe) {return true;}

# endif
