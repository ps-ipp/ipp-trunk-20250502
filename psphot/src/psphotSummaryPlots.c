# include "psphotInternal.h"

// the top portion of this file defines plotting functions which use kapa for plotting.
// if kapa is not available, these functions are defined in the bottom portion as stubs
// which perform NOP and return false (XXX should this be true??)

// this variable is defined in psmodules.h if ohana-config is found
# if (HAVE_KAPA)

# include <kapa.h>

// plot the sx, sy moments plane (faint and bright sources)
bool psphotPlotMoments (pmConfig *config, pmFPAview *view, psArray *sources) {

    // select model pixels (from output background model file, or create internal file)
    pmFPAfile *file = psMetadataLookupPtr (NULL, config->files, "PSPHOT.MOMENT.PLT");
    if (file == NULL) {
	psLogMsg ("psphot", 3, "skipping moments plot");
	return false;
    }

    // pmFPAfileOpen defers disk I/O for KAPA files: just get the correct name
    pmFPAfileOpen (file, view, config);

    Graphdata graphdata;

    psLogMsg ("psphot", 3, "creating moments plot");

    // XXX get the 'showWindow' option from the recipes somewhere
    int kapa = pmKapaOpen (false);
    if (kapa == -1) {
	psError(PSPHOT_ERR_UNKNOWN, true, "failure to open kapa");
	return false;
    }

    KapaResize (kapa, 500, 500);
    KapaInitGraph (&graphdata);

    // examine sources to set data range
    graphdata.xmin = -0.05;
    graphdata.ymin = -0.05;
    graphdata.xmax = +2.05;
    graphdata.ymax = +2.05;
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
	if (source->moments == NULL) continue;
    
	xFaint->data.F32[nF] = source->moments->Mxx;
	yFaint->data.F32[nF] = source->moments->Myy;
	nF++;
    
	// XXX make this a user-defined cutoff
	if (source->moments->SN < 25) continue;

	xBright->data.F32[nB] = source->moments->Mxx;
	yBright->data.F32[nB] = source->moments->Myy;
	nB++;
    }
    xFaint->n = nF;
    yFaint->n = nF;

    xBright->n = nB;
    yBright->n = nB;
  
    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (kapa, nF, &graphdata);
    KapaPlotVector (kapa, nF, xFaint->data.F32, "x");
    KapaPlotVector (kapa, nF, yFaint->data.F32, "y");
  
    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (kapa, nB, &graphdata);
    KapaPlotVector (kapa, nB, xBright->data.F32, "x");
    KapaPlotVector (kapa, nB, yBright->data.F32, "y");

    psLogMsg ("psphot", 3, "saving plot to %s", file->filename);
    KapaPNG (kapa, file->filename);

    psFree (xBright);
    psFree (yBright);
    psFree (xFaint);
    psFree (yFaint);

    return true;
}

// plot the sx, sy, sxy as vector field, 
// plot the PSF measured sx, sy, sxy as vector field
// pull the sources from the config / file?
bool psphotPlotPSFModel (pmConfig *config, pmFPAview *view, psArray *sources) {

    // select model pixels (from output background model file, or create internal file)
    pmFPAfile *file = psMetadataLookupPtr (NULL, config->files, "PSPHOT.PSFMODEL.PLT");
    if (file == NULL) {
	psLogMsg ("psphot", 3, "skipping psf model plot");
	return false;
    }

    // pmFPAfileOpen defers disk I/O for KAPA files: just get the correct name
    pmFPAfileOpen (file, view, config);

    Graphdata graphdata;

    psLogMsg ("psphot", 3, "creating psf model plot");

    int kapa = pmKapaOpen (false);
    if (kapa == -1) {
	psError(PSPHOT_ERR_UNKNOWN, true, "failure to open kapa");
	return false;
    }

    // XXX make the aspect-ratio match the image
    KapaResize (kapa, 800, 800);
    KapaInitGraph (&graphdata);

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
    float dx = 0;
    float dy = 0;
    float scale = 10;
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (source->moments == NULL) continue;
	if (source->moments->SN < 25) continue;
        if (source->type != PM_SOURCE_TYPE_STAR) continue;
    
	pmModel *model = source->modelPSF;
        if (model == NULL) continue;

	psF32 *PAR = model->params->data.F32;

	psEllipseMoments moments;
	moments.x2 = source->moments->Mxx;
	moments.xy = source->moments->Mxy;
	moments.y2 = source->moments->Myy;

	psEllipseShape shape;
	shape.sx  = PAR[PM_PAR_SXX] / sqrt(2.0);
	shape.sy  = PAR[PM_PAR_SYY] / sqrt(2.0);
	shape.sxy = PAR[PM_PAR_SXY];

	// force the axis ratio to be < 20.0
	psEllipseAxes axes_mnt = psEllipseMomentsToAxes (moments, 20.0);
	psEllipseAxes axes_psf = psEllipseShapeToAxes (shape, 20.0);

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
    KapaSendLabel (kapa, "vector is major axis (scale by 20) : black are moments, blue are psf model, red is psf minor axis", KAPA_LABEL_XP);
	       
    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 100;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (kapa, nMNT, &graphdata);
    KapaPlotVector (kapa, nMNT, xMNT->data.F32, "x");
    KapaPlotVector (kapa, nMNT, yMNT->data.F32, "y");
  
    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 100;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (kapa, nPSF, &graphdata);
    KapaPlotVector (kapa, nPSF, xPSF->data.F32, "x");
    KapaPlotVector (kapa, nPSF, yPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 100;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (kapa, nMIN, &graphdata);
    KapaPlotVector (kapa, nMIN, xMIN->data.F32, "x");
    KapaPlotVector (kapa, nMIN, yMIN->data.F32, "y");

    psLogMsg ("psphot", 3, "saving plot to %s", file->filename);
    KapaPNG (kapa, file->filename);

    psFree (xMNT);
    psFree (yMNT);
    psFree (xPSF);
    psFree (yPSF);
    psFree (xMIN);
    psFree (yMIN);

    return true;
}

# else

bool psphotPlotMoments (pmConfig *config, pmFPAview *view, psArray *sources) {
    psLogMsg ("psphot", 3, "skipping moments plot");
    return true;
}

bool psphotPlotPSFModel (pmConfig *config, pmFPAview *view, psArray *sources) {
    psLogMsg ("psphot", 3, "skipping psf model plot");
    return true;
}

# endif
