#include "psastroInternal.h"

bool psastroLuminosityFunctionPlot (psVector *lnMag, psVector *Mag,
                                    pmLumFunc *lumFunc, pmLumFunc *rawFunc) {
    //Set up graph window
    Graphdata graphdata;
    int kapa = pmKapaOpen(true);
    KapaSection section1 = {"s1", .05, .05, .9, .4};
    KapaSection section2 = {"s2", .05, .55, .9, .4};
    KapaSection section;
    if(rawFunc == NULL)
        section = section1;
    else
        section = section2;
    int DX = 1000, DY = 800;
    KapaResize(kapa, DX, DY);
    if (rawFunc == NULL)
        KapaClearPlots(kapa);
    KapaInitGraph(&graphdata);

    //Determine Plot Limits
    graphdata.xmin = 50;
    graphdata.ymin = 50;
    graphdata.xmax = -50;
    graphdata.ymax = -50;

    for(int i = 0; i < lnMag->n; i++) {
        graphdata.xmin = PS_MIN(graphdata.xmin,   Mag->data.F32[i]);
        graphdata.ymin = PS_MIN(graphdata.ymin, lnMag->data.F32[i]);
        graphdata.xmax = PS_MAX(graphdata.xmax,   Mag->data.F32[i]);
        graphdata.ymax = PS_MAX(graphdata.ymax, lnMag->data.F32[i]);
    }

    float range = graphdata.xmax - graphdata.xmin;
    graphdata.xmin -= .05 * range;
    graphdata.xmax += .05 * range;

    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymin -= .05 * range;
    graphdata.ymax += .05 * range;

    //Make a line for the fit
    float x[2] = {graphdata.xmin, graphdata.xmax};
    float y[2] = {lumFunc->offset + x[0] * lumFunc->slope,
                 lumFunc->offset + x[1] * lumFunc->slope};

    //Plot Data
    KapaSetSection(kapa, &section);
    KapaSetLimits(kapa, &graphdata);

    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "Magnitude", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "Log(N)", KAPA_LABEL_YM);
    if (rawFunc == NULL)
        KapaSendLabel (kapa, "Raw Star Luminosity Function", KAPA_LABEL_XP);
    else
        KapaSendLabel (kapa,
                       "Reference Star Luminosity Function, Shifted Raw Fit, and Cutoff",
                       KAPA_LABEL_XP);
    graphdata.color=KapaColorByName("black");
    graphdata.style = 1;
    KapaPrepPlot (kapa, lnMag->n, &graphdata);
    KapaPlotVector(kapa, lnMag->n,   Mag->data.F32, "x");
    KapaPlotVector(kapa, lnMag->n, lnMag->data.F32, "y");

    //Overplot fit
    graphdata.style=0;
    KapaPrepPlot(kapa,2,&graphdata);
    KapaPlotVector(kapa, 2, x, "x");
    KapaPlotVector(kapa, 2, y, "y");

    //If rawFunc was supplied, calculate the cutoff magnitude
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
        KapaPrepPlot(kapa, 2, &graphdata);
        KapaPlotVector(kapa, 2, x, "x");
        KapaPlotVector(kapa, 2, y, "y");
        KapaPrepPlot (kapa, 2, &graphdata);
        KapaPlotVector (kapa, 2, xraw, "x");
        KapaPlotVector (kapa, 2, yraw, "y");

        KapaPNG(kapa,"LumPlot.png");
        //char c;
        //fscanf(stdin, "%c", &c);
        //        pmKapaClose(kapa);
    }
    return true;
}
