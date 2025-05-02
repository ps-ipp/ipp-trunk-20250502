#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"

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
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmSourceVisual.h"

#if (HAVE_KAPA)
#include <kapa.h>
#include "pmVisual.h"
#include "pmVisualUtils.h"

// functions used to visualize the analysis as it goes
// these are invoked by the -visual options

static int kapa1 = -1;
static int kapa2 = -1;
static bool plotPSF = true;
// static int kapa3 = -1;

bool pmSourceVisualClose() {
    if (kapa1 != -1)
        KapaClose(kapa1);
    return true;
}

bool pmSourcePlotPoints3D (int myKapa, Graphdata *graphdata, psVector *xn, psVector *yn, psVector *zn, float theta, float phi);

bool pmSourceVisualPlotPSFMetric (pmPSFtry *psfTry) {

    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.psf.metric", 2)) return true;
    if (!pmVisualInitWindow (&kapa1, "pmSource:plots")) return false;

    KapaClearSections (kapa1);
    KapaInitGraph (&graphdata);

    psVector *x = psVectorAllocEmpty (psfTry->sources->n, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty (psfTry->sources->n, PS_TYPE_F32);
    psVector *dy = psVectorAllocEmpty(psfTry->sources->n, PS_TYPE_F32);

    graphdata.xmin = +32.0;
    graphdata.xmax = -32.0;
    graphdata.ymin = +32.0;
    graphdata.ymax = -32.0;

    // construct the plot vectors
    int n = 0;
    for (int i = 0; i < psfTry->sources->n; i++) {
	if (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) continue;
        x->data.F32[n] = psfTry->fitMag->data.F32[i];
	y->data.F32[n] = psfTry->metric->data.F32[i];
        dy->data.F32[n] = psfTry->metricErr->data.F32[i];
        graphdata.xmin = PS_MIN(graphdata.xmin, x->data.F32[n]);
        graphdata.xmax = PS_MAX(graphdata.xmax, x->data.F32[n]);
        graphdata.ymin = PS_MIN(graphdata.ymin, y->data.F32[n]);
        graphdata.ymax = PS_MAX(graphdata.ymax, y->data.F32[n]);
	n++;
    }
    x->n = y->n = dy->n = n;

    float range;
    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;
    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    // better choice for range?
    // graphdata.xmin = -17.0;
    // graphdata.xmax =  -9.0;
    graphdata.ymin = -0.51;
    graphdata.ymax = +0.51;

    KapaSetLimits (kapa1, &graphdata);

    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Ap Mag - PSF Mag", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.etype |= 0x01;

    KapaPrepPlot (kapa1, n, &graphdata);
    KapaPlotVector (kapa1, n, x->data.F32, "x");
    KapaPlotVector (kapa1, n, y->data.F32, "y");
    KapaPlotVector (kapa1, n, dy->data.F32, "dym");
    KapaPlotVector (kapa1, n, dy->data.F32, "dyp");

    psFree (x);
    psFree (y);
    psFree (dy);

    pmVisualAskUser(NULL);
    return true;
}

bool pmSourceVisualPlotPSFMetricSubpix (pmPSFtry *psfTry) {

    KapaSection section;  // put the positive profile in one and the residuals in another?
    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.psf.subpix", 3)) return true;
    if (!pmVisualInitWindow (&kapa1, "pmSource:plots")) return false;

    KapaClearSections (kapa1);
    KapaInitGraph (&graphdata);
    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    int n;
    float range;
    psVector *x = psVectorAllocEmpty (psfTry->sources->n, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty (psfTry->sources->n, PS_TYPE_F32);
    psVector *dy = psVectorAllocEmpty(psfTry->sources->n, PS_TYPE_F32);

    // section a: fractional-x pixel
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    graphdata.xmin = +32.0;
    graphdata.xmax = -32.0;
    graphdata.ymin = +32.0;
    graphdata.ymax = -32.0;

    // construct the plot vectors
    n = 0;
    for (int i = 0; i < psfTry->sources->n; i++) {
	if (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) continue;

        pmSource *source = psfTry->sources->data[i];
        x->data.F32[n] = source->modelEXT->params->data.F32[PM_PAR_XPOS] - (int)source->modelEXT->params->data.F32[PM_PAR_XPOS];

	y->data.F32[n] = psfTry->metric->data.F32[i];
        dy->data.F32[n] = psfTry->metricErr->data.F32[i];
        graphdata.xmin = PS_MIN(graphdata.xmin, x->data.F32[n]);
        graphdata.xmax = PS_MAX(graphdata.xmax, x->data.F32[n]);
        graphdata.ymin = PS_MIN(graphdata.ymin, y->data.F32[n]);
        graphdata.ymax = PS_MAX(graphdata.ymax, y->data.F32[n]);
	n++;
    }
    x->n = y->n = dy->n = n;

    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;
    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    // better choice for range?
    // graphdata.xmin = -17.0;
    // graphdata.xmax =  -9.0;
    graphdata.ymin = -0.51;
    graphdata.ymax = +0.51;

    KapaSetLimits (kapa1, &graphdata);

    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Ap Mag - PSF Mag", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.etype |= 0x01;

    KapaPrepPlot (kapa1, n, &graphdata);
    KapaPlotVector (kapa1, n, x->data.F32, "x");
    KapaPlotVector (kapa1, n, y->data.F32, "y");
    KapaPlotVector (kapa1, n, dy->data.F32, "dym");
    KapaPlotVector (kapa1, n, dy->data.F32, "dyp");

    // *** section b: fractional-x pixel
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    graphdata.xmin = +32.0;
    graphdata.xmax = -32.0;
    graphdata.ymin = +32.0;
    graphdata.ymax = -32.0;

    // construct the plot vectors
    n = 0;
    for (int i = 0; i < psfTry->sources->n; i++) {
	if (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) continue;

        pmSource *source = psfTry->sources->data[i];
        x->data.F32[n] = source->modelEXT->params->data.F32[PM_PAR_YPOS] - (int)source->modelEXT->params->data.F32[PM_PAR_YPOS];

	y->data.F32[n] = psfTry->metric->data.F32[i];
        dy->data.F32[n] = psfTry->metricErr->data.F32[i];
        graphdata.xmin = PS_MIN(graphdata.xmin, x->data.F32[n]);
        graphdata.xmax = PS_MAX(graphdata.xmax, x->data.F32[n]);
        graphdata.ymin = PS_MIN(graphdata.ymin, y->data.F32[n]);
        graphdata.ymax = PS_MAX(graphdata.ymax, y->data.F32[n]);
	n++;
    }
    x->n = y->n = dy->n = n;

    range = graphdata.xmax - graphdata.xmin;
    graphdata.xmax += 0.05*range;
    graphdata.xmin -= 0.05*range;
    range = graphdata.ymax - graphdata.ymin;
    graphdata.ymax += 0.05*range;
    graphdata.ymin -= 0.05*range;

    // better choice for range?
    // graphdata.xmin = -17.0;
    // graphdata.xmax =  -9.0;
    graphdata.ymin = -0.51;
    graphdata.ymax = +0.51;

    KapaSetLimits (kapa1, &graphdata);

    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Ap Mag - PSF Mag", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 0.5;
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.etype |= 0x01;

    KapaPrepPlot (kapa1, n, &graphdata);
    KapaPlotVector (kapa1, n, x->data.F32, "x");
    KapaPlotVector (kapa1, n, y->data.F32, "y");
    KapaPlotVector (kapa1, n, dy->data.F32, "dym");
    KapaPlotVector (kapa1, n, dy->data.F32, "dyp");

    psFree (x);
    psFree (y);
    psFree (dy);

    pmVisualAskUser(NULL);
    return true;
}

// to see the structure of the psf model, place the sources in a fake image 1/10th the size
// at their appropriate relative location. later sources stomp on earlier sources
bool pmSourceVisualShowModelFits (pmPSF *psf, psArray *sources, psImageMaskType maskVal) {

    if (!pmVisualTestLevel("psphot.psf.fits", 2)) return true;
    if (!pmVisualInitWindow (&kapa2, "pmSource:images")) return false;

    // create images 1/10 scale:
    psImage *image = psImageAlloc (0.1*psf->fieldNx, 0.1*psf->fieldNy, PS_TYPE_F32);
    psImage *model = psImageAlloc (0.1*psf->fieldNx, 0.1*psf->fieldNy, PS_TYPE_F32);
    psImage *resid = psImageAlloc (0.1*psf->fieldNx, 0.1*psf->fieldNy, PS_TYPE_F32);
    psImageInit (image, 0.0);
    psImageInit (model, 0.0);
    psImageInit (resid, 0.0);

    for (int i = sources->n - 1; i >= 0; i--) {
	pmSource *source = sources->data[i];
	if (!source) continue;
	if (!source->pixels) continue;

	pmSourceCacheModel (source, maskVal);
	if (!source->modelFlux) continue;

	pmModel *srcModel = pmSourceGetModel (NULL, source);
	if (!model) continue;

	float norm = srcModel->params->data.F32[PM_PAR_I0];

	int Xo = 0.1*srcModel->params->data.F32[PM_PAR_XPOS];
	int Yo = 0.1*srcModel->params->data.F32[PM_PAR_YPOS];

	// insert source pixels in the image at 1/10th offset
	for (int iy = 0; iy < source->pixels->numRows; iy++) {
	    int jy = iy + Yo;
	    if (jy >= image->numRows) continue;
	    for (int ix = 0; ix < source->pixels->numCols; ix++) {
		int jx = ix + Xo;
		if (jx >= image->numCols) continue;
		if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) continue;
		if (source->modelFlux->data.F32[iy][ix] < 0.001) continue;
		image->data.F32[jy][jx] = source->pixels->data.F32[iy][ix];
		model->data.F32[jy][jx] = source->modelFlux->data.F32[iy][ix];
		resid->data.F32[jy][jx] = source->pixels->data.F32[iy][ix] - norm*source->modelFlux->data.F32[iy][ix];
	    }
	}
    }

    // KapaClearSections (kapa2);
    pmVisualScaleImage (kapa2, image, "image", 0, true);
    pmVisualScaleImage (kapa2, model, "model", 1, true);
    pmVisualScaleImage (kapa2, resid, "resid", 2, true);

# ifdef DEBUG
    { 
	psFits *fits = psFitsOpen ("image.fits", "w");
	psFitsWriteImage (fits, NULL, image, 0, NULL);
	psFitsClose (fits);
	fits = psFitsOpen ("model.fits", "w");
	psFitsWriteImage (fits, NULL, model, 0, NULL);
	psFitsClose (fits);
	fits = psFitsOpen ("resid.fits", "w");
	psFitsWriteImage (fits, NULL, resid, 0, NULL);
	psFitsClose (fits);
    }
# endif

    psFree (image);
    psFree (model);
    psFree (resid);

    pmVisualAskUser(NULL);
    return true;
}

bool pmSourceVisualShowModelFit (pmSource *source) {

    if (!pmVisualTestLevel("psphot.psf.fitresid", 2)) return true;

    if (!source->pixels) return false;
    if (!source->modelFlux) return false;
    if (!pmVisualInitWindow (&kapa2, "pmSource:images")) return false;

    // KapaClearSections (kapa2);
    pmVisualScaleImage (kapa2, source->pixels, "source", 0, false);
    pmVisualScaleImage (kapa2, source->modelFlux, "model", 1, false);

    pmModel *model = pmSourceGetModel (NULL, source);
    float norm = model->params->data.F32[PM_PAR_I0];

    psImage *resid = psImageAlloc (source->pixels->numCols, source->pixels->numRows, PS_TYPE_F32);
    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
		resid->data.F32[iy][ix] = NAN;
		continue;
	    }
	    resid->data.F32[iy][ix] = source->pixels->data.F32[iy][ix] - norm*source->modelFlux->data.F32[iy][ix];
	}
    }
    pmVisualScaleImage (kapa2, resid, "resid", 2, false);

    psFree (resid);

    pmVisualAskUser(NULL);
    return true;
}

bool pmSourceVisualPSFModelResid (pmTrend2D *trend, psVector *x, psVector *y, psVector *param, psVector *mask) {

    KapaSection section;  // put the positive profile in one and the residuals in another?

    Graphdata graphdata;

    if (!plotPSF) return true;
    if (!pmVisualTestLevel("psphot.psf.resid", 2)) return true;
    if (!pmVisualInitWindow (&kapa1, "pmSource:plots")) return false;

    KapaClearPlots (kapa1);
    KapaInitGraph (&graphdata);
    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    float Xmin = +1e32;
    float Xmax = -1e32;
    float Ymin = +1e32;
    float Ymax = -1e32;
    float Fmin = +1e32;
    float Fmax = -1e32;

    psVector *resid = psVectorAlloc (x->n, PS_TYPE_F32);
    psVector *model = psVectorAlloc (x->n, PS_TYPE_F32);

    psVector *xm = psVectorAlloc (x->n, PS_TYPE_F32);
    psVector *ym = psVectorAlloc (x->n, PS_TYPE_F32);
    psVector *Fm = psVectorAlloc (x->n, PS_TYPE_F32);

    int n = 0;
    for (int i = 0; i < x->n; i++) {
        model->data.F32[i] = pmTrend2DEval (trend, x->data.F32[i], y->data.F32[i]);
        resid->data.F32[i] = param->data.F32[i] - model->data.F32[i];
        if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
        Xmin = PS_MIN (Xmin, x->data.F32[i]);
        Xmax = PS_MAX (Xmax, x->data.F32[i]);
        Ymin = PS_MIN (Ymin, y->data.F32[i]);
        Ymax = PS_MAX (Ymax, y->data.F32[i]);
        Fmin = PS_MIN (Fmin, param->data.F32[i]);
        Fmax = PS_MAX (Fmax, param->data.F32[i]);
	xm->data.F32[n] = x->data.F32[i];
	ym->data.F32[n] = y->data.F32[i];
	Fm->data.F32[n] = param->data.F32[i];
	n++;
    }
    xm->n = ym->n = Fm->n = n;

    // view 1 on resid
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Xmin;
    graphdata.xmax = Xmax;
    graphdata.ymin = Fmin;
    graphdata.ymax = Fmax;

    { 
	float range;
	range = graphdata.xmax - graphdata.xmin;
	graphdata.xmax += 0.05*range;
	graphdata.xmin -= 0.05*range;
	range = graphdata.ymax - graphdata.ymin;
	graphdata.ymax += 0.05*range;
	graphdata.ymin -= 0.05*range;
    }

    KapaSetLimits (kapa1, &graphdata);
    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "X (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Model Param", KAPA_LABEL_YM);

    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 1.0;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa1,   x->n, &graphdata);
    KapaPlotVector (kapa1, x->n, x->data.F32, "x");
    KapaPlotVector (kapa1, x->n, param->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    KapaPrepPlot (kapa1,   xm->n, &graphdata);
    KapaPlotVector (kapa1, xm->n, xm->data.F32, "x");
    KapaPlotVector (kapa1, xm->n, Fm->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    KapaPrepPlot (kapa1,   x->n, &graphdata);
    KapaPlotVector (kapa1, x->n, x->data.F32, "x");
    KapaPlotVector (kapa1, x->n, model->data.F32, "y");

    // view 2 on resid
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa1, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Ymin;
    graphdata.xmax = Ymax;
    graphdata.ymin = Fmin;
    graphdata.ymax = Fmax;
    { 
	float range;
	range = graphdata.xmax - graphdata.xmin;
	graphdata.xmax += 0.05*range;
	graphdata.xmin -= 0.05*range;
	range = graphdata.ymax - graphdata.ymin;
	graphdata.ymax += 0.05*range;
	graphdata.ymin -= 0.05*range;
    }

    KapaSetLimits (kapa1, &graphdata);
    KapaSetFont (kapa1, "helvetica", 14);
    KapaBox (kapa1, &graphdata);
    KapaSendLabel (kapa1, "Y (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (kapa1, "Model Param", KAPA_LABEL_YM);

    graphdata.ptype = KAPA_POINT_CROSS;
    graphdata.size = 1.0;
    graphdata.style = KAPA_PLOT_POINTS;
    KapaPrepPlot (kapa1,   y->n, &graphdata);
    KapaPlotVector (kapa1, y->n, y->data.F32, "x");
    KapaPlotVector (kapa1, y->n, param->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    KapaPrepPlot (kapa1,   xm->n, &graphdata);
    KapaPlotVector (kapa1, xm->n, ym->data.F32, "x");
    KapaPlotVector (kapa1, xm->n, Fm->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = KAPA_POINT_BOX_OPEN;
    KapaPrepPlot (kapa1,   y->n, &graphdata);
    KapaPlotVector (kapa1, y->n, y->data.F32, "x");
    KapaPlotVector (kapa1, y->n, model->data.F32, "y");

    psFree (xm);
    psFree (ym);
    psFree (Fm);

    psFree (resid);
    psFree (model);

    bool dumpData = false;

    // pause and wait for user input:
    // continue, save (provide name), ??
retry:
    pmVisualAskUserOrDump(&plotPSF, &dumpData);
    if (dumpData) {
      char name[128];
      fprintf (stderr, "filename: ");
      int status = fscanf (stdin, "%127s", name);
      if (status != 1) {
	fprintf (stderr, "odd response\n");
	goto retry;
      }

      FILE *f = fopen (name, "w");
      if (!f) {
	fprintf (stderr, "cannot open %s for output\n", name);
	goto retry;
      }
      for (int i = 0; i < x->n; i++) {
        float vModel = pmTrend2DEval (trend, x->data.F32[i], y->data.F32[i]);
	fprintf (f, "%f %f %f %f %d\n", x->data.F32[i], y->data.F32[i], param->data.F32[i], vModel, mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
      }
      fclose (f);
      goto retry;
    }

    return true;
}

// Somewhat broken 3D plotting function (was used by pmSourceVisualPSFModelResid, but not anymore)
bool pmSourcePlotPoints3D (int myKapa, Graphdata *graphdata, psVector *xn, psVector *yn, psVector *zn, float theta, float phi) {

    return true;

    psVector *xv = psVectorAlloc (PS_MAX(6, 2*xn->n), PS_TYPE_F32);
    psVector *yv = psVectorAlloc (PS_MAX(6, 2*xn->n), PS_TYPE_F32);
    psVector *zv = psVectorAlloc (PS_MAX(6, 2*xn->n), PS_TYPE_F32);

    graphdata->xmin = +1e32;
    graphdata->xmax = -1e32;
    graphdata->ymin = +1e32;
    graphdata->ymax = -1e32;

    for (int i = 0; i < xn->n; i++) {
        xv->data.F32[2*i+0] = +xn->data.F32[i]*cos(theta) + yn->data.F32[i]*sin(theta)*cos(phi) + zn->data.F32[i]*sin(theta)*sin(phi);
        yv->data.F32[2*i+0] = -xn->data.F32[i]*sin(theta) + yn->data.F32[i]*cos(theta)*cos(phi) + zn->data.F32[i]*cos(theta)*sin(phi);
        zv->data.F32[2*i+0] = -yn->data.F32[i]*sin(phi)   + zn->data.F32[i]*cos(phi);
        xv->data.F32[2*i+1] = +xn->data.F32[i]*cos(theta) + yn->data.F32[i]*sin(theta)*cos(phi);
        yv->data.F32[2*i+1] = -xn->data.F32[i]*sin(theta) + yn->data.F32[i]*cos(theta)*cos(phi);
        zv->data.F32[2*i+1] = -yn->data.F32[i]*sin(phi);
        graphdata->xmin = PS_MIN(graphdata->xmin, xv->data.F32[2*i+0]);
        graphdata->xmax = PS_MAX(graphdata->xmax, xv->data.F32[2*i+0]);
        graphdata->ymin = PS_MIN(graphdata->ymin, zv->data.F32[2*i+0]);
        graphdata->ymax = PS_MAX(graphdata->ymax, zv->data.F32[2*i+0]);
        graphdata->xmin = PS_MIN(graphdata->xmin, xv->data.F32[2*i+1]);
        graphdata->xmax = PS_MAX(graphdata->xmax, xv->data.F32[2*i+1]);
        graphdata->ymin = PS_MIN(graphdata->ymin, zv->data.F32[2*i+1]);
        graphdata->ymax = PS_MAX(graphdata->ymax, zv->data.F32[2*i+1]);
    }
    xv->n = xn->n;

    // examine sources to set data range
    KapaSetLimits (myKapa, graphdata);

    graphdata->color = KapaColorByName ("black");
    graphdata->ptype = KAPA_POINT_PAIR_CONNECT;
    graphdata->size = 0.5;
    graphdata->style = KAPA_PLOT_POINTS;
    KapaPrepPlot (myKapa, xv->n, graphdata);
    KapaPlotVector (myKapa, xv->n, xv->data.F32, "x");
    KapaPlotVector (myKapa, xv->n, zv->data.F32, "y");

    graphdata->color = KapaColorByName ("blue");
    graphdata->ptype = KAPA_POINT_BOX_OPEN;
    graphdata->size = 1.5;
    graphdata->style = KAPA_PLOT_POINTS;
    KapaPrepPlot (myKapa, xv->n, graphdata);
    KapaPlotVector (myKapa, xv->n, xv->data.F32, "x");
    KapaPlotVector (myKapa, xv->n, zv->data.F32, "y");

    xv->n = 6;

    // set the three axis lines
    xv->data.F32[0] = +0.0*cos(theta) + 0.0*sin(theta)*cos(phi) + 0.0*sin(theta)*sin(phi);
    yv->data.F32[0] = -0.0*sin(theta) + 0.0*cos(theta)*cos(phi) + 0.0*cos(theta)*sin(phi);
    zv->data.F32[0] =                 - 0.0*sin(phi)            + 0.0*cos(phi);
    xv->data.F32[1] = +1.0*cos(theta) + 0.0*sin(theta)*cos(phi);
    yv->data.F32[1] = -1.0*sin(theta) + 0.0*cos(theta)*cos(phi);
    zv->data.F32[1] =                 - 0.0*sin(phi)            + 0.0*cos(phi);

    xv->data.F32[2] = +0.0*cos(theta) + 0.0*sin(theta)*cos(phi) + 0.0*sin(theta)*sin(phi);
    yv->data.F32[2] = -0.0*sin(theta) + 0.0*cos(theta)*cos(phi) + 0.0*cos(theta)*sin(phi);
    zv->data.F32[2] =                 - 0.0*sin(phi)            + 0.0*cos(phi);
    xv->data.F32[3] = +0.0*cos(theta) + 1.0*sin(theta)*cos(phi);
    yv->data.F32[3] = -0.0*sin(theta) + 1.0*cos(theta)*cos(phi);
    zv->data.F32[3] =                 - 1.0*sin(phi)            + 0.0*cos(phi);

    xv->data.F32[4] = +0.0*cos(theta) + 0.0*sin(theta)*cos(phi) + 1.0*sin(theta)*sin(phi);
    yv->data.F32[4] = -0.0*sin(theta) + 0.0*cos(theta)*cos(phi) + 1.0*cos(theta)*sin(phi);
    zv->data.F32[4] =                 - 0.0*sin(phi)            + 1.0*cos(phi);
    xv->data.F32[5] = +0.0*cos(theta) + 0.0*sin(theta)*cos(phi);
    yv->data.F32[5] = -0.0*sin(theta) + 0.0*cos(theta)*cos(phi);
    zv->data.F32[5] =                 - 0.0*sin(phi)            + 0.0*cos(phi);

    graphdata->color = KapaColorByName ("red");
    graphdata->ptype = KAPA_POINT_PAIR_CONNECT;
    graphdata->size = 0.5;
    graphdata->style = KAPA_PLOT_POINTS;
    KapaPrepPlot (myKapa, xv->n, graphdata);
    KapaPlotVector (myKapa, xv->n, xv->data.F32, "x");
    KapaPlotVector (myKapa, xv->n, zv->data.F32, "y");

    psFree (xv);
    psFree (yv);
    psFree (zv);

    return true;
}

#else

bool pmSourceSetVisual(bool mode)
{
    return true;
}

bool pmSourceVisualPSFModelResid(pmTrend2D *trend, psVector *x, psVector *y, psVector *param, psVector *mask)
{
    return true;
}

#endif
