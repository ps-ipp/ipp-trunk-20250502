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

# define DEBUG 0

bool pmVisualLimitsFromVectors (Graphdata *graphdata, psVector *xVec, psVector *yVec);

// functions used to visualize the analysis as it goes
// these are invoked by the -visual options

static int kapa1 = -1;
static int kapa2 = -1;
static int kapa3 = -1;

/** destroy windows at the end of a run*/
bool psphotVisualClose(void)
{
    if(kapa1 != -1) KapaClose(kapa1);
    if(kapa2 != -1) KapaClose(kapa2);
    if(kapa3 != -1) KapaClose(kapa3);
    return true;
}

int psphotKapaChannel (int channel) {

    switch (channel) {
      case 1:
	pmVisualInitWindow (&kapa1, "psphot:images");
	return kapa1;
      case 2:
	pmVisualInitWindow (&kapa2, "psphot:plots");
        return kapa2;
      case 3:
	pmVisualInitWindow (&kapa3, "psphot:stamps");
        return kapa3;
      default:
        psAbort ("unknown kapa channel");
    }
    psAbort ("unknown kapa channel");
}

bool psphotVisualEraseOverlays (int channel, char *overlay) {

    int myKapa = psphotKapaChannel (channel);
    if (myKapa == -1) return false;

    if (!(strcasecmp (overlay, "all"))) {
	KiiEraseOverlay (myKapa, "red");
	KiiEraseOverlay (myKapa, "green");
	KiiEraseOverlay (myKapa, "blue");
	KiiEraseOverlay (myKapa, "yellow");
	return true;
    }
    KiiEraseOverlay (myKapa, overlay);
    return true;
}

bool psphotVisualShowMask (int kapaFD, psImage *inImage, const char *name, int channel) {

    KiiImage image;
    KapaImageData data;
    Coords coords;

    strcpy (coords.ctype, "RA---TAN");

    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;

    ALLOCATE (image.data2d, float *, image.Ny);
    for (int iy = 0; iy < image.Ny; iy++) {
        ALLOCATE (image.data2d[iy], float, image.Nx);
        for (int ix = 0; ix < image.Nx; ix++) {
            image.data2d[iy][ix] = inImage->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix];
        }
    }

    strcpy (data.name, name);
    strcpy (data.file, name);
    data.zero = -1;
    data.range = 32;
    data.logflux = 0;

    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);

    for (int iy = 0; iy < image.Ny; iy++) {
        free (image.data2d[iy]);
    }
    free (image.data2d);

    return true;
}

bool psphotVisualShowObjectRegions (pmReadout *readout, psMetadata *recipe, psArray *sources) {

    KiiImage image;
    KapaImageData data;
    Coords coords;

    bool status = false;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    maskVal |= markVal;

    if (!pmVisualTestLevel("psphot.image.objects", 2)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    strcpy (coords.ctype, "RA---TAN");

    psImage *inImage = readout->image;
    psImage *inMask = readout->mask;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;
    
    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    if (!psImageBackground(stats, NULL, inImage, inMask, 0xffff, rng)) {
        fprintf (stderr, "failed to get background values\n");
        return false;
    }

    ALLOCATE (image.data2d, float *, image.Ny);
    for (int iy = 0; iy < image.Ny; iy++) {
        ALLOCATE (image.data2d[iy], float, image.Nx);
	for (int ix = 0; ix < image.Nx; ix++) {
	    image.data2d[iy][ix] = 0;
	}
    }

    // loop over sources and set unmasked pixels to 0
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (source == NULL) continue;

	psImage *mask = source->maskObj;
	if (mask == NULL) continue;

	for (int iy = 0; iy < mask->numRows; iy++) {
	    int jy = iy + mask->row0;
	    if (jy < 0) continue;
	    if (jy >= inImage->numRows) continue;
	    for (int ix = 0; ix < mask->numCols; ix++) {
		int jx = ix + mask->col0;
		if (jx < 0) continue;
		if (jx >= inImage->numCols) continue;

		if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;
		image.data2d[jy][jx] = 1;
	    }
	}
    }

    for (int iy = 0; iy < image.Ny; iy++) {
	for (int ix = 0; ix < image.Nx; ix++) {
	    image.data2d[iy][ix] = (image.data2d[iy][ix] == 0.0) ? NAN : inImage->data.F32[iy][ix];
	}
    }

    strcpy (data.name, "maskObj");
    strcpy (data.file, "maskObj");
    // data.zero = 0.0;
    // data.range = 1.0;
    data.zero = stats->robustMedian - stats->robustStdev;
    data.range = 5*stats->robustStdev;
    data.logflux = 0;

    KiiSetChannel (kapa, 2);
    KiiNewPicture2D (kapa, &image, &data, &coords);

    for (int iy = 0; iy < image.Ny; iy++) {
        free (image.data2d[iy]);
    }
    free (image.data2d);

    psFree (stats);
    psFree (rng);
    
    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualScaleImage (int kapaFD, psImage *inImage, psImage *inMask, const char *name, float factor, int channel) {

    KiiImage image;
    KapaImageData data;
    Coords coords;

    strcpy (coords.ctype, "RA---TAN");

    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    if (!psImageBackground(stats, NULL, inImage, inMask, 0xffff, rng)) {
        fprintf (stderr, "failed to get background values\n");
        return false;
    }

    image.data2d = inImage->data.F32;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;

    strcpy (data.name, name);
    strcpy (data.file, name);
    data.zero = stats->robustMedian - factor*stats->robustStdev;

    // XXX I we have a smoothed image, this make a much-too-tight display range
    data.range = 5*factor*stats->robustStdev;
    data.logflux = 0;

    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);

    psFree (stats);
    psFree (rng);

    return true;
}

bool psphotVisualRangeImage (int kapaFD, psImage *inImage, const char *name, int channel, float min, float max) {

    KiiImage image;
    KapaImageData data;
    Coords coords;

    strcpy (coords.ctype, "RA---TAN");

    image.data2d = inImage->data.F32;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;

    strcpy (data.name, name);
    strcpy (data.file, name);
    data.zero = min;
    data.range = max - min;
    data.logflux = 0;

    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);

    return true;
}

static psImage *posImage = NULL;
static psImage *delImage = NULL;

bool psphotVisualShowImage (pmReadout *readout) {

    if (!pmVisualTestLevel("psphot.image", 1)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    float factor = 1.0;
    if (readout->covariance) {
	factor = psImageCovarianceFactorForAperture(readout->covariance, 10.0);
    }

    psphotVisualShowMask (kapa, readout->mask, "mask", 2);
    psphotVisualScaleImage (kapa, readout->variance, readout->mask, "variance", 1.0, 1);
    psphotVisualScaleImage (kapa, readout->image, readout->mask, "image", sqrt(factor), 0);

    if (posImage == NULL) {
	posImage = psImageCopy (NULL, readout->image, PS_TYPE_F32);
    }

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowBackground (pmConfig *config, const pmFPAview *view, pmReadout *readout) {

    if (!pmVisualTestLevel("psphot.image.backgnd", 2)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    bool status = false;
    pmFPAfile *file = psMetadataLookupPtr (&status, config->files, "PSPHOT.BACKGND");

    pmReadout *backgnd = READOUT_OR_INTERNAL(view, file);

    float factor = 1.0;
    if (readout->covariance) {
	factor = psImageCovarianceFactorForAperture(readout->covariance, 10.0);
    }

    psphotVisualScaleImage (kapa, backgnd->image, readout->mask, "backgnd", 1.0, 2);
    psphotVisualScaleImage (kapa, readout->image, readout->mask, "backsub", sqrt(factor), 0);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowSignificance (psImage *image, float min, float max) {

    if (!pmVisualTestLevel("psphot.image.signif", 2)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    psphotVisualRangeImage (kapa, image, "signif", 2, min, max);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowLogSignificance (psImage *image, float min, float max) {

    if (!pmVisualTestLevel("psphot.image.logsignif", 3)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    psImage *lsig = (psImage *) psUnaryOp (NULL, image, "log");
    psphotVisualRangeImage (kapa, lsig, "log-signif", 2, min, max);
    psFree (lsig);

    pmVisualAskUser(NULL);
    return true;
}

// requires psphotVisualShowImage
bool psphotVisualShowSources (psArray *sources) {

    int Noverlay;
    KiiOverlay *overlay;

    if (!pmVisualTestLevel("psphot.objects.sources", 1)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    // note: this uses the Ohana allocation tools:
    // ALLOCATE (overlay, KiiOverlay, 3*peaks->n + 1);
    ALLOCATE (overlay, KiiOverlay, sources->n + 2);

    Noverlay = 0;
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (!source) continue;

	pmPeak *peak = source->peak;
	if (!peak) continue;

	overlay[Noverlay].type = KII_OVERLAY_BOX;
	overlay[Noverlay].x = peak->xf;
	overlay[Noverlay].y = peak->yf;
	overlay[Noverlay].dx = 4.0;
	overlay[Noverlay].dy = 4.0;
	overlay[Noverlay].angle = 0.0;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
    }

    KiiLoadOverlay (kapa, overlay, Noverlay, "blue");
    FREE (overlay);

    pmVisualAskUser(NULL);
    return true;
}

// XXX : requires psphotVisualShowImage
bool psphotVisualShowPeaks (pmDetections *detections) {

    int Noverlay;
    KiiOverlay *overlay;

    if (!pmVisualTestLevel("psphot.objects.peaks", 1)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    psArray *peaks = detections->peaks;

    // note: this uses the Ohana allocation tools:
    // ALLOCATE (overlay, KiiOverlay, 3*peaks->n + 1);
    ALLOCATE (overlay, KiiOverlay, peaks->n + 2);

    Noverlay = 0;
    for (int i = 0; i < peaks->n; i++) {

	pmPeak *peak = peaks->data[i];
	if (peak == NULL) continue;

	overlay[Noverlay].type = KII_OVERLAY_BOX;
	overlay[Noverlay].x = peak->xf;
	overlay[Noverlay].y = peak->yf;
	overlay[Noverlay].dx = 2.0;
	overlay[Noverlay].dy = 2.0;
	overlay[Noverlay].angle = 0.0;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
    }

    KiiLoadOverlay (kapa, overlay, Noverlay, "red");
    FREE (overlay);

    pmVisualAskUser(NULL);
    return true;
}

// XXX : requires psphotVisualShowImage
bool psphotVisualShowFootprints (pmDetections *detections) {

    int Noverlay;
    KiiOverlay *overlay;

    if (!pmVisualTestLevel("psphot.objects.footprints", 3)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    psArray *footprints = detections->footprints;
    if (!footprints) return true;

    // note: this uses the Ohana allocation tools:
    int NOVERLAY = footprints->n;
    ALLOCATE (overlay, KiiOverlay, NOVERLAY);

    Noverlay = 0;
    for (int i = 0; i < footprints->n; i++) {

	pmSpan *span = NULL;

	pmFootprint *footprint = footprints->data[i];
	if (footprint == NULL) continue;
	if (footprint->spans == NULL) continue;
	if (footprint->spans->n < 1) continue;

	// draw the top
	// XXX need to allow top (and bottom) to have more than one span
	span = footprint->spans->data[0];
	overlay[Noverlay].type = KII_OVERLAY_LINE;
	overlay[Noverlay].x = span->x0;
	overlay[Noverlay].y = span->y;
	overlay[Noverlay].dx = span->x1 - span->x0;
	overlay[Noverlay].dy = 0;
	overlay[Noverlay].angle = 0.0;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
	CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);

	int ys = span->y;
	int x0s = span->x0;
	int x1s = span->x1;

	// draw the outer span edges
	for (int j = 1; j < footprint->spans->n; j++) {
	    pmSpan *span1 = footprint->spans->data[j];

	    int ye = span1->y;
	    int x0e = span1->x0;
	    int x1e = span1->x1;

	    // we cannot have two discontinuous spans on the top or bottom, right? (no, probably not right)
	    // find all of the spans in this row and generate x0e, x01:
	    for (int k = j + 1; k < footprint->spans->n; k++) {
		pmSpan *span2 = footprint->spans->data[k];
		if (span2->y > span1->y) break;
		x0e = PS_MIN (x0e, span2->x0);
		x1e = PS_MAX (x1e, span2->x1);
		j++;
	    }

	    overlay[Noverlay].type = KII_OVERLAY_LINE;
	    overlay[Noverlay].x = x0s;
	    overlay[Noverlay].y = ys;
	    overlay[Noverlay].dx = x0e - x0s;
	    overlay[Noverlay].dy = ye - ys;
	    overlay[Noverlay].angle = 0.0;
	    overlay[Noverlay].text = NULL;
	    Noverlay ++;
	    CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);

	    overlay[Noverlay].type = KII_OVERLAY_LINE;
	    overlay[Noverlay].x = x1s;
	    overlay[Noverlay].y = ys;
	    overlay[Noverlay].dx = x1e - x1s;
	    overlay[Noverlay].dy = ye - ys;
	    overlay[Noverlay].angle = 0.0;
	    overlay[Noverlay].text = NULL;
	    Noverlay ++;
	    CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);

	    ys = ye;
	    x0s = x0e;
	    x1s = x1e;
	}

	// draw the bottom
	span = footprint->spans->data[footprint->spans->n - 1];
	overlay[Noverlay].type = KII_OVERLAY_LINE;
	overlay[Noverlay].x = span->x0;
	overlay[Noverlay].y = span->y;
	overlay[Noverlay].dx = span->x1 - span->x0;
	overlay[Noverlay].dy = 0;
	overlay[Noverlay].angle = 0.0;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
	CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);
    }

    KiiLoadOverlay (kapa, overlay, Noverlay, "blue");
    FREE (overlay);

    pmVisualAskUser(NULL);
    return true;
}

// XXX : requires psphotVisualShowImage
bool psphotVisualShowMoments (psArray *sources) {

    int Noverlay;
    KiiOverlay *overlay;

    psEllipseMoments emoments;
    psEllipseAxes axes;

    if (!pmVisualTestLevel("psphot.objects.moments", 2)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    // XXX mark the different source classes with different color/shape dots
    // XXX are moments S/N and peak S/N consistent?

    // note: this uses the Ohana allocation tools:
    ALLOCATE (overlay, KiiOverlay, sources->n);

    Noverlay = 0;
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (source == NULL) continue;

	pmMoments *moments = source->moments;
	if (moments == NULL) continue;

	overlay[Noverlay].type = KII_OVERLAY_CIRCLE;
	overlay[Noverlay].x = moments->Mx;
	overlay[Noverlay].y = moments->My;

	emoments.x2 = moments->Mxx;
	emoments.xy = moments->Mxy;
	emoments.y2 = moments->Myy;

	axes = psEllipseMomentsToAxes (emoments, 20.0);

	overlay[Noverlay].dx = 2.0*axes.major;
	overlay[Noverlay].dy = 2.0*axes.minor;

	overlay[Noverlay].angle = axes.theta * PS_DEG_RAD;

	overlay[Noverlay].text = NULL;
	Noverlay ++;
    }

    KiiLoadOverlay (kapa, overlay, Noverlay, "yellow");
    FREE (overlay);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualPlotMoments (psMetadata *recipe, psMetadata *analysis, psArray *sources) {

    bool status;
    Graphdata graphdata;
    KapaSection section;

    if (!pmVisualTestLevel("psphot.moments", 1)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    KapaClearSections (myKapa);
    KapaInitGraph (&graphdata);
    KapaSetFont (myKapa, "courier", 14);

    section.bg = KapaColorByName ("none"); // XXX probably should be 'none'

    float SN_LIM = psMetadataLookupF32(&status, recipe, "PSF_SN_LIM");

    // select the max psfX,Y values for the plot limits
    float Xmin = 1000.0, Xmax = 0.0;
    float Ymin = 1000.0, Ymax = 0.0;
    {
	int nRegions = psMetadataLookupS32 (&status, analysis, "PSF.CLUMP.NREGIONS");
	for (int n = 0; n < nRegions; n++) {

	    char regionName[64];
	    snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", n);
	    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);

	    float psfX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");
	    float psfY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");
	    float psfdX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");
	    float psfdY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");

	    float X0 = psfX - 4.0*psfdX;
	    float X1 = psfX + 4.0*psfdX;
	    float Y0 = psfY - 4.0*psfdY;
	    float Y1 = psfY + 4.0*psfdY;

	    if (isfinite(X0)) { Xmin = PS_MIN(Xmin, X0); }
	    if (isfinite(X1)) { Xmax = PS_MAX(Xmax, X1); }
	    if (isfinite(Y0)) { Ymin = PS_MIN(Ymin, Y0); }
	    if (isfinite(Y1)) { Ymax = PS_MAX(Ymax, Y1); }
	}
    }
    Xmin = PS_MAX(Xmin, -0.1);
    Ymin = PS_MAX(Ymin, -0.1);

    // XXX test: hardwire plot limits
    // Xmin = -0.1; Ymin = -0.1;
    // Xmax = 20.1; Ymax = 20.1;

    // storage vectors for data to be plotted
    psVector *xBright = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yBright = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mBright = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *xFaint  = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yFaint  = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mFaint  = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    // construct the vectors
    int nB = 0;
    int nF = 0;
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (source->moments == NULL)
	    continue;

	xFaint->data.F32[nF] = source->moments->Mxx;
	yFaint->data.F32[nF] = source->moments->Myy;
	mFaint->data.F32[nF] = -2.5*log10(source->moments->Sum);
	nF++;

	// XXX make this a user-defined cutoff
	if (source->moments->SN < SN_LIM)
	    continue;

	xBright->data.F32[nB] = source->moments->Mxx;
	yBright->data.F32[nB] = source->moments->Myy;
	mBright->data.F32[nB] = -2.5*log10(source->moments->Sum);
	nB++;
    }
    xFaint->n = nF;
    yFaint->n = nF;
    mFaint->n = nF;

    xBright->n = nB;
    yBright->n = nB;
    mBright->n = nB;

    // three sections: MxxMyy, MagMxx, MagMyy

    // first section: MxxMyy
    section.dx = 0.75;
    section.dy = 0.75;
    section.x  = 0.00;
    section.y  = 0.00;
    section.name = psStringCopy ("MxxMyy");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Xmin;
    graphdata.ymin = Ymin;
    graphdata.xmax = Xmax;
    graphdata.ymax = Ymax;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = NAN;
    graphdata.padXp = 0.5;
    graphdata.padYp = 0.5;
    KapaBox (myKapa, &graphdata);

    KapaSendLabel (myKapa, "M_xx| (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "M_yy| (pixels)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nF, &graphdata);

    KapaPlotVector (myKapa, nF, xFaint->data.F32, "x");
    KapaPlotVector (myKapa, nF, yFaint->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nB, &graphdata);
    KapaPlotVector (myKapa, nB, xBright->data.F32, "x");
    KapaPlotVector (myKapa, nB, yBright->data.F32, "y");

    // second section: MagMyy
    section.dx = 0.75;
    section.dy = 0.25;
    section.x  = 0.00;
    section.y  = 0.75;
    section.name = psStringCopy ("MagMyy");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -17.1;
    graphdata.xmax =  -7.9;
    graphdata.ymin = Ymin;
    graphdata.ymax = Ymax;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = 0.5;
    graphdata.padYm = NAN;
    graphdata.padXp = NAN;
    graphdata.padYp = 0.5;
    strcpy (graphdata.labels, "0210");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "inst mag", KAPA_LABEL_XP);
    KapaSendLabel (myKapa, "M_yy| (pixels)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nF, &graphdata);
    KapaPlotVector (myKapa, nF, mFaint->data.F32, "x");
    KapaPlotVector (myKapa, nF, yFaint->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nB, &graphdata);
    KapaPlotVector (myKapa, nB, mBright->data.F32, "x");
    KapaPlotVector (myKapa, nB, yBright->data.F32, "y");

    // third section: MagMxx
    section.dx = 0.25;
    section.dy = 0.75;
    section.x  = 0.75;
    section.y  = 0.00;
    section.name = psStringCopy ("MagMxx");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Xmin;
    graphdata.xmax = Xmax;
    graphdata.ymin =  -7.9;
    graphdata.ymax = -17.1;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = 0.5;
    graphdata.padXp = 0.5;
    graphdata.padYp = NAN;
    strcpy (graphdata.labels, "2001");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "M_xx| (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "inst mag", KAPA_LABEL_YP);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nF, &graphdata);
    KapaPlotVector (myKapa, nF, xFaint->data.F32, "x");
    KapaPlotVector (myKapa, nF, mFaint->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nB, &graphdata);
    KapaPlotVector (myKapa, nB, xBright->data.F32, "x");
    KapaPlotVector (myKapa, nB, mBright->data.F32, "y");

    // draw N circles to outline the clumps
    {
	KapaSelectSection (myKapa, "MxxMyy");

	// draw a circle centered on psfX,Y with size of the psf limit
	psVector *xLimit  = psVectorAlloc (120, PS_TYPE_F32);
	psVector *yLimit  = psVectorAlloc (120, PS_TYPE_F32);

	int nRegions = psMetadataLookupS32 (&status, analysis, "PSF.CLUMP.NREGIONS");
	float PSF_CLUMP_NSIGMA = psMetadataLookupF32 (&status, recipe, "PSF_CLUMP_NSIGMA");

	graphdata.color = KapaColorByName ("blue");
	graphdata.style = 0;

	graphdata.xmin = Xmin;
	graphdata.ymin = Ymin;
	graphdata.xmax = Xmax;
	graphdata.ymax = Ymax;
	KapaSetLimits (myKapa, &graphdata);

	for (int n = 0; n < nRegions; n++) {

	    char regionName[64];
	    snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", n);
	    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);

	    float psfX  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");
	    float psfY  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");
	    float psfdX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");
	    float psfdY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");
	    float Rx = psfdX * PSF_CLUMP_NSIGMA;
	    float Ry = psfdY * PSF_CLUMP_NSIGMA;

	    for (int i = 0; i < xLimit->n; i++) {
		xLimit->data.F32[i] = Rx*cos(i*2.0*M_PI/120.0) + psfX;
		yLimit->data.F32[i] = Ry*sin(i*2.0*M_PI/120.0) + psfY;
	    }
	    KapaPrepPlot (myKapa, xLimit->n, &graphdata);
	    KapaPlotVector (myKapa, xLimit->n, xLimit->data.F32, "x");
	    KapaPlotVector (myKapa, yLimit->n, yLimit->data.F32, "y");
	}
	psFree (xLimit);
	psFree (yLimit);
    }

    psFree (xBright);
    psFree (yBright);
    psFree (mBright);
    psFree (xFaint);
    psFree (yFaint);
    psFree (mFaint);

    pmVisualAskUser(NULL);
    return true;
}

// assumes 'kapa' value is checked and set
bool psphotVisualShowRoughClass_Single (int myKapa, psArray *sources, pmSourceType type, pmSourceMode mode, char *color) {

    int Noverlay;
    KiiOverlay *overlay;

    psEllipseMoments emoments;
    psEllipseAxes axes;

    // note: this uses the Ohana allocation tools:
    ALLOCATE (overlay, KiiOverlay, sources->n);

    Noverlay = 0;
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (source == NULL) continue;

	if (source->type != type) continue;

	if (mode == PM_SOURCE_MODE_PSFSTAR) {
	    bool keep = false;
	    keep |= (source->tmpFlags & PM_SOURCE_TMPF_CANDIDATE_PSFSTAR);
	    keep |= (source->mode & PM_SOURCE_MODE_PSFSTAR);
	    if (!keep) continue;
	} else {
	    if (mode && !(source->mode & mode)) continue;
	}

	pmMoments *moments = source->moments;
	if (moments == NULL) continue;

	overlay[Noverlay].type = KII_OVERLAY_CIRCLE;
	overlay[Noverlay].x = moments->Mx;
	overlay[Noverlay].y = moments->My;

	emoments.x2 = moments->Mxx;
	emoments.y2 = moments->Myy;
	emoments.xy = moments->Mxy;

	axes = psEllipseMomentsToAxes (emoments, 20.0);

	overlay[Noverlay].dx = 2.0*axes.major;
	overlay[Noverlay].dy = 2.0*axes.minor;
	overlay[Noverlay].angle = axes.theta * PS_DEG_RAD;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
    }

    KiiLoadOverlay (myKapa, overlay, Noverlay, color);
    FREE (overlay);

    return true;
}

// XXX : requires psphotVisualShowImage
bool psphotVisualShowRoughClass (psArray *sources) {

    if (!pmVisualTestLevel("psphot.objects.size", 3)) return true;

    int myKapa = psphotKapaChannel (1);
    if (myKapa == -1) return false;

    KiiEraseOverlay (myKapa, "yellow"); // moments

    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_STAR, 0, "red");
    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_EXTENDED, 0, "blue");
    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_DEFECT, 0, "blue");
    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_SATURATED, 0, "red");
    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_STAR, PM_SOURCE_MODE_PSFSTAR, "yellow");
    psphotVisualShowRoughClass_Single (myKapa, sources, PM_SOURCE_TYPE_STAR, PM_SOURCE_MODE_SATSTAR, "green");

    fprintf (stdout, "red: STAR or SAT AREA; blue: EXTENDED or DEFECT; green: SATSTAR; yellow: PSFSTAR\n");
    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowPSFModel (pmReadout *readout, pmPSF *psf) {

    if (!pmVisualTestLevel("psphot.psf.model", 1)) return true;

    int myKapa = psphotKapaChannel (3);
    if (myKapa == -1) return false;

    int DX = 64;
    int DY = 64;

    psImage *psfMosaic = psImageAlloc (5*DX, 5*DY, PS_TYPE_F32);
    psImageInit (psfMosaic, 0.0);

    psImage *funMosaic = psImageAlloc (5*DX, 5*DY, PS_TYPE_F32);
    psImageInit (funMosaic, 0.0);

    psImage *resMosaic = psImageAlloc (5*DX, 5*DY, PS_TYPE_F32);
    psImageInit (resMosaic, 0.0);

    pmModel *modelRef = pmModelAlloc(psf->type);

    // generate a fake model at each of the 3x3 image grid positions
    for (int x = -2; x <= +2; x ++) {
	for (int y = -2; y <= +2; y ++) {
	    // use the center of the center pixel of the image
	    float xc = (int)((0.5 + 0.225*x)*readout->image->numCols) + readout->image->col0 + 0.5;
	    float yc = (int)((0.5 + 0.225*y)*readout->image->numRows) + readout->image->row0 + 0.5;

	    // assign the x and y coords to the image center
	    // create an object with center intensity of 1000
	    modelRef->params->data.F32[PM_PAR_SKY] = 0;
	    modelRef->params->data.F32[PM_PAR_I0] = 1000;
	    modelRef->params->data.F32[PM_PAR_XPOS] = xc;
	    modelRef->params->data.F32[PM_PAR_YPOS] = yc;

	    // create modelPSF from this model
	    pmModel *model = pmModelFromPSF (modelRef, psf);
	    if (!model) continue;

	    // place the reference object in the image center
	    // no need to mask the source here
	    // XXX should we measure this for the analytical model only or the full model?
	    pmModelAddWithOffset (psfMosaic, NULL, model, PM_MODEL_OP_FULL | PM_MODEL_OP_CENTER, 0, x*DX, y*DY);
	    pmModelAddWithOffset (funMosaic, NULL, model, PM_MODEL_OP_FUNC | PM_MODEL_OP_CENTER, 0, x*DX, y*DY);
	    pmModelAddWithOffset (resMosaic, NULL, model, PM_MODEL_OP_RES0 | PM_MODEL_OP_RES1 | PM_MODEL_OP_CENTER, 0, x*DX, y*DY);
	    psFree (model);
	}
    }

    psImage *psfLogFlux = (psImage *) psUnaryOp (NULL, psfMosaic, "log");
    psphotVisualRangeImage (myKapa, psfLogFlux, "psf_mosaic",    0, -2.0, 3.0);
    psphotVisualRangeImage (myKapa, funMosaic, "psf_analytical", 1, -10.0, 100.0);
    psphotVisualRangeImage (myKapa, resMosaic, "psf_residual",   2, -10.0, 100.0);

    psFree (psfMosaic);
    psFree (funMosaic);
    psFree (resMosaic);
    psFree (psfLogFlux);
    psFree (modelRef);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowPSFStars (psMetadata *recipe, pmPSF *psf, psArray *sources) {

    bool status;

    if (!pmVisualTestLevel("psphot.psf.stars", 2)) return true;

    int myKapa = psphotKapaChannel (3);
    if (myKapa == -1) return false;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // the source images are written to an image 10x the size of a PSF object
    // float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    // PS_ASSERT (status, false);

    int DX = 21;
    int DY = 21;

    // examine PSF sources in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // counters to track the size of the image and area used in a row
    int dX = 0;                         // starting corner of next box
    int dY = 0;                         // height of row so far
    int NX = 20*DX;                     // full width of output image
    int NY = 0;                         // total height of output image

    // first, examine the PSF stars:
    // - determine bounding boxes for summary image
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	bool keep = false;
	keep |= (source->mode & PM_SOURCE_MODE_PSFSTAR);
	if (!keep) continue;

	// how does this subimage get placed into the output image?
	// DX = source->pixels->numCols
	// DY = source->pixels->numRows

	if (dX + DX > NX) {
	    // too wide for the rest of this row
	    if (dX == 0) {
		// alone on this row
		NY += DY;
		dX = 0;
		dY = 0;
	    } else {
		// start the next row
		NY += dY;
		dX = DX;
		dY = DY;
	    }
	} else {
	    // extend this row
	    dX += DX;
	    dY = PS_MAX (dY, DY);
	}
    }
    NY += DY;

    // allocate output image
    psImage *outpos = psImageAlloc (NX, NY, PS_TYPE_F32);
    psImage *outsub = psImageAlloc (NX, NY, PS_TYPE_F32);
    psImageInit (outpos, 0.0);
    psImageInit (outsub, 0.0);

    int Xo = 0;                         // starting corner of next box
    int Yo = 0;                         // starting corner of next box
    dY = 0;                             // height of row so far

    int nPSF = 0;

    // next, examine the PSF stars:
    // - create output image array
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	bool keep = false;
	if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
	    nPSF ++;
	    keep = true;
	}
	if (!keep) continue;

	if (Xo + DX > NX) {
	    // too wide for the rest of this row
	    if (Xo == 0) {
		// place source alone on this row
		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

		pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);

		if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

		Yo += DY;
		Xo = 0;
		dY = 0;
	    } else {
		// start the next row
		Yo += dY;
		Xo = 0;

		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

		pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);

		if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

		Xo = DX;
		dY = DY;
	    }
	} else {
	    // extend this row
	    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
	    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	    psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

	    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	    psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);
	    if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

	    Xo += DX;
	    dY = PS_MAX (dY, DY);
	}
    }

    psphotVisualRangeImage (myKapa, outpos, "psfpos", 0, -0.05, 0.95);
    psphotVisualRangeImage (myKapa, outsub, "psfsub", 1, -0.05, 0.95);

    pmVisualAskUser(NULL);
    psFree (outpos);
    psFree (outsub);


    // after displaying (as an image) the psf stars, we cycle throught them and display their
    // radial profiles:
    psphotVisualPlotRadialProfiles (recipe, sources, PM_SOURCE_MODE_PSFSTAR);

    return true;
}

bool psphotVisualShowSatStars (psMetadata *recipe, pmPSF *psf, psArray *sources) {

    bool status;

    if (!pmVisualTestLevel("psphot.psf.sat", 3)) return true;

    int myKapa = psphotKapaChannel (3);
    if (myKapa == -1) return false;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // the source images are written to an image 10x the size of a PSF object
    // float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    // PS_ASSERT (status, false);

    int DX = 41;
    int DY = 41;

    // examine PSF sources in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // counters to track the size of the image and area used in a row
    int dX = 0;                         // starting corner of next box
    int dY = 0;                         // height of row so far
    int NX = 10*DX;                     // full width of output image
    int NY = 0;                         // total height of output image

    // first, examine the PSF and SAT stars:
    // - determine bounding boxes for summary image
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// only show "real" saturated stars (not defects)
	if (!(source->mode & PM_SOURCE_MODE_SATSTAR)) continue;;
	if (source->mode & PM_SOURCE_MODE_DEFECT) continue;;

	// how does this subimage get placed into the output image?
	// DX = source->pixels->numCols
	// DY = source->pixels->numRows

	if (dX + DX > NX) {
	    // too wide for the rest of this row
	    if (dX == 0) {
		// alone on this row
		NY += DY;
		dX = 0;
		dY = 0;
	    } else {
		// start the next row
		NY += dY;
		dX = DX;
		dY = DY;
	    }
	} else {
	    // extend this row
	    dX += DX;
	    dY = PS_MAX (dY, DY);
	}
    }
    NY += DY;

    // allocate output image
    psImage *outsat = psImageAlloc (NX, NY, PS_TYPE_F32);
    psImageInit (outsat, 0.0);

    int Xo = 0;                         // starting corner of next box
    int Yo = 0;                         // starting corner of next box
    dY = 0;                             // height of row so far

    int nSAT = 0;

    // next, examine the SAT stars:
    // - create output image array
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// only show "real" saturated stars (not defects)
	if (!(source->mode & PM_SOURCE_MODE_SATSTAR)) continue;;
	if (source->mode & PM_SOURCE_MODE_DEFECT) continue;;
	nSAT ++;

	if (Xo + DX > NX) {
	    // too wide for the rest of this row
	    if (Xo == 0) {
		// place source alone on this row
		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outsat, source, Xo, Yo, DX, DY, false);
		if (subtracted) pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

		Yo += DY;
		Xo = 0;
		dY = 0;
	    } else {
		// start the next row
		Yo += dY;
		Xo = 0;

		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
		psphotMosaicSubimage (outsat, source, Xo, Yo, DX, DY, false);
		if (subtracted) pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

		Xo = DX;
		dY = DY;
	    }
	} else {
	    // extend this row
	    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
	    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	    psphotMosaicSubimage (outsat, source, Xo, Yo, DX, DY, false);
	    if (subtracted) pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	    Xo += DX;
	    dY = PS_MAX (dY, DY);
	}
    }

    psphotVisualScaleImage (myKapa, outsat, NULL, "satstar", 1.0, 2);

    pmVisualAskUser(NULL);
    psFree (outsat);
    return true;
}

void plotline (int myKapa, Graphdata *graphdata, float x0, float y0, float x1, float y1) 
{
    float x[2], y[2];
    x[0] = x0;
    x[1] = x1;
    y[0] = y0;
    y[1] = y1;
    KapaPrepPlot   (myKapa, 2, graphdata);
    KapaPlotVector (myKapa, 2, x, "x");
    KapaPlotVector (myKapa, 2, y, "y");
}

bool psphotVisualPlotRadialProfile (int myKapa, pmSource *source, psImageMaskType maskVal, pmSourceMode showmode) {

    Graphdata graphdata;

    float Rmax = (showmode & PM_SOURCE_MODE_SATSTAR) ? 100.0 : 30.0;

    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

    int nPts = source->pixels->numRows * source->pixels->numCols;
    psVector *rg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *Rg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *fg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *rb = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *Rb = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *fb = psVectorAllocEmpty (nPts, PS_TYPE_F32);

    int ng = 0;
    int nb = 0;

    float Xo = NAN;
    float Yo = NAN;

    if (source->modelPSF) {
	Xo = source->modelPSF->params->data.F32[PM_PAR_XPOS] - source->pixels->col0;
	Yo = source->modelPSF->params->data.F32[PM_PAR_YPOS] - source->pixels->row0;
    } else {
	Xo = source->moments->Mx - source->pixels->col0;
	Yo = source->moments->My - source->pixels->row0;
    }

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    if ((source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal)) {
		rb->data.F32[nb] = hypot (ix + 0.5 - Xo, iy + 0.5 - Yo) ;
		// rb->data.F32[nb] = hypot (ix - Xo, iy - Yo) ;
		Rb->data.F32[nb] = log10(rb->data.F32[nb]);
		fb->data.F32[nb] = log10(source->pixels->data.F32[iy][ix]);
		nb++;
	    } else {
		rg->data.F32[ng] = hypot (ix + 0.5 - Xo, iy + 0.5 - Yo) ;
		// rg->data.F32[ng] = hypot (ix - Xo, iy - Yo) ;
		Rg->data.F32[ng] = log10(rg->data.F32[ng]);
		fg->data.F32[ng] = log10(source->pixels->data.F32[iy][ix]);
		ng++;
	    }
	}
    }

    KapaInitGraph (&graphdata);

    // ** linlog **
    KapaSelectSection (myKapa, "linlog");

    // examine sources to set data range
    graphdata.xmin =  -0.05;
    graphdata.xmax = Rmax + 0.05;
    graphdata.ymin = -0.05;
    graphdata.ymax = +8.05;
    KapaSetLimits (myKapa, &graphdata);

    KapaSetFont (myKapa, "helvetica", 14);
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "radius (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "log flux (counts)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, ng, &graphdata);
    KapaPlotVector (myKapa, ng, rg->data.F32, "x");
    KapaPlotVector (myKapa, ng, fg->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nb, &graphdata);
    KapaPlotVector (myKapa, nb, rb->data.F32, "x");
    KapaPlotVector (myKapa, nb, fb->data.F32, "y");

    // ** loglog **
    KapaSelectSection (myKapa, "loglog");

    // examine sources to set data range
    graphdata.xmin = -1.51;
    graphdata.xmax = log10(Rmax) + 0.02;
    graphdata.ymin = -0.05;
    graphdata.ymax = +8.05;
    graphdata.color = KapaColorByName ("black");
    KapaSetLimits (myKapa, &graphdata);

    KapaSetFont (myKapa, "helvetica", 14);
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "log radius (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "log flux (counts)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, ng, &graphdata);
    KapaPlotVector (myKapa, ng, Rg->data.F32, "x");
    KapaPlotVector (myKapa, ng, fg->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.3;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, nb, &graphdata);
    KapaPlotVector (myKapa, nb, Rb->data.F32, "x");
    KapaPlotVector (myKapa, nb, fb->data.F32, "y");

    if (source->modelPSF) {
	// generate model profiles (major and minor axis):
	// create a model with theta = 0.0 so major and minor axes are equiv to x and y:
	psEllipseShape rawShape, rotShape;

	rawShape.sx  = source->modelPSF->params->data.F32[PM_PAR_SXX] / M_SQRT2;
	rawShape.sy  = source->modelPSF->params->data.F32[PM_PAR_SYY] / M_SQRT2;
	rawShape.sxy = source->modelPSF->params->data.F32[PM_PAR_SXY];

	psEllipseAxes axes = psEllipseShapeToAxes (rawShape, 20.0);

	axes.theta = 0.0;

	rotShape = psEllipseAxesToShape (axes);

	psVector *params = psVectorAlloc(source->modelPSF->params->n, PS_TYPE_F32);
	for (int i = 0; i < source->modelPSF->params->n; i++) {
	    params->data.F32[i] = source->modelPSF->params->data.F32[i];
	}
	params->data.F32[PM_PAR_SXX] = rotShape.sx * M_SQRT2;
	params->data.F32[PM_PAR_SYY] = rotShape.sy * M_SQRT2;
	params->data.F32[PM_PAR_SXY] = rotShape.sxy;
	params->data.F32[PM_PAR_XPOS] = 0.0;
	params->data.F32[PM_PAR_YPOS] = 0.0;

	psVector *rmod = psVectorAlloc(Rmax*10, PS_TYPE_F32);
	psVector *fmaj = psVectorAlloc(Rmax*10, PS_TYPE_F32);
	psVector *fmin = psVectorAlloc(Rmax*10, PS_TYPE_F32);

	psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

	float r = 0.0;
	for (int i = 0; i < rmod->n; i++) {
	    r = i*0.1;
	    rmod->data.F32[i] = r;

	    coord->data.F32[1] = r;
	    coord->data.F32[0] = 0.0;
	    fmaj->data.F32[i] = log10(source->modelPSF->class->modelFunc (NULL, params, coord));

	    coord->data.F32[0] = r;
	    coord->data.F32[1] = 0.0;
	    fmin->data.F32[i] = log10(source->modelPSF->class->modelFunc (NULL, params, coord));
	}
	psFree (coord);
	psFree (params);

	float FWHM_MAJOR = 2.0*source->modelPSF->class->modelRadius (source->modelPSF->params, 0.5*source->modelPSF->params->data.F32[PM_PAR_I0]);
	float FWHM_MINOR = FWHM_MAJOR * (axes.minor / axes.major);
	if (FWHM_MAJOR < FWHM_MINOR) PS_SWAP (FWHM_MAJOR, FWHM_MINOR); 

	psEllipseMoments emoments;
	emoments.x2 = source->moments->Mxx;
	emoments.xy = source->moments->Mxy;
	emoments.y2 = source->moments->Myy;
	axes = psEllipseMomentsToAxes (emoments, 20.0);
	float MOMENTS_MAJOR = 2.355*axes.major;
	float MOMENTS_MINOR = 2.355*axes.minor;

	float logHM = log10(0.5*source->modelPSF->params->data.F32[PM_PAR_I0]);

	// reset source Add/Sub state to recorded
	if (subtracted) pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	// ** linlog **
	KapaSelectSection (myKapa, "linlog");
	KapaGetGraphData (myKapa, &graphdata);
	graphdata.color = KapaColorByName ("blue");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (myKapa, rmod->n, &graphdata);
	KapaPlotVector (myKapa, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (myKapa, rmod->n, fmin->data.F32, "y");
	plotline (myKapa, &graphdata, graphdata.xmin, logHM, graphdata.xmax, logHM);
	plotline (myKapa, &graphdata, 0.5*FWHM_MINOR, graphdata.ymin, 0.5*FWHM_MINOR, graphdata.ymax);
	graphdata.ltype = 1;
	plotline (myKapa, &graphdata, 0.5*MOMENTS_MINOR, graphdata.ymin, 0.5*MOMENTS_MINOR, graphdata.ymax);
	graphdata.ltype = 0;
	
	graphdata.color = KapaColorByName ("green");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (myKapa, rmod->n, &graphdata);
	KapaPlotVector (myKapa, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (myKapa, rmod->n, fmaj->data.F32, "y");
	plotline (myKapa, &graphdata, 0.5*FWHM_MAJOR, graphdata.ymin, 0.5*FWHM_MAJOR, graphdata.ymax);
	graphdata.ltype = 1;
	plotline (myKapa, &graphdata, 0.5*MOMENTS_MAJOR, graphdata.ymin, 0.5*MOMENTS_MAJOR, graphdata.ymax);
	graphdata.ltype = 0;
	
	for (int i = 0; i < rmod->n; i++) {
	    rmod->data.F32[i] = log10(rmod->data.F32[i]);
	}

	// ** loglog **
	KapaSelectSection (myKapa, "loglog");
	KapaGetGraphData (myKapa, &graphdata);
	graphdata.color = KapaColorByName ("blue");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (myKapa, rmod->n, &graphdata);
	KapaPlotVector (myKapa, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (myKapa, rmod->n, fmin->data.F32, "y");

	graphdata.color = KapaColorByName ("green");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (myKapa, rmod->n, &graphdata);
	KapaPlotVector (myKapa, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (myKapa, rmod->n, fmaj->data.F32, "y");

	psFree (rmod);
	psFree (fmin);
	psFree (fmaj);
    }

    psFree (rg);
    psFree (Rg);
    psFree (fg);
    psFree (rb);
    psFree (Rb);
    psFree (fb);
    return true;
}

bool psphotVisualPlotRadialProfiles (psMetadata *recipe, psArray *sources, pmSourceMode showmode) {

    KapaSection section;  // put the positive profile in one and the residuals in another?

    if (!pmVisualTestLevel("psphot.profiles", 3)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    bool status;
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    section.bg  = KapaColorByName ("none"); // XXX probably should be 'none'

    KapaClearSections (myKapa);
    // first section : mag vs CR nSigma
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "linlog");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    // first section : mag vs CR nSigma
    section.dx = 1.0;
    section.dy = 0.5;
    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "loglog");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    // loop over the PSF stars
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (!(source->mode & showmode)) continue;

	psphotVisualPlotRadialProfile (myKapa, source, maskVal, showmode);

	if (pmVisualTestLevel("psphot.image", 1)) {
	  int display = psphotKapaChannel (1);
	  KiiCenter (display, source->peak->xf, source->peak->yf, 1);
	  KiiOverlay overlay;
	  overlay.x = source->peak->xf;
	  overlay.y = source->peak->yf;
	  overlay.dx = 5;
	  overlay.dy = 5;
	  overlay.angle = 0.0;
	  overlay.type = KiiOverlayTypeByName ("circle");
	  KiiLoadOverlay (display, &overlay, 1, "red");
	  overlay.x = source->moments->Mx;
	  overlay.y = source->moments->My;
	  overlay.dx = 8;
	  overlay.dy = 8;
	  overlay.angle = 0.0;
	  overlay.type = KiiOverlayTypeByName ("circle");
	  KiiLoadOverlay (display, &overlay, 1, "blue");
	}

	// pause and wait for user input:
	// continue, save (provide name), ??
	char key[10];
	fprintf (stdout, "[e]rase and continue? [o]verplot and continue? [s]kip rest of stars? : ");
	if (!fgets(key, 8, stdin)) {
	    psWarning("Unable to read option");
	}
	if (key[0] == 'e') {
	    KapaClearPlots (myKapa);
	}
	if (key[0] == 's') {
	    break;
	}
    }

    return true;
}

bool psphotVisualShowFlags (psArray *sources) {

    int NoverlayE, NOVERLAYE;
    int NoverlayO, NOVERLAYO;
    KiiOverlay *overlayE, *overlayO;

    psEllipseMoments emoments;
    psEllipseAxes axes;

    // XXX skip this for now: it is not very clear
    return true;

    if (!pmVisualTestLevel("psphot.objects.flags", 3)) return true;

    int myKapa = psphotKapaChannel (1);
    if (myKapa == -1) return false;

    // note: this uses the Ohana allocation tools:
    NoverlayE = 0;
    NOVERLAYE = 100;
    ALLOCATE (overlayE, KiiOverlay, NOVERLAYE);

    NoverlayO = 0;
    NOVERLAYO = 100;
    ALLOCATE (overlayO, KiiOverlay, NOVERLAYO);

    for (int i = 0; i < sources->n; i++) {

	float Xo, Yo, Rmaj, Rmin, cs, sn;

	pmSource *source = sources->data[i];
	if (source == NULL) continue;

	pmMoments *moments = source->moments;
	if (0) {
	    emoments.x2 = moments->Mxx;
	    emoments.y2 = moments->Myy;
	    emoments.xy = moments->Mxy;
	    Xo = moments->Mx;
	    Yo = moments->My;

	    axes = psEllipseMomentsToAxes (emoments, 20.0);
	    Rmaj = 2.0*axes.major;
	    Rmin = 2.0*axes.minor;
	    cs = cos(axes.theta);
	    sn = sin(axes.theta);
	} else {
	    Rmaj = Rmin = 5.0;
	    cs = 1.0;
	    sn = 0.0;
	    Xo = source->peak->xf;
	    Yo = source->peak->yf;
	}

	unsigned short int flagMask = 0x01;
	for (int j = 0; j < 8; j++) {
	    if (source->mode & flagMask) {
		overlayE[NoverlayE].type = KII_OVERLAY_LINE;
		overlayE[NoverlayE].x = Xo;
		overlayE[NoverlayE].y = Yo;

		float phi = j*M_PI/4.0;
		overlayE[NoverlayE].dx = +Rmaj*cos(phi)*cs - Rmin*sin(phi)*sn;
		overlayE[NoverlayE].dy = +Rmaj*cos(phi)*sn + Rmin*sin(phi)*cs;
		overlayE[NoverlayE].angle = 0;
		overlayE[NoverlayE].text = NULL;
		NoverlayE ++;
		CHECK_REALLOCATE (overlayE, KiiOverlay, NOVERLAYE, NoverlayE, 100);
	    }
	    flagMask <<= 1;

	    if (source->mode & flagMask) {
		overlayO[NoverlayO].type = KII_OVERLAY_LINE;
		overlayO[NoverlayO].x = Xo + 1;
		overlayO[NoverlayO].y = Yo;

		float phi = j*M_PI/4.0;
		overlayO[NoverlayO].dx = +Rmaj*cos(phi)*cs - Rmin*sin(phi)*sn;
		overlayO[NoverlayO].dy = +Rmaj*cos(phi)*sn + Rmin*sin(phi)*cs;
		overlayO[NoverlayO].angle = 0;
		overlayO[NoverlayO].text = NULL;
		NoverlayO ++;
		CHECK_REALLOCATE (overlayO, KiiOverlay, NOVERLAYO, NoverlayO, 100);
	    }
	    flagMask <<= 1;
	}
    }

    KiiLoadOverlay (myKapa, overlayE, NoverlayE, "red");
    KiiLoadOverlay (myKapa, overlayO, NoverlayO, "yellow");
    FREE (overlayE);
    FREE (overlayO);

    fprintf (stdout, "even bits (0x0001, 0x0004, ... : red\n");
    fprintf (stdout, "odd bits (0x0002, 0x0008, ... : yellow\n");
    pmVisualAskUser(NULL);

    return true;
}

bool psphotVisualShowSourceSize_Single (int myKapa, psArray *sources, pmSourceMode mode, bool keep, float scale, char *color) {

    int Noverlay;
    KiiOverlay *overlay;

    psEllipseMoments emoments;
    psEllipseAxes axes;

    // note: this uses the Ohana allocation tools:
    ALLOCATE (overlay, KiiOverlay, sources->n);

    Noverlay = 0;
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (source == NULL) continue;

	if (mode) {
	    if (keep) {
		if (!(source->mode & mode)) continue;
	    } else {
		if (source->mode & mode) continue;
	    }
	}

	pmMoments *moments = source->moments;
	if (moments == NULL) continue;

	overlay[Noverlay].type = KII_OVERLAY_CIRCLE;
	overlay[Noverlay].x = moments->Mx;
	overlay[Noverlay].y = moments->My;

	emoments.x2 = moments->Mxx;
	emoments.y2 = moments->Myy;
	emoments.xy = moments->Mxy;

	axes = psEllipseMomentsToAxes (emoments, 20.0);

	overlay[Noverlay].dx = scale*2.0*axes.major;
	overlay[Noverlay].dy = scale*2.0*axes.minor;
	overlay[Noverlay].angle = axes.theta * PS_DEG_RAD;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
    }

    KiiLoadOverlay (myKapa, overlay, Noverlay, color);
    FREE (overlay);

    return true;
}

bool psphotVisualShowSourceSize (pmReadout *readout, psArray *sources) {

    if (!pmVisualTestLevel("psphot.objects.size", 2)) return true;

    int myKapa = psphotKapaChannel (1);
    if (myKapa == -1) return false;

    KiiEraseOverlay (myKapa, "red");
    KiiEraseOverlay (myKapa, "blue");
    KiiEraseOverlay (myKapa, "green");
    KiiEraseOverlay (myKapa, "yellow");

    psphotVisualShowSourceSize_Single (myKapa, sources, PM_SOURCE_MODE_EXT_LIMIT | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_SATSTAR, 0, 1.0, "green");
    psphotVisualShowSourceSize_Single (myKapa, sources, PM_SOURCE_MODE_EXT_LIMIT, 1, 1.0, "blue");
    psphotVisualShowSourceSize_Single (myKapa, sources, PM_SOURCE_MODE_CR_LIMIT, 1, 1.0, "red");
    psphotVisualShowSourceSize_Single (myKapa, sources, PM_SOURCE_MODE_DEFECT, 1, 2.0, "red");
    psphotVisualShowSourceSize_Single (myKapa, sources, PM_SOURCE_MODE_SATSTAR, 1, 1.0, "yellow");

    fprintf (stdout, "red: CR; blue: EXTENDED; green: PSF-like; yellow: SATSTAR\n");
    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualPlotSourceSize (psMetadata *recipe, psMetadata *analysis, psArray *sources) {

    bool status;
    Graphdata graphdata;
    KapaSection section;

    if (!pmVisualTestLevel("psphot.size", 2)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    KapaClearSections (myKapa);
    KapaInitGraph (&graphdata);
    KapaSetFont (myKapa, "courier", 14);

    section.bg  = KapaColorByName ("none"); // XXX probably should be 'none'

    // select the max psfX,Y values for the plot limits
    float Xmin = 1000.0, Xmax = 0.0;
    float Ymin = 1000.0, Ymax = 0.0;
    {
	int nRegions = psMetadataLookupS32 (&status, analysis, "PSF.CLUMP.NREGIONS");
	for (int n = 0; n < nRegions; n++) {

	    char regionName[64];
	    snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", n);
	    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);

	    float psfX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");
	    float psfY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");
	    float psfdX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");
	    float psfdY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");

	    float X0 = psfX - 10.0*psfdX;
	    float X1 = psfX + 10.0*psfdX;
	    float Y0 = psfY - 10.0*psfdY;
	    float Y1 = psfY + 10.0*psfdY;

	    if (isfinite(X0)) { Xmin = PS_MIN(Xmin, X0); }
	    if (isfinite(X1)) { Xmax = PS_MAX(Xmax, X1); }
	    if (isfinite(Y0)) { Ymin = PS_MIN(Ymin, Y0); }
	    if (isfinite(Y1)) { Ymax = PS_MAX(Ymax, Y1); }
	}
    }
    Xmin = PS_MAX(Xmin, -0.1);
    Ymin = PS_MAX(Ymin, -0.1);

    // storage vectors for data to be plotted
    psVector *xSAT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *ySAT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mSAT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sSAT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *xPSF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yPSF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mPSF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sPSF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *xEXT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yEXT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mEXT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sEXT = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *xDEF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yDEF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mDEF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sDEF = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *xLOW = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yLOW = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mLOW = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sLOW = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *xCR = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *yCR = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *mCR = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *sCR = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    // construct the vectors
    int nSAT = 0;
    int nEXT = 0;
    int nPSF = 0;
    int nDEF = 0;
    int nLOW = 0;
    int nCR  = 0;
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (source->moments == NULL) continue;

	// only plot the measured sources...
	if (!(source->tmpFlags & PM_SOURCE_TMPF_SIZE_MEASURED)) continue;

	if (source->mode & PM_SOURCE_MODE_CR_LIMIT) {
	    xCR->data.F32[nCR] = source->moments->Mxx;
	    yCR->data.F32[nCR] = source->moments->Myy;
	    mCR->data.F32[nCR] = -2.5*log10(source->moments->Sum);
	    sCR->data.F32[nCR] = source->extNsigma;
	    nCR++;
	}
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    xSAT->data.F32[nSAT] = source->moments->Mxx;
	    ySAT->data.F32[nSAT] = source->moments->Myy;
	    mSAT->data.F32[nSAT] = -2.5*log10(source->moments->Sum);
	    sSAT->data.F32[nSAT] = source->extNsigma;
	    nSAT++;
	}
	if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
	    xEXT->data.F32[nEXT] = source->moments->Mxx;
	    yEXT->data.F32[nEXT] = source->moments->Myy;
	    mEXT->data.F32[nEXT] = -2.5*log10(source->moments->Sum);
	    sEXT->data.F32[nEXT] = source->extNsigma;
	    nEXT++;
	    continue;
	}
	if (source->mode & PM_SOURCE_MODE_DEFECT) {
	    xDEF->data.F32[nDEF] = source->moments->Mxx;
	    yDEF->data.F32[nDEF] = source->moments->Myy;
	    mDEF->data.F32[nDEF] = -2.5*log10(source->moments->Sum);
	    sDEF->data.F32[nDEF] = source->extNsigma;
	    nDEF++;
	    continue;
	}
	if (source->psfMagErr > 0.1) {
	    xLOW->data.F32[nLOW] = source->moments->Mxx;
	    yLOW->data.F32[nLOW] = source->moments->Myy;
	    mLOW->data.F32[nLOW] = -2.5*log10(source->moments->Sum);
	    sLOW->data.F32[nLOW] = source->extNsigma;
	    nLOW++;
	    continue;
	}
	xPSF->data.F32[nPSF] = source->moments->Mxx;
	yPSF->data.F32[nPSF] = source->moments->Myy;
	mPSF->data.F32[nPSF] = -2.5*log10(source->moments->Sum);
	sPSF->data.F32[nPSF] = source->extNsigma;
	nPSF++;
    }

    xSAT->n = nSAT;
    ySAT->n = nSAT;
    mSAT->n = nSAT;
    sSAT->n = nSAT;

    xPSF->n = nPSF;
    yPSF->n = nPSF;
    mPSF->n = nPSF;
    sPSF->n = nPSF;

    xEXT->n = nEXT;
    yEXT->n = nEXT;
    mEXT->n = nEXT;
    sEXT->n = nEXT;

    xCR->n = nCR;
    yCR->n = nCR;
    mCR->n = nCR;
    sCR->n = nCR;

    xDEF->n = nDEF;
    yDEF->n = nDEF;
    mDEF->n = nDEF;
    sDEF->n = nDEF;

    xLOW->n = nLOW;
    yLOW->n = nLOW;
    mLOW->n = nLOW;
    sLOW->n = nLOW;

    // four sections: MxxMyy, MagMxx, MagMyy, MagSigma

    // first section: MxxMyy
    section.dx = 0.75;
    section.dy = 0.60;
    section.x  = 0.00;
    section.y  = 0.00;
    section.name = psStringCopy ("MxxMyy");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Xmin;
    graphdata.ymin = Ymin;
    graphdata.xmax = Xmax;
    graphdata.ymax = Ymax;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = NAN;
    graphdata.padXp = 0.5;
    graphdata.padYp = 0.5;
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "M_xx| (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "M_yy| (pixels)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nPSF, &graphdata);
    KapaPlotVector (myKapa, nPSF, xPSF->data.F32, "x");
    KapaPlotVector (myKapa, nPSF, yPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nEXT, &graphdata);
    KapaPlotVector (myKapa, nEXT, xEXT->data.F32, "x");
    KapaPlotVector (myKapa, nEXT, yEXT->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nDEF, &graphdata);
    KapaPlotVector (myKapa, nDEF, xDEF->data.F32, "x");
    KapaPlotVector (myKapa, nDEF, yDEF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nCR, &graphdata);
    KapaPlotVector (myKapa, nCR, xCR->data.F32, "x");
    KapaPlotVector (myKapa, nCR, yCR->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nSAT, &graphdata);
    KapaPlotVector (myKapa, nSAT, xSAT->data.F32, "x");
    KapaPlotVector (myKapa, nSAT, ySAT->data.F32, "y");

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nLOW, &graphdata);
    KapaPlotVector (myKapa, nLOW, xLOW->data.F32, "x");
    KapaPlotVector (myKapa, nLOW, yLOW->data.F32, "y");

    // second section: MagMyy
    section.dx = 0.75;
    section.dy = 0.20;
    section.x  = 0.00;
    section.y  = 0.80;
    section.name = psStringCopy ("MagMyy");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -17.1;
    graphdata.xmax =  -6.9;
    graphdata.ymin = Ymin;
    graphdata.ymax = Ymax;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = 0.5;
    graphdata.padYm = NAN;
    graphdata.padXp = NAN;
    graphdata.padYp = 0.5;
    strcpy (graphdata.labels, "0210");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "inst mag", KAPA_LABEL_XP);
    KapaSendLabel (myKapa, "M_yy| (pixels)", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nPSF, &graphdata);
    KapaPlotVector (myKapa, nPSF, mPSF->data.F32, "x");
    KapaPlotVector (myKapa, nPSF, yPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nEXT, &graphdata);
    KapaPlotVector (myKapa, nEXT, mEXT->data.F32, "x");
    KapaPlotVector (myKapa, nEXT, yEXT->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nDEF, &graphdata);
    KapaPlotVector (myKapa, nDEF, mDEF->data.F32, "x");
    KapaPlotVector (myKapa, nDEF, yDEF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nCR, &graphdata);
    KapaPlotVector (myKapa, nCR, mCR->data.F32, "x");
    KapaPlotVector (myKapa, nCR, yCR->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nSAT, &graphdata);
    KapaPlotVector (myKapa, nSAT, mSAT->data.F32, "x");
    KapaPlotVector (myKapa, nSAT, ySAT->data.F32, "y");

    // third section: MagMxx
    section.dx = 0.25;
    section.dy = 0.60;
    section.x  = 0.75;
    section.y  = 0.00;
    section.name = psStringCopy ("MagMxx");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = Xmin;
    graphdata.xmax = Xmax;
    graphdata.ymin =  -6.9;
    graphdata.ymax = -17.1;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = 0.5;
    graphdata.padXp = 0.5;
    graphdata.padYp = NAN;
    strcpy (graphdata.labels, "2001");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "M_xx| (pixels)", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "inst mag", KAPA_LABEL_YP);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nPSF, &graphdata);
    KapaPlotVector (myKapa, nPSF, xPSF->data.F32, "x");
    KapaPlotVector (myKapa, nPSF, mPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nEXT, &graphdata);
    KapaPlotVector (myKapa, nEXT, xEXT->data.F32, "x");
    KapaPlotVector (myKapa, nEXT, mEXT->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nDEF, &graphdata);
    KapaPlotVector (myKapa, nDEF, xDEF->data.F32, "x");
    KapaPlotVector (myKapa, nDEF, mDEF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nCR, &graphdata);
    KapaPlotVector (myKapa, nCR, xCR->data.F32, "x");
    KapaPlotVector (myKapa, nCR, mCR->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nSAT, &graphdata);
    KapaPlotVector (myKapa, nSAT, xSAT->data.F32, "x");
    KapaPlotVector (myKapa, nSAT, mSAT->data.F32, "y");

    // fourth section: MagSigma
    section.dx = 0.75;
    section.dy = 0.20;
    section.x  = 0.00;
    section.y  = 0.60;
    section.name = psStringCopy ("MagSigma");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmax =  -6.9;
    graphdata.xmin = -17.1;
    graphdata.ymin = -20.1;
    graphdata.ymax = +20.1;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = 0.5;
    graphdata.padYm = NAN;
    graphdata.padXp = 0.5;
    graphdata.padYp = 0.5;
    strcpy (graphdata.labels, "0100");
    KapaBox (myKapa, &graphdata);
    // KapaSendLabel (myKapa, "inst mag", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "EXT&ss&c", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nPSF, &graphdata);
    KapaPlotVector (myKapa, nPSF, mPSF->data.F32, "x");
    KapaPlotVector (myKapa, nPSF, sPSF->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nEXT, &graphdata);
    KapaPlotVector (myKapa, nEXT, mEXT->data.F32, "x");
    KapaPlotVector (myKapa, nEXT, sEXT->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nDEF, &graphdata);
    KapaPlotVector (myKapa, nDEF, mDEF->data.F32, "x");
    KapaPlotVector (myKapa, nDEF, sDEF->data.F32, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nCR, &graphdata);
    KapaPlotVector (myKapa, nCR, mCR->data.F32, "x");
    KapaPlotVector (myKapa, nCR, sCR->data.F32, "y");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 7;
    graphdata.size = 1.0;
    graphdata.style = 2;
    KapaPrepPlot   (myKapa, nSAT, &graphdata);
    KapaPlotVector (myKapa, nSAT, mSAT->data.F32, "x");
    KapaPlotVector (myKapa, nSAT, sSAT->data.F32, "y");

    // draw N circles to outline the clumps
    {
	KapaSelectSection (myKapa, "MxxMyy");

	// draw a circle centered on psfX,Y with size of the psf limit
	psVector *xLimit  = psVectorAlloc (120, PS_TYPE_F32);
	psVector *yLimit  = psVectorAlloc (120, PS_TYPE_F32);

	int nRegions = psMetadataLookupS32 (&status, analysis, "PSF.CLUMP.NREGIONS");
	float PSF_CLUMP_NSIGMA = psMetadataLookupF32 (&status, recipe, "PSF_CLUMP_NSIGMA");

	graphdata.color = KapaColorByName ("blue");
	graphdata.style = 0;

	graphdata.xmin = Xmin;
	graphdata.ymin = Ymin;
	graphdata.xmax = Xmax;
	graphdata.ymax = Ymax;
	KapaSetLimits (myKapa, &graphdata);

	for (int n = 0; n < nRegions; n++) {

	    char regionName[64];
	    snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", n);
	    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);

	    float psfX  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");
	    float psfY  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");
	    float psfdX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");
	    float psfdY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");
	    float Rx = psfdX * PSF_CLUMP_NSIGMA;
	    float Ry = psfdY * PSF_CLUMP_NSIGMA;

	    for (int i = 0; i < xLimit->n; i++) {
		xLimit->data.F32[i] = Rx*cos(i*2.0*M_PI/120.0) + psfX;
		yLimit->data.F32[i] = Ry*sin(i*2.0*M_PI/120.0) + psfY;
	    }
	    KapaPrepPlot (myKapa, xLimit->n, &graphdata);
	    KapaPlotVector (myKapa, xLimit->n, xLimit->data.F32, "x");
	    KapaPlotVector (myKapa, yLimit->n, yLimit->data.F32, "y");
	}
	psFree (xLimit);
	psFree (yLimit);
    }

    psFree (xSAT);
    psFree (ySAT);
    psFree (mSAT);
    psFree (sSAT);

    psFree (xEXT);
    psFree (yEXT);
    psFree (mEXT);
    psFree (sEXT);

    psFree (xPSF);
    psFree (yPSF);
    psFree (mPSF);
    psFree (sPSF);

    psFree (xDEF);
    psFree (yDEF);
    psFree (mDEF);
    psFree (sDEF);

    psFree (xLOW);
    psFree (yLOW);
    psFree (mLOW);
    psFree (sLOW);

    psFree (xCR);
    psFree (yCR);
    psFree (mCR);
    psFree (sCR);

    pmVisualAskUser(NULL);
    return true;
}

bool PlotSourceSizeAltSetVectors(psVector *m, psVector *k, psVector *v1, psVector *v2, psVector *v3, pmSource *source) {

    float Mxx = source->moments->Mxx;
    float Myy = source->moments->Myy;
    float Mxy = source->moments->Mxy;
    float Mminor = 0.5*(Mxx + Myy) - 0.5*sqrt(PS_SQR(Mxx - Myy) + 4.0*PS_SQR(Mxy));
    float KronMag = -2.5*log10(source->moments->KronFlux);
    
    psVectorAppend(m,  source->psfMag);
    psVectorAppend(k,  KronMag);
    psVectorAppend(v1, Mminor);
    psVectorAppend(v2, source->psfMag - KronMag);
    psVectorAppend(v3, source->extNsigma);
    return true;
}

bool PlotSourceSizeAltAddPoints(Graphdata *graphdata, int myKapa, psVector *x, psVector *y, char *colorname, int ptype, float size) {

    graphdata->color = KapaColorByName (colorname);
    graphdata->ptype = ptype;
    graphdata->size = size;
    graphdata->style = 2;
    KapaPrepPlot   (myKapa, x->n, graphdata);
    KapaPlotVector (myKapa, x->n, x->data.F32, "x");
    KapaPlotVector (myKapa, x->n, y->data.F32, "y");
    return true;
}

bool psphotVisualPlotSourceSizeAlt (psMetadata *recipe, psMetadata *analysis, psArray *sources) {

    Graphdata graphdata;
    KapaSection section;

    if (!pmVisualTestLevel("psphot.size", 2)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    KapaClearSections (myKapa);
    KapaInitGraph (&graphdata);
    KapaSetFont (myKapa, "courier", 14);

    section.bg  = KapaColorByName ("none"); // XXX probably should be 'none'

    // storage vectors for data to be plotted
    psVector *SATm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *SATk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *SAT1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *SAT2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *SAT3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *PSFm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *PSFk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *PSF1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *PSF2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *PSF3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *EXTm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *EXTk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *EXT1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *EXT2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *EXT3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *DEFm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *DEFk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *DEF1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *DEF2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *DEF3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *BADm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *BADk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *BAD1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *BAD2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *BAD3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    psVector *CRm = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *CRk = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *CR1 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *CR2 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *CR3 = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    // int notPSF = PM_SOURCE_MODE_SATSTAR | PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT | PM_SOURCE_MODE_DEFECT;
    int nSkip = 0;

    // construct the vectors
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (source->moments == NULL) {
	    nSkip ++;
	    continue;
	}

	bool found = false;

	// only plot the measured sources...
	if (!(source->tmpFlags & PM_SOURCE_TMPF_SIZE_MEASURED)) {
	    nSkip ++;
	    continue;
	}

        // any sources missing a large fraction should just be treated as PSFs
        if ((source->pixWeightNotBad < 0.9) || (source->pixWeightNotPoor < 0.9)) {
	    PlotSourceSizeAltSetVectors(BADm, BADk, BAD1, BAD2, BAD3, source);
        }
	if ((source->mode & PM_SOURCE_MODE_CR_LIMIT) || (source->tmpFlags & PM_SOURCE_TMPF_SIZE_CR_CANDIDATE)) {
	    PlotSourceSizeAltSetVectors(CRm, CRk, CR1, CR2, CR3, source);
	    found = true;
	}
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    PlotSourceSizeAltSetVectors(SATm, SATk, SAT1, SAT2, SAT3, source);
	    found = true;
	}
	if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
	    PlotSourceSizeAltSetVectors(EXTm, EXTk, EXT1, EXT2, EXT3, source);
	    found = true;
	}
	if (source->mode & PM_SOURCE_MODE_DEFECT) {
	    PlotSourceSizeAltSetVectors(DEFm, DEFk, DEF1, DEF2, DEF3, source);
	    found = true;
	}
	if (!found) {
	    PlotSourceSizeAltSetVectors(PSFm, PSFk, PSF1, PSF2, PSF3, source);
	}
	// if (!(source->mode & notPSF) && !(source->tmpFlags & PM_SOURCE_TMPF_SIZE_CR_CANDIDATE)) {
	//     PlotSourceSizeAltSetVectors(PSFm, PSFk, PSF1, PSF2, PSF3, source);
	// }
    }
    // three sections: kronMag vs Mminor, psfMag vs psfMag - KronMag, psfMag vs extNsigma

    // --- first section: kronMag vs Mminor ---
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.00;
    section.name = psStringCopy ("Mminor");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -17.1;
    graphdata.xmax =  -6.9;
    graphdata.ymin = -0.5;
    graphdata.ymax = +7.1;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = 5.0;
    graphdata.padXp = NAN;
    graphdata.padYp = NAN;
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "kron mag", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "M_minor| (pixels^2|)", KAPA_LABEL_YM);

    PlotSourceSizeAltAddPoints(&graphdata, myKapa, PSFk, PSF1, "black", 0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, EXTk, EXT1, "blue",  0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa,  CRk,  CR1, "red",   0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, DEFk, DEF1, "red",   7, 1.0);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, SATk, SAT1, "blue",  7, 1.2);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, BADk, BAD1, "green", 7, 1.4);

    // --- second section: dMag ----
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.33;
    section.name = psStringCopy ("dMag");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -17.1;
    graphdata.xmax =  -6.9;
    graphdata.ymin = -0.75;
    graphdata.ymax = +1.50;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = NAN;
    graphdata.padYm = 5.0;
    graphdata.padXp = 0.0;
    graphdata.padYp = NAN;
    strcpy (graphdata.labels, "0200");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "dMag", KAPA_LABEL_YM);

    PlotSourceSizeAltAddPoints(&graphdata, myKapa, PSFm, PSF2, "black", 0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, EXTm, EXT2, "blue",  0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa,  CRm,  CR2, "red",   0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, DEFm, DEF2, "red",   7, 1.0);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, SATm, SAT2, "blue",  7, 1.2);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, BADm, BAD2, "green", 7, 1.4);

    // --- third section: nSigma ---
    section.dx = 1.00;
    section.dy = 0.33;
    section.x  = 0.00;
    section.y  = 0.66;
    section.name = psStringCopy ("nSigma");
    KapaSetSection (myKapa, &section);
    psFree (section.name);

    graphdata.color = KapaColorByName ("black");
    graphdata.xmin = -17.1;
    graphdata.xmax = -6.9;
    graphdata.ymin = -10.1;
    graphdata.ymax = +10.1;
    KapaSetLimits (myKapa, &graphdata);

    graphdata.padXm = 0.0;
    graphdata.padYm = 5.0;
    graphdata.padXp = NAN;
    graphdata.padYp = NAN;
    strcpy (graphdata.labels, "0210");
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "psf msg", KAPA_LABEL_XP);
    KapaSendLabel (myKapa, "EXT nSigma", KAPA_LABEL_YM);

    PlotSourceSizeAltAddPoints(&graphdata, myKapa, PSFm, PSF3, "black", 0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, EXTm, EXT3, "blue",  0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa,  CRm,  CR3, "red",   0, 0.5);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, DEFm, DEF3, "red",   7, 1.0);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, SATm, SAT3, "blue",  7, 1.0);
    PlotSourceSizeAltAddPoints(&graphdata, myKapa, BADm, BAD3, "green", 7, 1.4);

    fprintf (stderr, "PSF: %ld, EXT: %ld, CR: %ld, DEF: %ld, SAT: %ld, TOTAL: %ld, nSkip: %d\n", 
	     PSFm->n, EXTm->n, CRm->n, DEFm->n, SATm->n, 
	     PSFm->n+ EXTm->n+ CRm->n+ DEFm->n+ SATm->n, nSkip);

    psFree (SATk);
    psFree (SATm);
    psFree (SAT1);
    psFree (SAT2);
    psFree (SAT3);

    psFree (EXTk);
    psFree (EXTm);
    psFree (EXT1);
    psFree (EXT2);
    psFree (EXT3);

    psFree (PSFk);
    psFree (PSFm);
    psFree (PSF1);
    psFree (PSF2);
    psFree (PSF3);

    psFree (DEFk);
    psFree (DEFm);
    psFree (DEF1);
    psFree (DEF2);
    psFree (DEF3);

    psFree (BADk);
    psFree (BADm);
    psFree (BAD1);
    psFree (BAD2);
    psFree (BAD3);

    psFree (CRk);
    psFree (CRm);
    psFree (CR1);
    psFree (CR2);
    psFree (CR3);

    pmVisualAskUser(NULL);
    return true;
}

// option to redo variance since in some cases we may have displayed a different image in the meanwhile
bool psphotVisualShowResidualImage (pmReadout *readout, bool reshow) {

    if (!pmVisualTestLevel("psphot.image.resid", 2)) return true;

    int myKapa = psphotKapaChannel (1);
    if (myKapa == -1) return false;

    float factor = 1.0;
    if (readout->covariance) {
	factor = psImageCovarianceFactorForAperture(readout->covariance, 10.0);
    }

    if (false && reshow) {
	psphotVisualShowMask (myKapa, readout->mask, "mask", 2);
	psphotVisualScaleImage (myKapa, readout->variance, readout->mask, "variance", 1.0, 1);
    }

    if (posImage) {
	delImage = (psImage *) psBinaryOp(delImage, posImage, "-", readout->image);
	psphotVisualScaleImage (myKapa, posImage, readout->mask, "posimage", sqrt(factor), 0);
	psphotVisualScaleImage (myKapa, delImage, readout->mask, "delimage", sqrt(factor), 2);
    }

    psphotVisualScaleImage (myKapa, readout->image, readout->mask, "resid", sqrt(factor), 1);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualPlotApResid (psArray *sources, float mean, float error, bool useApMag) {

    Graphdata graphdata;
    float lineX[2], lineY[2];

    if (!pmVisualTestLevel("psphot.apresid", 1)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    KapaClearSections (myKapa);
    KapaInitGraph (&graphdata);

    psVector *x = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *y = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *dy = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

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

	float dMag;
	if (useApMag) {
	    dMag = source->apMag - source->psfMag;
	} else {
	    float kMag = -2.5*log10(source->moments->KronFlux);
	    dMag = source->psfMag - kMag;
	}

	x->data.F32[n] = source->psfMag;
	y->data.F32[n] = dMag;
	dy->data.F32[n] = source->psfMagErr;
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

    // XXX test
    graphdata.xmin = -17.0;
    graphdata.xmax =  -9.0;
    graphdata.ymin = -0.31;
    graphdata.ymax = +0.31;

    KapaSetLimits (myKapa, &graphdata);

    KapaSetFont (myKapa, "helvetica", 14);
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "Ap Mag - PSF Mag", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    graphdata.etype |= 0x01;
    KapaPrepPlot (myKapa, n, &graphdata);
    KapaPlotVector (myKapa, n, x->data.F32, "x");
    KapaPlotVector (myKapa, n, y->data.F32, "y");
    KapaPlotVector (myKapa, n, dy->data.F32, "dym");
    KapaPlotVector (myKapa, n, dy->data.F32, "dyp");

    graphdata.color = KapaColorByName ("blue");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 0;
    graphdata.etype = 0;
    lineX[0] = graphdata.xmin;
    lineX[1] = graphdata.xmax;
    lineY[0] = lineY[1] = mean;
    KapaPrepPlot (myKapa, 2, &graphdata);
    KapaPlotVector (myKapa, 2, lineX, "x");
    KapaPlotVector (myKapa, 2, lineY, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 0;
    graphdata.etype = 0;
    lineX[0] = graphdata.xmin;
    lineX[1] = graphdata.xmax;
    lineY[0] = lineY[1] = mean + error;
    KapaPrepPlot (myKapa, 2, &graphdata);
    KapaPlotVector (myKapa, 2, lineX, "x");
    KapaPlotVector (myKapa, 2, lineY, "y");

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 0;
    graphdata.etype = 0;
    lineX[0] = graphdata.xmin;
    lineX[1] = graphdata.xmax;
    lineY[0] = lineY[1] = mean - error;
    KapaPrepPlot (myKapa, 2, &graphdata);
    KapaPlotVector (myKapa, 2, lineX, "x");
    KapaPlotVector (myKapa, 2, lineY, "y");

    psFree (x);
    psFree (y);
    psFree (dy);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualPlotChisq (psArray *sources) {

    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.chisq", 1)) return true;

    int myKapa = psphotKapaChannel (2);
    if (myKapa == -1) return false;

    KapaClearSections (myKapa);
    KapaInitGraph (&graphdata);

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
	if (!source->moments) continue;
	if (!isfinite(source->moments->Sum)) continue;
	if (!source->modelPSF) continue;
	if (!isfinite(source->modelPSF->chisq)) continue;

	x->data.F32[n] = -2.5*log10(source->moments->Sum);
	y->data.F32[n] = source->modelPSF->chisq / source->modelPSF->nDOF;
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

    // XXX test
    graphdata.xmin = -17.0;
    graphdata.xmax =  -3.0;
    graphdata.ymin =  -0.1;
    graphdata.ymax = +10.1;

    KapaSetLimits (myKapa, &graphdata);

    KapaSetFont (myKapa, "helvetica", 14);
    KapaBox (myKapa, &graphdata);
    KapaSendLabel (myKapa, "PSF Mag", KAPA_LABEL_XM);
    KapaSendLabel (myKapa, "ChiSq", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.size = 0.5;
    graphdata.style = 2;
    KapaPrepPlot (myKapa, n, &graphdata);
    KapaPlotVector (myKapa, n, x->data.F32, "x");
    KapaPlotVector (myKapa, n, y->data.F32, "y");

    psFree (x);
    psFree (y);

    pmVisualAskUser(NULL);
    return true;
}

bool psphotVisualShowPetrosians (psArray *sources) {

    int Noverlay, NOVERLAY;
    KiiOverlay *overlay;

    if (!pmVisualTestLevel("psphot.objects.petro", 2)) return true;

    int kapa = psphotKapaChannel (1);
    if (kapa == -1) return false;

    Noverlay = 0;
    NOVERLAY = 100;
    ALLOCATE (overlay, KiiOverlay, NOVERLAY);

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];

	if (!source) continue;
	if (!source->extpars) continue;
	if (!source->extpars->petProfile) continue;

	float petrosianRadius = source->extpars->petrosianRadius;
	psEllipseAxes *axes = &source->extpars->axes;

	overlay[Noverlay].type = KII_OVERLAY_CIRCLE;
	overlay[Noverlay].x = source->peak->xf;
	overlay[Noverlay].y = source->peak->yf;
	overlay[Noverlay].dx = 1.0*petrosianRadius;
	overlay[Noverlay].dy = 1.0*petrosianRadius*axes->minor/axes->major;
	overlay[Noverlay].angle = axes->theta * PS_DEG_RAD;
	overlay[Noverlay].text = NULL;
	Noverlay ++;
	CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);

	// overlay[Noverlay].type = KII_OVERLAY_CIRCLE;
	// overlay[Noverlay].x = source->peak->xf;
	// overlay[Noverlay].y = source->peak->yf;
	// overlay[Noverlay].dx = 2.0*petrosianRadius;
	// overlay[Noverlay].dy = 2.0*petrosianRadius*axes->minor/axes->major;
	// overlay[Noverlay].angle = axes->theta * PS_DEG_RAD;
	// overlay[Noverlay].text = NULL;
	// Noverlay ++;
	// CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 100);
    }

    KiiLoadOverlay (kapa, overlay, Noverlay, "red");
    FREE (overlay);

    pmVisualAskUser(NULL);
    return true;
}

# else

bool psphotVisualShowImage (pmConfig *config, pmReadout *readout) { return true; }
bool psphotVisualShowBackground (pmConfig *config, const pmFPAview *view, pmReadout *readout) { return true; }
bool psphotVisualShowSignificance (psImage *image) { return true; }
bool psphotVisualShowPeaks (pmConfig *config, const pmFPAview *view, pmDetections *detections) { return true; }
bool psphotVisualShowFootprints (pmConfig *config, const pmFPAview *view, pmDetections *detections) { return true; }
bool psphotVisualShowMoments (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }
bool psphotVisualPlotMoments (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }
bool psphotVisualShowRoughClass (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }
bool psphotVisualShowPSFStars (pmConfig *config, const pmFPAview *view, pmPSF *psf, psArray *sources) { return true; }
bool psphotVisualShowSatStars (pmConfig *config, const pmFPAview *view, pmPSF *psf, psArray *sources) { return true; }
bool psphotVisualShowPSFModel (pmConfig *config, pmReadout *readout, pmPSF *psf) { return true; }
bool psphotVisualShowFlags (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }
bool psphotVisualSourceSize (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }
bool psphotVisualShowResidualImage (pmConfig *config, pmReadout *readout) { return true; }
bool psphotVisualPlotApResid (pmConfig *config, const pmFPAview *view, psArray *sources) { return true; }

# endif

# if (0)
// *** make a histogram of the source counts in the x and y directions
psHistogram *nX = psHistogramAlloc (graphdata.xmin, graphdata.xmax, 50.0);
psHistogram *nY = psHistogramAlloc (graphdata.ymin, graphdata.ymax, 50.0);
psVectorHistogram (nX, xFaint, NULL, NULL, 0);
psVectorHistogram (nY, yFaint, NULL, NULL, 0);
psVector *dX = psVectorAlloc (nX->nums->n, PS_TYPE_F32);
psVector *vX = psVectorAlloc (nX->nums->n, PS_TYPE_F32);
psVector *dY = psVectorAlloc (nY->nums->n, PS_TYPE_F32);
psVector *vY = psVectorAlloc (nY->nums->n, PS_TYPE_F32);
for (int i = 0; i < nX->nums->n; i++) {
    dX->data.F32[i] = nX->nums->data.S32[i];
    vX->data.F32[i] = 0.5*(nX->bounds->data.F32[i] + nX->bounds->data.F32[i+1]);
}
for (int i = 0; i < nY->nums->n; i++) {
    dY->data.F32[i] = nY->nums->data.S32[i];
    vY->data.F32[i] = 0.5*(nY->bounds->data.F32[i] + nY->bounds->data.F32[i+1]);
}

graphdata.color = KapaColorByName ("black");
graphdata.ptype = 0;
graphdata.size = 0.0;
graphdata.style = 0;
KapaPrepPlot (myKapa, dX->n, &graphdata);
KapaPlotVector (myKapa, dX->n, dX->data.F32, "x");
KapaPlotVector (myKapa, vX->n, vX->data.F32, "y");

psFree (nX);
psFree (dX);
psFree (vX);

psFree (nY);
psFree (dY);
psFree (vY);

# endif

