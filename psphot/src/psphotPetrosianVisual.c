# include "psphotInternal.h"

// this function displays representative images as the psphot analysis progresses:
// 0 : image, 1 : variance
// 0 : backsub, 1 : variance, 2 : backgnd
// 0 : backsub, 1 : variance, 2 : signif
// (overlay peaks on images)
// (overlay footprints on images)
// (overlay moments on images)
// (overlay rough class on images)
// 0 : backsub, 1 : psfpos, 2: psfsub
// 0 : backsub, 1 : lin_resid, 2: psfsub

# if (HAVE_KAPA)
# include <kapa.h>

// functions used to visualize the analysis as it goes
// these are invoked by the -visual options

static int kapa = -1;
static int kapa2 = -1;

// if no valid data is supplied (NULL or n <- 0), leave limits as they were
bool pmVisualLimitsFromVectors (Graphdata *graphdata, psVector *xVec, psVector *yVec) {

    if (xVec && xVec->n > 0) {
	graphdata->xmin = graphdata->xmax = xVec->data.F32[0];
	for (int i = 1; i < xVec->n; i++) {
	    if (!isfinite(xVec->data.F32[i])) continue;
	    graphdata->xmin = PS_MIN (graphdata->xmin, xVec->data.F32[i]);
	    graphdata->xmax = PS_MAX (graphdata->xmax, xVec->data.F32[i]);
	}
	float range = graphdata->xmax - graphdata->xmin;
	graphdata->xmax += 0.05*range;
	graphdata->xmin -= 0.05*range;
    }
    if (yVec && yVec->n > 0) {
	graphdata->ymin = graphdata->ymax = yVec->data.F32[0];
	for (int i = 1; i < yVec->n; i++) {
	    if (!isfinite(yVec->data.F32[i])) continue;
	    graphdata->ymin = PS_MIN (graphdata->ymin, yVec->data.F32[i]);
	    graphdata->ymax = PS_MAX (graphdata->ymax, yVec->data.F32[i]);
	}
	float range = graphdata->ymax - graphdata->ymin;
	graphdata->ymax += 0.05*range;
	graphdata->ymin -= 0.05*range;
    }
    return true;
}

bool psphotPetrosianVisualProfileByAngle (psVector *radius, psVector *flux) {

    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.petro.byangle", 2)) return true;

    if (kapa2 == -1) {
        kapa2 = KapaOpenNamedSocket ("kapa", "psphot:plots");
        if (kapa2 == -1) {
            fprintf (stderr, "failure to open kapa; visual mode disabled\n");
            pmVisualSetVisual(false);
            return false;
        }
    }

    KapaClearPlots (kapa2);
    KapaInitGraph (&graphdata);
    KapaSetFont (kapa2, "courier", 14);

    pmVisualLimitsFromVectors (&graphdata, radius, flux);
    KapaSetLimits (kapa2, &graphdata);

    KapaBox (kapa2, &graphdata);
    KapaSendLabel (kapa2, "radius", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "flux", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 2;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    KapaPrepPlot (kapa2, radius->n, &graphdata);
    KapaPlotVector (kapa2, radius->n, radius->data.F32, "x");
    KapaPlotVector (kapa2, radius->n, flux->data.F32, "y");

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10];
    fprintf (stdout, "[c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Unable to read option");
    }
    return true;
}

bool psphotPetrosianVisualProfileRadii (psVector *radius, psVector *flux, psVector *radiusBin, psVector *fluxBin, float peakFlux, float RadiusRef) {

    float FluxRef = 500.0;

    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.petro.radii", 2)) return true;

    if (kapa == -1) {
        kapa = KapaOpenNamedSocket ("kapa", "psphot:plots");
        if (kapa == -1) {
            fprintf (stderr, "failure to open kapa; visual mode disabled\n");
            pmVisualSetVisual(false);
            return false;
        }
    }

    KapaClearPlots (kapa);
    KapaInitGraph (&graphdata);
    KapaSetFont (kapa, "courier", 14);

    graphdata.ymax = +1.05*peakFlux;
    graphdata.ymin = -0.05*peakFlux;
    pmVisualLimitsFromVectors (&graphdata, radius, NULL);
    KapaSetLimits (kapa, &graphdata);

    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "radius", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "flux", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 2;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    KapaPrepPlot (kapa, radius->n, &graphdata);
    KapaPlotVector (kapa, radius->n, radius->data.F32, "x");
    KapaPlotVector (kapa, radius->n, flux->data.F32, "y");

    // do this with log-r, log-flux?
    graphdata.color = KapaColorByName ("blue");
    graphdata.style = 2;
    graphdata.ptype = 2;
    graphdata.size = 2.0;
    KapaPrepPlot   (kapa, radiusBin->n, &graphdata);
    KapaPlotVector (kapa, radiusBin->n, radiusBin->data.F32, "x");
    KapaPlotVector (kapa, radiusBin->n, fluxBin->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.style = 2;
    graphdata.ptype = 7;
    graphdata.size = 3.0;
    KapaPrepPlot (kapa, 1, &graphdata);
    KapaPlotVector (kapa, 1, &RadiusRef, "x");
    KapaPlotVector (kapa, 1, &FluxRef, "y");

    fprintf (stderr, "radius: %f\n", RadiusRef);

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10];
    fprintf (stdout, "[c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Unable to read option");
    }
    return true;
}

bool psphotPetrosianVisualStats (psVector *radBin, psVector *fluxBin, 
				 psVector *refRadius, psVector *meanSB, 
				 psVector *petRatio, psVector *petRatioErr,
				 psVector *fluxSum, 
				 float petRadius, float ratioForRadius,
				 float petFlux, float radiusForFlux)
{
    Graphdata graphdata;
    KapaSection section;

    if (!pmVisualTestLevel("psphot.petro.stats", 2)) return true;

    if (kapa2 == -1) {
        kapa2 = KapaOpenNamedSocket ("kapa", "psphot:stats");
        if (kapa2 == -1) {
            fprintf (stderr, "failure to open kapa; visual mode disabled\n");
            pmVisualSetVisual(false);
            return false;
        }
    }

    KapaClearPlots (kapa2);
    KapaInitGraph (&graphdata);
    KapaSetFont (kapa2, "courier", 14);

    // radius vs flux
    // radius vs mean SB
    // radius vs petRatio

    // *** section 1: radius vs mean SB
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.00;
    section.name = psStringCopy ("meanSB");
    KapaSetSection (kapa2, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    pmVisualLimitsFromVectors (&graphdata, radBin, fluxBin);
    KapaSetLimits (kapa2, &graphdata);

    KapaBox (kapa2, &graphdata);
    KapaSendLabel (kapa2, "radius", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "mean SB", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 2;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    KapaPrepPlot (kapa2, radBin->n, &graphdata);
    KapaPlotVector (kapa2, radBin->n, radBin->data.F32, "x");
    KapaPlotVector (kapa2, radBin->n, fluxBin->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.style = 2;
    graphdata.ptype = 1;
    graphdata.size = 2.0;
    KapaPrepPlot (kapa2, refRadius->n, &graphdata);
    KapaPlotVector (kapa2, refRadius->n, refRadius->data.F32, "x");
    KapaPlotVector (kapa2, refRadius->n, meanSB->data.F32, "y");

    // *** section 2: radius vs petrosian ratio
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.33;
    section.name = psStringCopy ("ratio");
    KapaSetSection (kapa2, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.ymax = +1.05;
    graphdata.ymin = -0.05;
    pmVisualLimitsFromVectors (&graphdata, radBin, NULL);
    KapaSetLimits (kapa2, &graphdata);

    KapaBox (kapa2, &graphdata);
    KapaSendLabel (kapa2, "radius", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "ratio", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 2;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    graphdata.etype = 0x01;
    KapaPrepPlot (kapa2, refRadius->n, &graphdata);
    KapaPlotVector (kapa2, refRadius->n, refRadius->data.F32, "x");
    KapaPlotVector (kapa2, refRadius->n, petRatio->data.F32, "y");
    KapaPlotVector (kapa2, refRadius->n, petRatioErr->data.F32, "dym");
    KapaPlotVector (kapa2, refRadius->n, petRatioErr->data.F32, "dyp");
    graphdata.etype = 0;

    graphdata.color = KapaColorByName ("red");
    graphdata.style = 2;
    graphdata.ptype = 2;
    graphdata.size = 2.0;
    KapaPrepPlot   (kapa2, 1, &graphdata);
    KapaPlotVector (kapa2, 1, &petRadius, "x");
    KapaPlotVector (kapa2, 1, &ratioForRadius, "y");

    // *** section 3: radius vs integrated flux
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.66;
    section.name = psStringCopy ("flux");
    KapaSetSection (kapa2, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    pmVisualLimitsFromVectors (&graphdata, radBin, fluxSum);
    KapaSetLimits (kapa2, &graphdata);

    KapaBox (kapa2, &graphdata);
    KapaSendLabel (kapa2, "radius", KAPA_LABEL_XM);
    KapaSendLabel (kapa2, "integrated flux", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 2;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    KapaPrepPlot   (kapa2, refRadius->n, &graphdata);
    KapaPlotVector (kapa2, refRadius->n, refRadius->data.F32, "x");
    KapaPlotVector (kapa2, refRadius->n, fluxSum->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 2;
    graphdata.style = 2;
    graphdata.size = 2.0;
    KapaPrepPlot   (kapa2, 1, &graphdata);
    KapaPlotVector (kapa2, 1, &radiusForFlux, "x");
    KapaPlotVector (kapa2, 1, &petFlux, "y");

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10];
    fprintf (stdout, "[c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Unable to read option");
    }
    return true;
}

bool psphotPetrosianVisualEllipticalContour (pmSourceRadialFlux *radFlux, pmSourceExtendedPars *extpars) {

    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.petro.ellipse", 2)) return true;

    if (kapa == -1) {
        kapa = KapaOpenNamedSocket ("kapa", "psphot:plots");
        if (kapa == -1) {
            fprintf (stderr, "failure to open kapa; visual mode disabled\n");
            pmVisualSetVisual(false);
            return false;
        }
    }

    KapaClearPlots (kapa);
    KapaInitGraph (&graphdata);
    KapaSetFont (kapa, "courier", 14);

    psVector *theta = radFlux->theta;
    psVector *radius = radFlux->isophotalRadii;

    // find Rmin and Rmax for the initial guess
    float Rmin = radius->data.F32[0];
    float Rmax = radius->data.F32[0];

    psVector *Rx = psVectorAlloc(radius->n, PS_TYPE_F32);
    psVector *Ry = psVectorAlloc(radius->n, PS_TYPE_F32);

    for (int i = 0; i < theta->n; i++) {
	Rx->data.F32[i] = radius->data.F32[i]*cos(theta->data.F32[i]);
	Ry->data.F32[i] = radius->data.F32[i]*sin(theta->data.F32[i]);

	// check the radius range
	Rmin = MIN (Rmin, radius->data.F32[i]);
	Rmax = MAX (Rmax, radius->data.F32[i]);
    }	

    psVector *rx = psVectorAlloc(361, PS_TYPE_F32);
    psVector *ry = psVectorAlloc(361, PS_TYPE_F32);

    float epsilon = extpars->axes.minor / extpars->axes.major;

    for (int i = 0; i < 361; i++) {

	float alpha = PS_RAD_DEG * i;

	float cs_alpha = cos(alpha);
	float sn_alpha = sin(alpha);

	float cs_phi = cos(alpha - extpars->axes.theta);
	float sn_phi = sin(alpha - extpars->axes.theta);

	float r = 1.0 / sqrt(SQ(sn_phi) + SQ(epsilon*cs_phi));

	// generate the model fit here
	rx->data.F32[i] = extpars->axes.minor * cs_alpha * r;
	ry->data.F32[i] = extpars->axes.minor * sn_alpha * r;
    }	

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -1.1*Rmax;
    graphdata.ymin = -1.1*Rmax;
    graphdata.xmax = +1.1*Rmax;
    graphdata.ymax = +1.1*Rmax;
    KapaSetLimits (kapa, &graphdata);

    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "R_x", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "R_y", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("red");
    graphdata.style = 2;
    graphdata.ptype = 2;
    graphdata.size = 1.0;
    KapaPrepPlot (kapa, Rx->n, &graphdata);
    KapaPlotVector (kapa, Rx->n, Rx->data.F32, "x");
    KapaPlotVector (kapa, Rx->n, Ry->data.F32, "y");

    graphdata.color = KapaColorByName ("black");
    graphdata.style = 0;
    graphdata.ptype = 0;
    graphdata.size = 1.0;
    KapaPrepPlot (kapa, rx->n, &graphdata);
    KapaPlotVector (kapa, rx->n, rx->data.F32, "x");
    KapaPlotVector (kapa, rx->n, ry->data.F32, "y");

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10];
    fprintf (stdout, "[c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Unable to read option");
    }
    return true;
}

# endif
