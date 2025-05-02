# include "psphotInternal.h"

typedef struct {
    float min;
    float max;
    float lower20;
    float upper20;
    int Npts;
} QuickStats;

bool psImageQuickStats (QuickStats *stats, psImage *image, psRegion region);
float InterpolateValues (float X0, float Y0, float X1, float Y1, float X);
bool psphotVisualScaleImage (int kapaFD, psImage *inImage, psImage *inMask, const char *name, float factor, int channel);

// for now, let's store the detections on the readout->analysis for each readout
bool psphotDeblendSatstars (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = false;

    int num = psphotFileruleCount(config, filerule);

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full extended source non-linear fits?
    if (!psMetadataLookupBool (&status, recipe, "SUBTRACT_SATSTAR_PROFILE")) {
        psLogMsg ("psphot", PS_LOG_INFO, "skipping extended source fits\n");
        return true;
    }

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotDeblendSatstarsReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on saturated star deblend analysis for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

/** this function does 3 things:

    1) choose likely saturated stars
       - mode | SATSTAR 
       - if moments->Peak > 0.5*SATURATION, examine region of peak pixels (ensure we go down to 0.1*SAT) for SAT mask
      
    2) adjust the window for saturated objects

    3) measure the radial profile

    4) subtract the radial profile
    
    There is also support code for calculation of magnitudes and add/subtract the profile (a la pmSourceOp)

    TBD:

    * recenter the profile (based on cross-correlation / convolution with 1D profile)
 **/

bool psphotDeblendSatstarsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int fileIndex, psMetadata *recipe) {

    int N;
    pmSource *source;
    bool status;

    psTimerStart ("psphot.deblend.sat");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, fileIndex); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping satstar blend");
	return true;
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskSat = psMetadataLookupImageMask(&status, recipe, "MASK.SAT"); // Mask value for bad pixels
    assert (maskSat);

    float SATURATION = NAN;
    { 
	// XXX do we need to set this differently from the value used to mark saturated pixels?
	pmCell *cell     = readout->parent;

	// do not completely trust the values in the header...
	float CELL_SATURATION = psMetadataLookupF32 (&status, cell->concepts, "CELL.SATURATION");
	float MIN_SATURATION = psMetadataLookupF32 (&status, recipe, "DEBLEND_MIN_SATURATION");
	if (!status || !isfinite(MIN_SATURATION)) {
	    MIN_SATURATION = 40000.0;
	}
	if (!isfinite(CELL_SATURATION)) {
	    SATURATION = MIN_SATURATION;
	} else {
	    SATURATION = PS_MAX(MIN_SATURATION, CELL_SATURATION);
	}
    }

    // source analysis is done in peak order (brightest first)
    // we use an index for this so the spatial sorting is kept
    psVector *SN = psVectorAlloc (sources->n, PS_DATA_F32);
    for (int i = 0; i < SN->n; i++) {
        source = sources->data[i];
        SN->data.F32[i] = source->moments->SN;
    }
    psVector *index = psVectorSortIndex (NULL, SN);
    // this results in an index of increasing SN

    // psphotVisualPlotRadialProfiles (recipe, sources, PM_SOURCE_MODE_SATSTAR);

    int BIG_RADIUS = 250;
    int BIG_SIGMA = BIG_RADIUS / 4.0;

    // int display = psphotKapaChannel (1);
    // psphotVisualScaleImage (display, (psImage *) source->pixels->parent, NULL, "image", 1.0, 0);

    int Nsatstar = 0;

    // examine sources in decreasing SN order
    for (int i = sources->n - 1; i >= 0; i--) {
        N = index->data.U32[i];
        source = sources->data[N];

        if (!source->pixels) continue;
#ifndef DEBLEND_PASS2_SOURCES
        // XXX: Need to define this if we want to rerun the satstar stuff again on the pass2 sources
        // This is needed for psphotStack in -updatemode
        if ((source->mode2 & PM_SOURCE_MODE2_PASS1_SRC) == 0) continue;
#endif

	bool isSat = source->mode & PM_SOURCE_MODE_SATSTAR;

	// for fairly bright stars, check the pixels near the peak again
	if (source->moments->Peak > 0.5*SATURATION) {
	    // pmSourceRoughClass does this analysis, but uses a small (5x5) window.
	    // here we use a larger window since stacks can have some funny features
	    psRegion inner;
	    QuickStats stats;
	    // grow out the search radius until we have lower20 < 0.1*SATURATION
	    for (int radius = 2; radius < 30; radius ++) {
		inner = psRegionForSquare (source->peak->x, source->peak->y, radius);
		inner = psRegionForImage (source->maskView, inner);
		psImageQuickStats (&stats, readout->image, inner);
		if ((stats.Npts > 1) && (stats.lower20 < 0.1*SATURATION)) break;
	    }
	    int Nsatpix = psImageCountPixelMask (source->maskView, inner, maskSat);
	    // fprintf (stderr, "test object: %d,%d : %d vs %d : %f - %f - %f - %f\n",
	    // source->peak->x, source->peak->y, Nsatpix, stats.Npts, 
	    // stats.min, stats.lower20, stats.upper20, stats.max); 
	    if (Nsatpix > 1) isSat = true;
	}
        if (!isSat) continue;

	// For saturated stars, choose a much larger box NOTE this is slightly sleazy, but
	// only slightly: pmSourceRedefinePixels uses the readout to pass the pointers to
	// the parent image data.  I guess the API could be simplified: we could recover
	// this from the source in the function

	pmReadout tmpReadout;
	tmpReadout.image    = (psImage *)source->pixels->parent;
	tmpReadout.mask     = (psImage *)source->maskView->parent;
	tmpReadout.variance = (psImage *)source->variance->parent;

	// re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	pmSourceRedefinePixels (source, &tmpReadout, source->peak->x, source->peak->y, BIG_RADIUS + 2);

	psTrace ("psphot", 4, "retrying moments for %d, %d\n", source->peak->x, source->peak->y);
	status = pmSourceMoments (source, BIG_RADIUS, BIG_SIGMA, 0.0, 5.0, maskVal);
	source->mode |= PM_SOURCE_MODE_BIG_RADIUS;

	if (!psphotSatstarProfileModel (source, maskVal)) continue;

	source->mode |= PM_SOURCE_MODE_SATSTAR; // yes, this source IS saturated
	source->mode2 |= PM_SOURCE_MODE2_SATSTAR_PROFILE; // and we have in fact subtracted the profile

	Nsatstar ++;

	// XXX visualize, model, and subtract
	// if (!psphotVisualRadialProfileSatstar (source, maskVal)) {
	// break;
	// }

	// generate radial profile, store on the source structure
    }
    // show the image after object have been subtracted
    // psphotVisualScaleImage (display, (psImage *) source->pixels->parent, NULL, "image", 1.0, 1);

    psFree (SN);
    psFree (index);

    psLogMsg ("psphot", PS_LOG_INFO, "deblend %d satstars: %f sec\n", Nsatstar, psTimerMark ("psphot.deblend.sat"));
    return true;
}

bool psphotDeblendSatstarsReadoutOld (pmConfig *config, const pmFPAview *view, const char *filerule, int fileIndex) {

    int N;
    pmSource *source;
    bool status;

    psTimerStart ("psphot.deblend.sat");

    int Nblend = 0;
    float SAT_MIN_RADIUS = 5.0;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, fileIndex); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping satstar blend");
	return true;
    }

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    pmCell *cell = readout->parent;

    float SATURATION = NAN;

    // do not completely trust the values in the header...
    float CELL_SATURATION = psMetadataLookupF32 (&status, cell->concepts, "CELL.SATURATION");
    float MIN_SATURATION = psMetadataLookupF32 (&status, recipe, "DEBLEND_MIN_SATURATION");
    if (!status || !isfinite(MIN_SATURATION)) {
	MIN_SATURATION = 40000.0;
    }
    if (!isfinite(CELL_SATURATION)) {
	SATURATION = MIN_SATURATION;
    } else {
	SATURATION = PS_MAX(MIN_SATURATION, CELL_SATURATION);
    }
    float SAT_TEST_LEVEL = 0.5*SATURATION;

    // we need sources spatially-sorted to find overlaps
    sources = psArraySort (sources, pmSourceSortByY);

    // source analysis is done in peak order (brightest first)
    // we use an index for this so the spatial sorting is kept
    psVector *SN = psVectorAlloc (sources->n, PS_DATA_F32);
    for (int i = 0; i < SN->n; i++) {
        source = sources->data[i];
        SN->data.F32[i] = source->peak->rawFlux;
    }
    psVector *index = psVectorSortIndex (NULL, SN);
    // this results in an index of increasing SN

    // examine sources in decreasing SN order
    for (int i = sources->n - 1; i >= 0; i--) {
        N = index->data.U32[i];
        source = sources->data[N];

        // XXX filter? if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
        if (source->mode & PM_SOURCE_MODE_BLEND) continue;
        if (source->peak->rawFlux < SAT_TEST_LEVEL) continue;

	// save these for reference below
	int xPeak = source->peak->x;
	int yPeak = source->peak->y;

        // generate a basic contour (set of x,y coordinates at-or-below flux level)
        psArray *contour = pmSourceContour (source->pixels, xPeak, yPeak, SAT_TEST_LEVEL);
        if (contour == NULL) continue;

	// contour consists of a set of X,Y coords giving the boundary
	psVector *xVec = contour->data[0];
	psVector *yVec = contour->data[1];
	if (xVec->n < 5) {
	    psFree(contour);
	    continue;
	}

	// find the center of the contour (let's just use mid[x,y])
	int xMin = xVec->data.F32[0];
	int xMax = xVec->data.F32[0];
	int yMin = yVec->data.F32[0];
	int yMax = yVec->data.F32[0];
	for (int j = 0; j < xVec->n; j++) {
	    xMin = PS_MIN (xMin, xVec->data.F32[j]);
	    xMax = PS_MAX (xMax, xVec->data.F32[j]);
	    yMin = PS_MIN (yMin, yVec->data.F32[j]);
	    yMax = PS_MAX (yMax, yVec->data.F32[j]);
	}	
	int xCenter = 0.5*(xMin + xMax);
	int yCenter = 0.5*(yMin + yMax);
	psFree (contour);

	psAssert (xCenter >= source->pixels->col0, "invalid shift in object center");
	psAssert (xCenter <  source->pixels->col0 + source->pixels->numCols, "invalid shift in object center");
	psAssert (yCenter >= source->pixels->row0, "invalid shift in object center");
	psAssert (yCenter <  source->pixels->row0 + source->pixels->numRows, "invalid shift in object center");

	// reset the peak for this source to the value of the center pixel
	source->peak->x = xCenter;
	source->peak->y = yCenter;
	source->peak->xf = xCenter;
	source->peak->yf = yCenter;
	
	// temporary array for overlapping objects we find
        psArray *overlap = psArrayAllocEmpty (100);

        // search backwards for overlapping sources
        for (int j = N - 1; j >= 0; j--) {
            pmSource *testSource = sources->data[j];
            if (testSource->peak->x <  source->pixels->col0) continue;
            if (testSource->peak->x >= source->pixels->col0 + source->pixels->numCols) continue;
            if (testSource->peak->y <  source->pixels->row0) break;
            if (testSource->peak->y >= source->pixels->row0 + source->pixels->numRows) {
                fprintf (stderr, "warning: invalid condition\n");
                continue;
            }
            psArrayAdd (overlap, 100, testSource);
        }

        // search forwards for overlapping sources
        for (int j = N + 1; j < sources->n; j++) {
            pmSource *testSource = sources->data[j];
            if (testSource->peak->x <  source->pixels->col0) continue;
            if (testSource->peak->x >= source->pixels->col0 + source->pixels->numCols) continue;
            if (testSource->peak->y <  source->pixels->row0) {
                fprintf (stderr, "warning: invalid condition\n");
                continue;
            }
            if (testSource->peak->y >= source->pixels->row0 + source->pixels->numRows) break;
            psArrayAdd (overlap, 100, testSource);
        }

        if (overlap->n == 0) {
            psFree (overlap);
            continue;
        }

	// now find the contour which is at 0.5*SAT_TEST_LEVEL (xPeak, yPeak is valid high pixel)
        contour = pmSourceContour (source->pixels, xPeak, yPeak, 0.5*SAT_TEST_LEVEL);
        if (contour == NULL) {
	    psFree (overlap);
	    continue; 
	}
	
	// any peaks within this contour should be dropped (mark as blend, drop after loop is done)
	// also drop any peaks which are too close to this peak
        psVector *xv = contour->data[0];
        psVector *yv = contour->data[1];
        for (int k = 0; k < overlap->n; k++) {
            pmSource *testSource = overlap->data[k];
	    float radius = hypot((testSource->peak->x - xCenter), (testSource->peak->y - yCenter));
	    if (radius < SAT_MIN_RADIUS) {
                testSource->mode |= PM_SOURCE_MODE_BLEND;
                Nblend ++;
		continue;
	    }
            for (int j = 0; j < xv->n; j+=2) {
                if (fabs(yv->data.F32[j] - testSource->peak->y) > 0.5) continue;
                if (xv->data.F32[j+0] > testSource->peak->x) break;
                if (xv->data.F32[j+1] < testSource->peak->x) break;
                testSource->mode |= PM_SOURCE_MODE_BLEND;
                Nblend ++;
                j = xv->n; // skip rest of contour
            }
        }
        psFree (overlap);
        psFree (contour);
    }

    // drop the sources marked as BLEND
    for (int i = 0; i < sources->n;) {
	pmSource *source = sources->data[i];

        if (!(source->mode & PM_SOURCE_MODE_BLEND)) {
	    i++;
	    continue;
	}

	// shuffle the remaining sources forward
	for (int j = i; j < sources->n - 1; j++) {
	    sources->data[j] = sources->data[j+1];
	}
	psFree (source);
	sources->n --;
    }

    psFree (SN);
    psFree (index);

    psLogMsg ("psphot", PS_LOG_INFO, "found %d satstar blend peaks, leaving %ld sources: %f sec\n", Nblend, sources->n, psTimerMark ("psphot.deblend.sat"));
    return true;
}

// create a model for the radial profile of a saturated star (is this actually more generic?)
bool psphotSatstarProfileModel (pmSource *source, psImageMaskType maskVal) {

    // XXX user define somewhere?
    float Rmax = 320.0;

    // XXX is this ever the case??
    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

    int nPts = source->pixels->numRows * source->pixels->numCols;
    psVector *logR = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *flux = psVectorAllocEmpty (nPts, PS_TYPE_F32);

    int ng = 0;

    // choose the best center for this profile
    float Xo = NAN;
    float Yo = NAN;

    if (source->modelPSF) {
	// XXX do we ever have a PSF model for a SATSTAR?
	Xo = source->modelPSF->params->data.F32[PM_PAR_XPOS];
	Yo = source->modelPSF->params->data.F32[PM_PAR_YPOS];
    } else {
	Xo = source->moments->Mx;
	Yo = source->moments->My;
    }

    float Xc = Xo - source->pixels->col0 - 0.5;
    float Yc = Yo - source->pixels->row0 - 0.5;

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    if ((source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal)) continue;
	    // XXX do this faster by generating R^2 and returning 0.5*log10(R^2)?
	    logR->data.F32[ng] = log10(hypot (ix - Xc, iy - Yc));
	    flux->data.F32[ng] = source->pixels->data.F32[iy][ix];
	    ng++;
	}
    }
    logR->n = ng;
    flux->n = ng;

    // XXX do something sensible here if there are no pixels

    // measure the radial profile 
    psVector *logFmodel = NULL;
    psVector *logRmodel = NULL;
    psphotSatstarProfileCreate (source, &logRmodel, &logFmodel, logR, flux, Rmax);
    
    // XXX do something sensible here if the profile is crap

    // replace an existing profile
    psFree (source->satstar);
    source->satstar = pmSourceSatstarAlloc();
    source->satstar->Xo = Xo;
    source->satstar->Yo = Yo;
    source->satstar->Rmax = Rmax;
    source->satstar->logFmodel = logFmodel;
    source->satstar->logRmodel = logRmodel;

    // subtract the profile (false => subtract)
    psphotSatstarProfileOp (source, maskVal, 1.0, 0, false);

    psFree (logR);
    psFree (flux);

    return true;
}

// Take logR + flux and generate radial bins logRmodelOut, logFmodelOut
bool psphotSatstarProfileCreate (pmSource *source, psVector **logRmodelOut, psVector **logFmodelOut, psVector *logR, psVector *flux, float Rmax) {

  // we have log(radius) & log(flux).  find the median flux in log radial bins

  float logRmax = log10(Rmax);
  float logRdel = 0.1; // XXX user-define?

  psVector *logRmodel = psVectorAlloc (1 + (int) (logRmax / logRdel), PS_TYPE_F32);
  psVector *logFmodel = psVectorAlloc (1 + (int) (logRmax / logRdel), PS_TYPE_F32);

  pmSourceRadialProfileSortPair (logR, flux);

  psStats *fluxStats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);
  psVector *fluxVals = psVectorAllocEmpty (100, PS_TYPE_F32);

  int bin = 0;
  for (int i = 0; i < logRmodel->n; i++) {
    
    // bin (i) has log radius range (i*logRdel : (i+1)*logRdel)
    float lRmin = logRdel*(i + 0);
    float lRmax = logRdel*(i + 1);

    // reset the flux vector
    fluxVals->n = 0;
    psStatsInit (fluxStats);

    while (bin < logR->n && logR->data.F32[bin] < lRmax) {
      if (isfinite(flux->data.F32[bin])) {
	  psVectorAppend (fluxVals, flux->data.F32[bin]);
      }
      bin ++;
    }
    
    // we have the set of fluxes for this bin; find the median values
    
    float Rmin = pow(10.0, lRmin);
    float Rmax = pow(10.0, lRmax);

    float Rmean = (2.0/3.0) * (pow(Rmax, 3.0) - pow(Rmin, 3.0)) / (PS_SQR(Rmax) - PS_SQR(Rmin));
    logRmodel->data.F32[i] = log10(Rmean);

    float Area = M_PI * (PS_SQR(Rmax) - PS_SQR(Rmin));
    if (fluxVals->n < 0.25*Area) {
      logFmodel->data.F32[i] = NAN;
      continue;
    }
    
    psVectorStats (fluxStats, fluxVals, NULL, NULL, 0);
    if (fluxStats->robustMedian > 0.0) {
	logFmodel->data.F32[i] = log10(fluxStats->robustMedian);
    } else {
	logFmodel->data.F32[i] = -3.0;
    }
    // fprintf (stderr, "R: %f, F: %f +/- %f\n", Rmean, fluxStats->robustMedian, fluxStats->robustStdev);
  }

  // now how do i use this to subtract a model??
  *logRmodelOut = logRmodel;
  *logFmodelOut = logFmodel;
  
  // need to free stuff
  psFree (fluxStats);
  psFree (fluxVals);

  return true;
}

bool psphotSatstarProfileOp (pmSource *source, psImageMaskType maskVal, float FACTOR, pmModelOpMode mode, bool add) {

    int alt;
    float logRdel = 0.1;

    pmSourceSatstar *satstar = source->satstar ? source->satstar : (source->parent ? source->parent->satstar : NULL);
    psAssert (satstar, "null satstar");
    float Xc = satstar->Xo - source->pixels->col0 - 0.5;
    float Yc = satstar->Yo - source->pixels->row0 - 0.5;
    psVector *logRmodel = satstar->logRmodel;
    psVector *logFmodel = satstar->logFmodel;

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    if ((source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal)) continue;

	    float radius = hypot (ix - Xc, iy - Yc) ;
	    float logR = log10(radius);

	    int bin = (int)(logR / logRdel);

	    // we are going to interpolate if possible, or extrapolate if necessary
	    if (bin >= logRmodel->n - 1) bin = logRmodel->n - 1;
	    if (bin < 0) bin = 0;
      
	    // interpolate to the current radial position
	    // XXX BIG HACK : skip nan bins for now

	    float logF0 = logFmodel->data.F32[bin];
	    float logR0 = logRmodel->data.F32[bin];
	    if (!isfinite(logF0)) continue; 
    
	    // interpolate between closest two bins if possible, extrapolate on ends
	    if (logR < logR0) {
		alt = (bin > 0) ? bin - 1 : bin + 1;
	    } else {
		alt = (bin < logRmodel->n - 1) ? bin + 1 : bin - 1;
	    }

	    float logF1 = logFmodel->data.F32[alt];
	    float logR1 = logRmodel->data.F32[alt];
	    if (!isfinite(logF1)) continue; 

	    // XXX use linear flux, not logFlux
	    float logF = InterpolateValues (logR0, logF0, logR1, logF1, logR);
	    float flux = pow (10.0, logF);

	    if (mode & PM_MODEL_OP_NOISE) {
		if (add) {
		    source->variance->data.F32[iy][ix] += FACTOR * flux;
		} else {
		    source->variance->data.F32[iy][ix] -= FACTOR * flux;
		}
	    } else {
		if (add) {
		    source->pixels->data.F32[iy][ix] += flux;
		} else {
		    source->pixels->data.F32[iy][ix] -= flux;
		}
	    }
	}
    }
    return true;
}

// Take logR + flux and generate radial bins logRmodelOut, logFmodelOut
bool psphotSatstarPhotometry (pmSource *source) {

    if (!source->satstar) return false;

    psVector *logRmodel = source->satstar->logRmodel;
    psVector *logFmodel = source->satstar->logFmodel;
    
    float fluxTotal = 0.0;
    float logRdel = 0.1; // XXX user-define (or carry in satstar)

    // integrate flux in radial bins 
    for (int i = 0; i < logRmodel->n; i++) {
	// just add up the mean flux in each bin and multiply by the area of the bin
	
	float logF = logFmodel->data.F32[i];
	if (!isfinite(logF)) continue;
	float flux = pow(10.0, logF); // this is the mean flux per pixel (ie, surface brightness)

	// bin (i) has log radius range (i*logRdel : (i+1)*logRdel)
	float lRmin = logRdel*(i + 0);
	float lRmax = logRdel*(i + 1);

	float Rmin = pow(10.0, lRmin);
	float Rmax = pow(10.0, lRmax);

	float Area = M_PI * (PS_SQR(Rmax) - PS_SQR(Rmin));

	fluxTotal += flux * Area;
    }

    source->psfMag    = -2.5*log10(fluxTotal);
    source->psfMagErr = 0.05; 

    // XXX I have no idea of a realistic error on the photometry here.  the bottom line is that
    // the error is totally dominated by the loss of charge due to saturation.  I am not
    // modeling the profile in any real detail other than to follow the radial bins.  

    source->extMag    = NAN;
    source->apMag     = NAN;
    source->apMagRaw  = NAN;
    source->apFlux    = fluxTotal;
    source->apFluxErr = 0.05*fluxTotal;

    return true;
}

# if (0)
static bool skipDisplay = false;
bool psphotVisualRadialProfileSatstar (pmSource *source, psImageMaskType maskVal) {

    int kapaImage = -1;
    int kapaGraph = -1;
    Graphdata graphdata;

    if (!pmVisualTestLevel("psphot.satstar", 3)) {
	skipDisplay = true;
    }

    float Rmax = 320.0;

    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

    int nPts = source->pixels->numRows * source->pixels->numCols;
    psVector *rg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *Rg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *fg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
    psVector *Fg = psVectorAllocEmpty (nPts, PS_TYPE_F32);
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
		Fg->data.F32[ng] = source->pixels->data.F32[iy][ix];
		fg->data.F32[ng] = log10(Fg->data.F32[ng]);
		ng++;
	    }
	}
    }
    rg->n = ng;
    Rg->n = ng;
    fg->n = ng;
    Fg->n = ng;
    rb->n = nb;
    Rb->n = nb;
    fb->n = nb;

    if (!skipDisplay) {
	kapaImage = psphotKapaChannel (1);
	if (kapaImage == -1) return false;

	kapaGraph = psphotKapaChannel (2);
	if (kapaGraph == -1) return false;

	KapaSection section;  // put the positive profile in one and the residuals in another?

	// first section : mag vs CR nSigma
	section.dx = 1.0;
	section.dy = 0.5;
	section.x = 0.0;
	section.y = 0.0;
	section.bg = -1;
	section.name = NULL;
	psStringAppend (&section.name, "linlog");
	KapaSetSection (kapaGraph, &section);
	psFree (section.name);

	// first section : mag vs CR nSigma
	section.dx = 1.0;
	section.dy = 0.5;
	section.x = 0.0;
	section.y = 0.5;
	section.bg = -1;
	section.name = NULL;
	psStringAppend (&section.name, "loglog");
	KapaSetSection (kapaGraph, &section);
	psFree (section.name);

	KapaInitGraph (&graphdata);

	// ** linlog **
	KapaSelectSection (kapaGraph, "linlog");

	// examine sources to set data range
	graphdata.xmin =  -0.05;
	graphdata.xmax = Rmax + 0.05;
	graphdata.ymin = -0.05;
	graphdata.ymax = +8.05;
	KapaSetLimits (kapaGraph, &graphdata);

	KapaSetFont (kapaGraph, "helvetica", 14);
	KapaBox (kapaGraph, &graphdata);
	KapaSendLabel (kapaGraph, "radius (pixels)", KAPA_LABEL_XM);
	KapaSendLabel (kapaGraph, "log flux (counts)", KAPA_LABEL_YM);

	graphdata.color = KapaColorByName ("black");
	graphdata.ptype = 2;
	graphdata.size = 0.5;
	graphdata.style = 2;
	KapaPrepPlot (kapaGraph, ng, &graphdata);
	KapaPlotVector (kapaGraph, ng, rg->data.F32, "x");
	KapaPlotVector (kapaGraph, ng, fg->data.F32, "y");

	graphdata.color = KapaColorByName ("red");
	graphdata.ptype = 0;
	graphdata.size = 0.3;
	graphdata.style = 2;
	KapaPrepPlot (kapaGraph, nb, &graphdata);
	KapaPlotVector (kapaGraph, nb, rb->data.F32, "x");
	KapaPlotVector (kapaGraph, nb, fb->data.F32, "y");

	// ** loglog **
	KapaSelectSection (kapaGraph, "loglog");

	// examine sources to set data range
	graphdata.xmin = -1.51;
	graphdata.xmax = log10(Rmax) + 0.02;
	graphdata.ymin = -5.05;
	graphdata.ymax = +8.05;
	graphdata.color = KapaColorByName ("black");
	KapaSetLimits (kapaGraph, &graphdata);

	KapaSetFont (kapaGraph, "helvetica", 14);
	KapaBox (kapaGraph, &graphdata);
	KapaSendLabel (kapaGraph, "log radius (pixels)", KAPA_LABEL_XM);
	KapaSendLabel (kapaGraph, "log flux (counts)", KAPA_LABEL_YM);

	graphdata.color = KapaColorByName ("black");
	graphdata.ptype = 2;
	graphdata.size = 0.5;
	graphdata.style = 2;
	KapaPrepPlot (kapaGraph, ng, &graphdata);
	KapaPlotVector (kapaGraph, ng, Rg->data.F32, "x");
	KapaPlotVector (kapaGraph, ng, fg->data.F32, "y");

	graphdata.color = KapaColorByName ("red");
	graphdata.ptype = 0;
	graphdata.size = 0.3;
	graphdata.style = 2;
	KapaPrepPlot (kapaGraph, nb, &graphdata);
	KapaPlotVector (kapaGraph, nb, Rb->data.F32, "x");
	KapaPlotVector (kapaGraph, nb, fb->data.F32, "y");
    }

    // measure the radial profile 
    psVector *logFmodel = NULL;
    psVector *logRmodel = NULL;
    psphotTestRadialModel (source, &logRmodel, &logFmodel, Rg, Fg, Rmax);
    
    if (!skipDisplay) {
	graphdata.color = KapaColorByName ("red");
	graphdata.ptype = 2;
	graphdata.size = 1.0;
	graphdata.style = 2;
	KapaPrepPlot (kapaGraph, logRmodel->n, &graphdata);
	KapaPlotVector (kapaGraph, logRmodel->n, logRmodel->data.F32, "x");
	KapaPlotVector (kapaGraph, logRmodel->n, logFmodel->data.F32, "y");
    }

    // subtract the model from the images
    psphotTestRadialModelSub (source, logRmodel, logFmodel, Xo, Yo, Rmax, maskVal);

    psFree (logRmodel);
    psFree (logFmodel);

    if (!skipDisplay && source->modelPSF) {
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
	    fmaj->data.F32[i] = log10(source->modelPSF->modelFunc (NULL, params, coord));

	    coord->data.F32[0] = r;
	    coord->data.F32[1] = 0.0;
	    fmin->data.F32[i] = log10(source->modelPSF->modelFunc (NULL, params, coord));
	}
	psFree (coord);
	psFree (params);

	float FWHM_MAJOR = 2.0*source->modelPSF->modelRadius (source->modelPSF->params, 0.5*source->modelPSF->params->data.F32[PM_PAR_I0]);
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
	KapaSelectSection (kapaGraph, "linlog");
	KapaGetGraphData (kapaGraph, &graphdata);
	graphdata.color = KapaColorByName ("blue");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (kapaGraph, rmod->n, &graphdata);
	KapaPlotVector (kapaGraph, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (kapaGraph, rmod->n, fmin->data.F32, "y");
	plotline (kapaGraph, &graphdata, graphdata.xmin, logHM, graphdata.xmax, logHM);
	plotline (kapaGraph, &graphdata, 0.5*FWHM_MINOR, graphdata.ymin, 0.5*FWHM_MINOR, graphdata.ymax);
	graphdata.ltype = 1;
	plotline (kapaGraph, &graphdata, 0.5*MOMENTS_MINOR, graphdata.ymin, 0.5*MOMENTS_MINOR, graphdata.ymax);
	graphdata.ltype = 0;
	
	graphdata.color = KapaColorByName ("green");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (kapaGraph, rmod->n, &graphdata);
	KapaPlotVector (kapaGraph, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (kapaGraph, rmod->n, fmaj->data.F32, "y");
	plotline (kapaGraph, &graphdata, 0.5*FWHM_MAJOR, graphdata.ymin, 0.5*FWHM_MAJOR, graphdata.ymax);
	graphdata.ltype = 1;
	plotline (kapaGraph, &graphdata, 0.5*MOMENTS_MAJOR, graphdata.ymin, 0.5*MOMENTS_MAJOR, graphdata.ymax);
	graphdata.ltype = 0;
	
	for (int i = 0; i < rmod->n; i++) {
	    rmod->data.F32[i] = log10(rmod->data.F32[i]);
	}

	// ** loglog **
	KapaSelectSection (kapaGraph, "loglog");
	KapaGetGraphData (kapaGraph, &graphdata);
	graphdata.color = KapaColorByName ("blue");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (kapaGraph, rmod->n, &graphdata);
	KapaPlotVector (kapaGraph, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (kapaGraph, rmod->n, fmin->data.F32, "y");

	graphdata.color = KapaColorByName ("green");
	graphdata.ptype = 0;
	graphdata.size = 0.0;
	graphdata.style = 0;
	KapaPrepPlot   (kapaGraph, rmod->n, &graphdata);
	KapaPlotVector (kapaGraph, rmod->n, rmod->data.F32, "x");
	KapaPlotVector (kapaGraph, rmod->n, fmaj->data.F32, "y");

	psFree (rmod);
	psFree (fmin);
	psFree (fmaj);
    }

    if (!skipDisplay) {
	KiiCenter (kapaImage, source->peak->xf, source->peak->yf, 1);
	psphotVisualScaleImage (kapaImage, (psImage *) source->pixels->parent, NULL, "image", 1.0, 1);
	KiiOverlay overlay;
	overlay.x = source->peak->xf;
	overlay.y = source->peak->yf;
	overlay.dx = 5;
	overlay.dy = 5;
	overlay.angle = 0.0;
	overlay.type = KiiOverlayTypeByName ("circle");
	KiiLoadOverlay (kapaImage, &overlay, 1, "red");
	overlay.x = source->moments->Mx;
	overlay.y = source->moments->My;
	overlay.dx = 8;
	overlay.dy = 8;
	overlay.angle = 0.0;
	overlay.type = KiiOverlayTypeByName ("circle");
	KiiLoadOverlay (kapaImage, &overlay, 1, "blue");
    }

    psFree (rg);
    psFree (Rg);
    psFree (fg);
    psFree (Fg);
    psFree (rb);
    psFree (Rb);
    psFree (fb);

    if (skipDisplay) return true;

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10];
    fprintf (stdout, "[q]uit satstar? [e]rase and continue? [o]verplot and continue? [s]kip rest of stars? : ");
    if (!fgets(key, 8, stdin)) {
      psWarning("Unable to read option");
    }
    if (key[0] == 'e') {
      KapaClearPlots (kapaGraph);
    }
    if (key[0] == 'q') {
	return false;
    }
    if (key[0] == 's') {
	skipDisplay = true;
    }
    return true;
}
# endif

// only valid for F32
bool psImageQuickStats (QuickStats *stats, psImage *image, psRegion region) {

    psAssert (image->type.type == PS_TYPE_F32, "unsupported image type");

    int Npix = (region.y1 - region.y0) * (region.x1 - region.x0);
    psVector *tmp = psVectorAllocEmpty (Npix, PS_TYPE_F32);

    int bin = 0;
    for (int iy = region.y0; iy < region.y1; iy++) {
	for (int ix = region.x0; ix < region.x1; ix++) {
	    if (!isfinite(image->data.F32[iy][ix])) continue;
	    tmp->data.F32[bin] = image->data.F32[iy][ix];
	    bin ++;
	}
    }
    tmp->n = bin;

    if (bin < 1) {
	stats->min     = NAN;
	stats->lower20 = NAN;
	stats->upper20 = NAN;
	stats->max     = NAN;
	stats->Npts    = 0;
	psFree (tmp);
	return true;
    }


    psVectorSortInPlace (tmp);

    int N20  = 0.2*tmp->n;
    int N80  = 0.8*tmp->n;
    int Nmax = tmp->n - 1;

    stats->min     = tmp->data.F32[0];
    stats->lower20 = tmp->data.F32[N20];
    stats->upper20 = tmp->data.F32[N80];
    stats->max     = tmp->data.F32[Nmax];
    stats->Npts    = bin;
    psFree (tmp);

    return true;
}

// the return state indicates if any sources were actually subtracted
bool psphotAddOrSubSatstarsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int fileIndex, psMetadata *recipe, bool add) {

    bool status;
    bool modified = false;

    psTimerStart ("psphot.deblend.sat");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, fileIndex); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    // if no work to do, should just return false
    if (!sources) return false;

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping satstar blend");
	return false;
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // examine sources in decreasing SN order
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

	if (!(source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE)) continue;

	// tell the calling function that we modified the image
	modified = true;

	if (!psphotSatstarProfileOp (source, maskVal, 1.0, 0, add)) continue;
    }

    psLogMsg ("psphot", PS_LOG_DETAIL, "satstar op: %f sec\n", psTimerMark ("psphot.deblend.sat"));
    return modified;
}

