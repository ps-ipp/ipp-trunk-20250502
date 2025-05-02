# include "psphotInternal.h"

// this variable is defined in psmodules.h if ohana-config is found
# if (HAVE_KAPA)

# include <kapa.h>

static int nCount = 0;

bool psphotRadialPlot (int *kapa, const char *filename, pmSource *source) {

    Graphdata graphdata;

    // only plot 50 stars for now...
    if (nCount > 00) {
	if (*kapa != 0) {
	    KapaClose (*kapa);
	    *kapa = 0;
	}
	return true;
    }

    // XXX get the 'showWindow' option from the recipes somewhere
    // XXX 'showWindow = false' is broken
    if (*kapa == 0) {
	*kapa = pmKapaOpen (false);
	KapaResize (*kapa, 500, 500);
	unlink (filename);
    }
    if (*kapa == -1) {
	psError(PSPHOT_ERR_UNKNOWN, true, "failure to open kapa");
	return false;
    }

    KapaInitGraph (&graphdata);
    KapaClearPlots (*kapa);

    // examine sources to set data range
    graphdata.xmin =  -0.05;
    graphdata.xmax = +30.05;
    graphdata.ymin = -0.05;
    graphdata.ymax = +5.05;
    KapaSetLimits (*kapa, &graphdata);
  
    KapaSetFont (*kapa, "helvetica", 14);
    KapaBox (*kapa, &graphdata);
    KapaSendLabel (*kapa, "radius (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (*kapa, "log flux (counts)", KAPA_LABEL_YM);
	       
    int nPts = source->pixels->numRows * source->pixels->numCols;
    psVector *rg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *fg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *rb = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *fb = psVectorAllocEmpty (nPts, PS_TYPE_F32);

    int ng = 0;
    int nb = 0;
    float Xo = source->modelPSF->params->data.F32[PM_PAR_XPOS] - source->pixels->col0;
    float Yo = source->modelPSF->params->data.F32[PM_PAR_YPOS] - source->pixels->row0;
    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
		rb->data.F32[nb] = hypot (ix - Xo, iy - Yo) ;
		fb->data.F32[nb] = log10(source->pixels->data.F32[iy][ix]);
		nb++;
	    } else {
		rg->data.F32[ng] = hypot (ix - Xo, iy - Yo) ;
		fg->data.F32[ng] = log10(source->pixels->data.F32[iy][ix]);
		ng++;
	    }
	}
    }
  
    // set the plot range here based on lflux, radius

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (*kapa, ng, &graphdata);
    KapaPlotVector (*kapa, ng, rg->data.F32, "x");
    KapaPlotVector (*kapa, ng, fg->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (*kapa, nb, &graphdata);
    KapaPlotVector (*kapa, nb, rb->data.F32, "x");
    KapaPlotVector (*kapa, nb, fb->data.F32, "y");
  
    psLogMsg ("psphot", 3, "saving plot to %s", filename);

    char pagename[16];
    sprintf (pagename, "%02d", nCount);
    if (nCount == 0) {
	KiiPS (*kapa, filename, false, KAPA_PS_NEWPLOT, pagename);
    } else {
	KiiPS (*kapa, filename, false, KAPA_PS_NEWPAGE, pagename);
    }

    psFree (rg);
    psFree (fg);
    psFree (rb);
    psFree (fb);

    nCount ++;
    return true;
}

# else

bool psphotRadialPlot (int *kapa, const char *filename, pmSource *source) {
    psLogMsg ("psphot", 3, "skipping source radial plots");
    return true;
}

# endif
