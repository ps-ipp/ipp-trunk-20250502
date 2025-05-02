# include "fpcamera.h"

# define ESCAPE(ERROR, MSG) { psErrorStackPrint(stderr, MSG); return false; }

bool fpcameraMarkStar (pmReadout *readout, float Xo, float Yo);
bool fpcameraMakeSources (pmReadout *readout, psArray *refstars);

/* \brief this function loops over chips and performs forced photometry for the references */
bool fpcameraChooseRefstars (pmFPAfile *input, pmFPAfile *astrom, pmFPAview *view) {

    // the astrometry FPA and Chip store the references and the astrometric info
    pmFPA *astromFPA = astrom->fpa;
    pmChip *astromChip = pmFPAviewThisChip(view, astromFPA);
    pmReadout *readout = pmFPAviewThisReadout(view, input->fpa);

    // use the ra,dec range of the chip to make an initial cut before
    // transforming the positions to x,y.  Note the reference coordinates (refs->sky->r,d) are
    // stored in radians, so compare in radians
    float rMin = RAD_DEG*psMetadataLookupF32 (NULL, astromChip->analysis, "RA_MIN");
    float rMax = RAD_DEG*psMetadataLookupF32 (NULL, astromChip->analysis, "RA_MAX");
    float dMin = RAD_DEG*psMetadataLookupF32 (NULL, astromChip->analysis, "DEC_MIN");
    float dMax = RAD_DEG*psMetadataLookupF32 (NULL, astromChip->analysis, "DEC_MAX");

    // the full set of references is saved on the analysis MD of the astrometry FPA structure
    psArray *allrefs = psMetadataLookupPtr (NULL, astromFPA->analysis, "FPCAMERA.REFSTARS");

    // XXX is the astrom readout extent the same as the input readout?
    psRegion *extent = pmReadoutExtent (readout);
    if (!extent) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find readout size!");

    // full pixel range of the chip
    float minX = extent->x0;
    float maxX = extent->x1;
    float minY = extent->y0;
    float maxY = extent->y1;
    psFree (extent);

    // the refstars is a subset within range of this chip
    psArray *refstars = psArrayAllocEmpty (100);

    // select the reference objects within range of this readout
    // project the reference objects to this chip
    for (int i = 0; i < allrefs->n; i++) {

	pmAstromObj *ref = allrefs->data[i];
	
	if (ref->sky->r < rMin) continue;
	if (ref->sky->r > rMax) continue;
	if (ref->sky->d < dMin) continue;
	if (ref->sky->d > dMax) continue;

	// use the astrometry source to transform to chip coordinates
	psProject (ref->TP, ref->sky, astromFPA->toSky);
	psPlaneTransformApply (ref->FP, astromFPA->fromTPA, ref->TP);
	psPlaneTransformApply (ref->chip, astromChip->fromFPA, ref->FP);

	// limit the X,Y range of the refs to the selected chip
	if (ref->chip->x < minX) continue;
	if (ref->chip->x > maxX) continue;
	if (ref->chip->y < minY) continue;
	if (ref->chip->y > maxY) continue;

	// XXX mark the location of the refstars with a small box
	// fpcameraMarkStar (readout, ref->chip->x, ref->chip->y);

	psArrayAdd (refstars, 100, ref);
    }

    fpcameraMakeSources (readout, refstars);

    const char *chipName = psMetadataLookupStr(NULL, astromChip->concepts, "CHIP.NAME");

    psLogMsg ("fpcamera", 3, "Extracted %d reference stars for chip %s\n", (int) refstars->n, chipName);
    psFree (refstars);
    
    return true;
}

bool fpcameraMarkStar (pmReadout *readout, float Xo, float Yo) {

    psImage *image = readout->image;

    int nX = image->numCols;
    int nY = image->numRows;

    int xS = PS_MIN(PS_MAX(Xo - 5, 0), nX - 1);
    int xE = PS_MIN(PS_MAX(Xo + 5, 0), nX - 1);
    int yS = PS_MIN(PS_MAX(Yo - 5, 0), nY - 1);
    int yE = PS_MIN(PS_MAX(Yo + 5, 0), nY - 1);

    for (int ix = xS; ix <= xE; ix++) {
	for (int iy = yS; iy <= yE; iy++) {

	    float value = image->data.F32[iy][ix];
	    value = value * 0.5;
	    image->data.F32[iy][ix] = value;
	}
    }
    return true;
}

bool fpcameraMakeSources (pmReadout *readout, psArray *refstars) {

    // generate pmDetections to carry the sources
    pmDetections *detections = pmDetectionsAlloc();
    detections->allSources   = psArrayAllocEmpty (100);
    detections->newSources   = psArrayAllocEmpty (100);

    bool status = psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections);
    if (!status) ESCAPE(FPCAMERA_ERR_CONFIG, "problem saving detections on readout");

    // define PSF model type
    // int modelType = pmModelClassGetType ("PS_MODEL_GAUSS");

    // for now, just use a PSF model (add EXT model later, see pmSourceIO_CFF.c)

    for (int i = 0; i < refstars->n; i++) { 

	pmAstromObj *ref = refstars->data[i];
	
	float Xraw = ref->chip->x;
	float Yraw = ref->chip->y;

	// pixel coordinate in the image
	int Xoff = Xraw - readout->image->row0 - 0.5;
	int Yoff = Yraw - readout->image->col0 - 0.5;

	// create a new source
	pmSource *source = pmSourceAlloc();

	source->seq       = i;
	source->imageID   = 0;
	source->type      = PM_SOURCE_TYPE_UNKNOWN; // RoughClass wants source type to be unknown
	source->type      = PM_SOURCE_TYPE_STAR;    // until we know more, assume a PSF fit
	source->mode     |= PM_SOURCE_MODE_EXTERNAL;
	source->mode2    |= PM_SOURCE_MODE2_MATCHED; // source is generated based on another image
	source->tmpFlags  = 0;
	source->tmpFlags |= PM_SOURCE_TMPF_CANDIDATE_PSFSTAR; // XXX choose good PSF stars

	source->sky    = 0.0;
	source->skyErr = 0.0;
	
	source->psfMag    = 0.0;
	source->psfMagErr = 0.0;
	source->apMag     = 0.0;
	source->apRadius  = 0.0;

	// The peak type is not used in psphot. PM_PEAK_LONE may be wrong, but irrelevant
	// the supplied peak flux needs to be re-normalized
	float peakFlux = readout->image->data.F32[Yoff][Xoff];
	source->peak     = pmPeakAlloc(Xraw, Yraw, peakFlux, PM_PEAK_LONE);
	source->peak->xf = Xraw;
	source->peak->yf = Yraw;
	source->peak->dx = 0.0;
	source->peak->dy = 0.0;
	source->peak->rawFlux    = 1.0;
	source->peak->smoothFlux = 1.0;
	source->peak->detValue   = 1.0;

	// allocate space for moments
	source->moments = pmMomentsAlloc();
	source->moments->Mx = Xraw;
	source->moments->My = Yraw;
	source->moments->Mrf = 5; // kronRadius is 2.5 * first radial moment
	
	// allocate image, weight, mask arrays for each peak (square of radius OUTER)
	// XXX how does this happen? pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);
	
	// XXX not clear we need to define a model here
# if (0)
	pmModel *model = pmModelAlloc (modelType);
	source->modelPSF  = model;

	// NOTE: A SEGV here because "model" is NULL is probably caused by not initialising the models.
	psF32 *PAR = model->params->data.F32;
	psF32 *dPAR = model->dparams->data.F32;

	PAR[PM_PAR_XPOS]  = Xraw;
	PAR[PM_PAR_YPOS]  = Yraw;
	
	dPAR[PM_PAR_XPOS] = 0.0;
	dPAR[PM_PAR_YPOS] = 0.0;
	
	PAR[PM_PAR_SKY]   = 0.0;
	dPAR[PM_PAR_SKY]  = 0.0;
	
	PAR[PM_PAR_I0]    = 1.0;
	dPAR[PM_PAR_I0]   = 0.0;
	
	// we generate a somewhat fake PSF model here -- 
	// in most (all?) contexts, we will replace this with a measured psf model
	// elsewhere
	psEllipseAxes axes;
	axes.major        = 1.0;
	axes.minor        = 1.0;
	axes.theta        = 0.0;
	pmPSF_AxesToModel (PAR, axes, model->class->useReff);
# endif

	psArrayAdd (detections->allSources, 100, source);
	psFree (source);
    }
    psFree (detections); // we have placed a reference on readout->analysis so we must free this copy
    return true;
}

