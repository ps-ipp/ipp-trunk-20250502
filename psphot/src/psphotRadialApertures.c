# include "psphotInternal.h"

bool psphotRadialAperturesSortFlux (psVector *radius, psVector *pixFlux, psVector *pixVar);

// this function measures the radial aperture fluxes for the set of readouts.  this function
// may be called multiple times, presumably for different versions of PSF-matched or unmatched images.  

// for now, let's store the detections on the readout->analysis for each readout
bool psphotRadialApertures (pmConfig *config, const pmFPAview *view, const char *filerule, int entry)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Radial Apertures ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full non-linear fits / extended source analysis?
    if (!psMetadataLookupBool (&status, recipe, "RADIAL_APERTURES")) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping radial apertures\n");
	return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

	if (!psphotRadialAperturesReadout (config, view, filerule, i, recipe, entry)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on measure extended source aperture-like parameters for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// these values are used by all threads repeatedly (and are not modified)
static psVector *aperRadii = NULL;
static psVector *aperRadii2 = NULL;
static float outerRadius = NAN;
static float SN_LIM = NAN;
static int RADIAL_AP_MIN = 5;
static psImageMaskType maskVal = 0;

// aperture-like measurements for extended sources
// flux in simple, circular apertures
// 'entry' tells us which of the matched-PSF images we are working on (0 == unmatched image, also non-stack psphot)
bool psphotRadialAperturesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, int entry) {

    bool status;
    int Nradial = 0;

    // perform full non-linear fits / extended source analysis?
    if (!psMetadataLookupBool (&status, recipe, "RADIAL_APERTURES")) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping radial apertures\n");
	return true;
    }

    psTimerStart ("psphot.radial");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");
    
    if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "skipping radial aptertures for input file %d", index);
        return true;
    }


    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
	return true;
    }

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // aperRadii stores the upper bounds of the annuli
    // XXX keep the same name here as for the petrosian / elliptical apertures?
    aperRadii = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.UPPER");
    psAssert (aperRadii, "annular bins (RADIAL.ANNULAR.BINS.UPPER) are not defined in the recipe");
    psAssert (aperRadii->n, "no valid annular bins (RADIAL.ANNULAR.BINS.UPPER) are define");

    outerRadius = aperRadii->data.F32[aperRadii->n - 1];

    // save the R^2 values as well for quicker comparison
    aperRadii2 = psVectorAlloc(aperRadii->n, PS_TYPE_F32);
    for (int i = 0; i < aperRadii->n; i++) {
	aperRadii2->data.F32[i] = PS_SQR(aperRadii->data.F32[i]);
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // S/N limit to perform full non-linear fits
    SN_LIM = psMetadataLookupF32 (&status, recipe, "RADIAL_APERTURES_SN_LIM");

    // S/N limit to perform full non-linear fits
    RADIAL_AP_MIN = psMetadataLookupS32 (&status, recipe, "RADIAL.ANNULAR.BINS.MIN");
    if (!status) {
      RADIAL_AP_MIN = 5;
    }

    // source analysis is done in S/N order (brightest first)
    // XXX are we getting the objects out of order? does it matter?
    sources = psArraySort (sources, pmSourceSortByFlux);

    // XXX make this consistent with entry 0 == unmatched
    int nEntry = 1;
    psVector *fwhmValues = psMetadataLookupVector(&status, readout->analysis, "STACK.PSF.FWHM.VALUES");
    if (fwhmValues) {
	psAssert (entry < fwhmValues->n, "inconsistent matched-PSF entry");
	nEntry = fwhmValues->n;
    }
    if (entry > 0) {
	psLogMsg ("psphot", PS_LOG_DETAIL, "Radial Apertures for matched image %s : PSF FWHM = %f pixels\n", file->name, fwhmValues->data.F32[entry]);
    } else {
	psLogMsg ("psphot", PS_LOG_DETAIL, "Radial Apertures for unmatched image %s\n", file->name);
    }

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion *AnalysisRegion = psRegionAlloc(0,0,0,0);
    *AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (*AnalysisRegion)) psAbort("analysis region mis-defined");

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_RADIAL_APERTURES");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, AnalysisRegion);
            PS_ARRAY_ADD_SCALAR(job->args, entry,  PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, nEntry, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nradial

	    // set this to 0 to run without threading
# if (1)	    
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
                return false;
            } 
# else
	    if (!psphotRadialApertures_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
		return false;
	    }
	    psScalar *scalar = NULL;
	    scalar = job->args->data[5];
	    Nradial += scalar->data.S32;
	    psFree(job);
# endif
	}

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	    psFree(AnalysisRegion);
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
		psScalar *scalar = NULL;
		scalar = job->args->data[5];
		Nradial += scalar->data.S32;
            }
            psFree(job);
	}
    }
    psFree (cellGroups);
    psFree(AnalysisRegion);
    psFree (aperRadii2);

    psLogMsg ("psphot", PS_LOG_WARN, "radial source apertures: %f sec for %d objects\n", psTimerMark ("psphot.radial"), Nradial);
    return true;
}
 
bool psphotRadialApertures_Threaded (psThreadJob *job) {

    int Nradial = 0;

    // arguments: readout, sources, models, region, psfSize, maskVal, markVal
    pmReadout *readout      = job->args->data[0];
    psArray *sources        = job->args->data[1];
    psRegion *region        = job->args->data[2];
    int entry               = PS_SCALAR_VALUE(job->args->data[3],S32); // which psf-matched image are we working on? (0 == unmatched)
    int nEntry              = PS_SCALAR_VALUE(job->args->data[4],S32); // total number of psf-matched images + 1 unmatched

    // storage for the derived pixel values (these are passed into psphotRadialApertureSource)
    psVector *pixRadius2 = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *pixFlux    = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *pixVar     = psVectorAllocEmpty(100, PS_TYPE_F32);

    // choose the sources of interest
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// if we have checked the source validity on the basis of the object set, then 
	// we either skip these tests below or we skip the source completely
	if (source->tmpFlags & PM_SOURCE_TMPF_RADIAL_SKIP) continue;
	if (source->tmpFlags & PM_SOURCE_TMPF_RADIAL_KEEP) goto keepSource;

	// skip PSF-like and non-astronomical objects
	if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	if (source->mode & PM_SOURCE_MODE_DEFECT) continue;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

	// XXX measure radial apertures even for saturated stars
	// if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;

	// limit selection to some SN limit
	assert (source->peak); // how can a source not have a peak?
	if (sqrt(source->peak->detValue) < SN_LIM) continue;

	// limit selection by analysis region
	if (source->peak->x < region->x0) continue;
	if (source->peak->y < region->y0) continue;
	if (source->peak->x > region->x1) continue;
	if (source->peak->y > region->y1) continue;

    keepSource:

	// allocate pmSourceExtendedParameters, if not already defined
	// XXX check that nPSFsizes is consistent with targets
	if (source->parent) {
	    if (!source->parent->radialAper) {
		source->parent->radialAper = psArrayAlloc(nEntry);
	    }
	} else {
	    if (!source->radialAper) {
		source->radialAper = psArrayAlloc(nEntry);
	    }
	}

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	Nradial ++;

	if (!psphotRadialApertureSource (source, readout, entry, pixRadius2, pixFlux, pixVar)) {
	    psTrace ("psphot", 5, "failed to extract radial profile for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
	} else {
	    source->mode |= PM_SOURCE_MODE_RADIAL_FLUX;
	}

	// re-subtract the object, leave local sky
	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }
    psScalar *scalar = job->args->data[5];
    scalar->data.S32 = Nradial;

    psFree (pixRadius2);
    psFree (pixFlux);
    psFree (pixVar);

    return true;
}

bool psphotRadialApertureSource (pmSource *source, pmReadout *readout, int entry, psVector *pixRadius2, psVector *pixFlux, psVector *pixVar) {
					    
    // if we are a child source, save the results to the parent source radial aperture array
    psArray *radialAperSet = source->radialAper;
    if (source->parent) {
	radialAperSet = source->parent->radialAper;
    }
    psAssert(radialAperSet, "this should be defined before calling");
    psAssert(radialAperSet->data[entry] == NULL, "why is this already defined?");

    pmSourceRadialApertures *radialAper = pmSourceRadialAperturesAlloc ();
    radialAperSet->data[entry] = radialAper;

    // find the largest aperture of interest (use only apertures with inner radii <=
    // source->skyRadius, as long as i >= RADIAL_AP_MIN
    int lastAp = aperRadii->n;
    for (int i = RADIAL_AP_MIN; i < aperRadii->n; i++) {
	if (aperRadii->data.F32[i] < source->skyRadius) continue; // measure out to this radius
	lastAp = i + 1;
	break;
    }

    // outer-most radius for initial truncation
    float Rmax  = aperRadii->data.F32[lastAp - 1];
    float Rmax2 = PS_SQR(Rmax);

    // in this function, the operatins are relative to the full image (readout->image, etc)

    float xCM = NAN, yCM = NAN;
    if (pmSourcePositionUseMoments(source)) {
	xCM = source->moments->Mx; // index coord of peak in readout
	yCM = source->moments->My; // index coord of peak in readout
    } else {
	xCM = source->peak->xf; // index coord of peak in readout
	yCM = source->peak->yf; // index coord of peak in readout
    }

    int Nx = readout->image->numCols;
    int Ny = readout->image->numRows;

    pixRadius2->n = 0;
    pixFlux->n = 0;
    pixVar->n = 0;

    // one pass through the pixels to select the valid pixels and calculate R^2
    for (int iy = -Rmax; iy < Rmax + 1; iy++) {

	float yDiff = iy + 0.5 + yCM;  // y-coordinate at this offse
	int yPix = (int) yDiff;

	if (yPix < 0) continue;
	if (yPix > Ny - 1) continue;
	if (fabs(iy) > Rmax) continue;

	float *vPix = readout->image->data.F32[yPix];
	float *vWgt = readout->variance->data.F32[yPix];
	psImageMaskType  *vMsk = readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[yPix];

	for (int ix = -Rmax; ix < Rmax + 1; ix++) {

	    float xDiff = ix + 0.5 + xCM;  // x-coordinate at this offse
	    int xPix = (int) xDiff;
	    
	    if (xPix < 0) continue;
	    if (xPix > Nx - 1) continue;
	    if (fabs(ix) > Rmax) continue;
	    
	    if (vMsk[xPix] & maskVal) continue;
	    if (isnan(vPix[xPix])) continue;

	    // radius is just a function of (xDiff, yDiff)
	    float r2  = PS_SQR(ix) + PS_SQR(iy);
	    if (r2 > Rmax2) continue;

	    psVectorAppend(pixRadius2, r2);
	    psVectorAppend(pixFlux, vPix[xPix]);
	    psVectorAppend(pixVar, vWgt[xPix]);
	}
    }

    psVector *flux    = psVectorAlloc(aperRadii->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *fluxErr = psVectorAlloc(aperRadii->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *fluxStd = psVectorAlloc(aperRadii->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *fill    = psVectorAlloc(aperRadii->n, PS_TYPE_F32); // surface brightness of radial bin

    // init the apertures of interest to 0.0, the rest go to NAN
    psVectorInit (flux,    0.0);
    psVectorInit (fluxStd, 0.0);
    psVectorInit (fluxErr, 0.0);
    psVectorInit (fill,    0.0);
    for (int i = lastAp; i < flux->n; i++) {
	flux->data.F32[i] = NAN;
	fluxStd->data.F32[i] = NAN;
	fluxErr->data.F32[i] = NAN;
	fill->data.F32[i] = NAN;
    }

    float *rPix2 = pixRadius2->data.F32;
    for (int i = 0; i < pixRadius2->n; i++, rPix2++) {

	int j = 0;
	float *aRad2 = aperRadii2->data.F32;
	for (; (*aRad2 < *rPix2) && (j < lastAp); j++, aRad2++);

	// XXX I can speed this up by only saving this single aperture
	for (; j < lastAp; j++, aRad2++) {
	    flux->data.F32[j]    += pixFlux->data.F32[i];
	    fluxStd->data.F32[j] += PS_SQR(pixFlux->data.F32[i]);
	    fluxErr->data.F32[j] += pixVar->data.F32[i];
	    fill->data.F32[j]    += 1.0;
	}
    }

    /* for each radial bin, R(i), we measure:
       1) the flux within that aperture: F(i) = \sum_{r_j<R_i}(F_j)
       2) the fractional fill factor (count of valid pixels / effective area of the aperture
       3) the error on the flux within that aperture
    */

    for (int i = 0; i < lastAp; i++) {
	// calculate the total flux for bin 'nOut'
	float Area = M_PI*aperRadii2->data.F32[i];

	int nPix = fill->data.F32[i];
	float SBmean = flux->data.F32[i] / nPix;
	float SBstdv = sqrt((fluxStd->data.F32[i] / nPix) - PS_SQR(SBmean));

	// XXX report the total flux or the mask-corrected flux?
	// flux->data.F32[i]    = SBmean * Area;
	// fluxErr->data.F32[i] = sqrt(fluxErr->data.F32[i]) * Area / Pinx;

	fluxErr->data.F32[i] = sqrt(fluxErr->data.F32[i]);
	fluxStd->data.F32[i] = SBstdv * Area;
	fill->data.F32[i] /= Area;

	psTrace ("psphot", 5, "radial bins: %3d  %5.1f : %8.1f +/- %7.2f : %8.1f +/- %8.1f : %4.2f %6.1f\n", 
		 i, aperRadii->data.F32[i], flux->data.F32[i], fluxErr->data.F32[i], SBmean, SBstdv, fill->data.F32[i], Area);
    }
    
# if (1)
    radialAper->flux = flux;
    radialAper->fluxStdev = fluxStd;
    radialAper->fluxErr = fluxErr;
    radialAper->fill = fill;
# else
    // XXX TEST
    psFree(flux);
    psFree(fluxStd);
    psFree(fluxErr);
    psFree(fill);
# endif

    return true;
}

/*** below is a test to use a sort to speed this up, not very successfully ***/

static int nCalls = 0;
static int nPass = 0;
static int nPix = 0;

bool psphotRadialApertureSource_With_Sort (pmSource *source, psMetadata *recipe, psImageMaskType maskVal, const psVector *radMax, int entry) {

    psAssert(source->radialAper->data[entry] == NULL, "why is this already defined?");

    pmSourceRadialApertures *radialAper = pmSourceRadialAperturesAlloc ();
    source->radialAper->data[entry] = radialAper;

    psVector *pixRadius  = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *pixFlux = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *pixVar  = psVectorAllocEmpty(100, PS_TYPE_F32);

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {

	    // 0.5 PIX: get pixRadius as a function of pixel coord
	    float x = ix + 0.5 - source->peak->xf + source->pixels->col0;
	    float y = iy + 0.5 - source->peak->yf + source->pixels->row0;

	    float r = hypot(x, y);

	    psVectorAppend(pixRadius, r);
	    psVectorAppend(pixFlux, source->pixels->data.F32[iy][ix]);
	    psVectorAppend(pixVar, source->variance->data.F32[iy][ix]);
	    nPix ++;
	    // if (nPix % 10000 == 0) {fprintf (stderr, "?");}
	}
    }
    psphotRadialAperturesSortFlux(pixRadius, pixFlux, pixVar);

    psVector *flux    = psVectorAllocEmpty(radMax->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *fluxErr = psVectorAllocEmpty(radMax->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *fill    = psVectorAllocEmpty(radMax->n, PS_TYPE_F32); // surface brightness of radial bin

    psVectorInit (flux,    0.0);
    psVectorInit (fluxErr, 0.0);
    psVectorInit (fill,    0.0);

    float fluxSum = 0.0;
    float varSum = 0.0;
    int nPixSum = 0;

    bool done = false;
    int nOut = 0;
    float Rmax = radMax->data.F32[nOut];

    // XXX assume (or enforce) that the bins are contiguous and non-overlapping (Rmax[i] = Rmin[i+1])
    for (int i = 0; !done && (i < pixRadius->n); i++) {
	if (pixRadius->data.F32[i] > Rmax) {
	    // calculate the total flux for bin 'nOut'
	    float Area = M_PI*PS_SQR(Rmax);
	    flux->data.F32[nOut] = fluxSum;
	    fluxErr->data.F32[nOut] = sqrt(varSum);
	    fill->data.F32[nOut] = nPixSum / Area;

	    psTrace ("psphot", 5, "radial bins: %3d  %5.1f : %8.1f +/- %7.2f : %4.2f %6.1f\n", 
		     nOut, radMax->data.F32[nOut], flux->data.F32[nOut], fluxErr->data.F32[nOut], fill->data.F32[nOut], Area);

	    nPass ++;
	    // if (nPass % 1000 == 0) {fprintf (stderr, "!");}

	    nOut ++;
	    if (nOut >= radMax->n) break;
	    Rmax = radMax->data.F32[nOut];
	}
	fluxSum += pixFlux->data.F32[i];
	varSum += pixVar->data.F32[i];
	nPixSum ++;
    }
    flux->n = fluxErr->n = fill->n = nOut;
    
    radialAper->flux = flux;
    radialAper->fluxErr = fluxErr;
    radialAper->fill = fill;

    psFree (pixRadius);
    psFree (pixFlux);
    psFree (pixVar);

    nCalls ++;
    // if (nCalls % 100 == 0) {fprintf (stderr, "*");}
    return true;
}

// *** pmSourceRadialProfileSortPair is a utility function for sorting a pair of vectors
# define COMPARE_VECT(A,B) (radius->data.F32[A] < radius->data.F32[B])
# define SWAP_VECT(TYPE,A,B) {					\
	float tmp;						\
	if (A != B) {						\
	    tmp = radius->data.F32[A];				\
	    radius->data.F32[A] = radius->data.F32[B];		\
	    radius->data.F32[B] = tmp;				\
	    tmp = pixFlux->data.F32[A];				\
	    pixFlux->data.F32[A] = pixFlux->data.F32[B];	\
	    pixFlux->data.F32[B] = tmp;				\
	    tmp = pixVar->data.F32[A];				\
	    pixVar->data.F32[A] = pixVar->data.F32[B];		\
	    pixVar->data.F32[B] = tmp;				\
	}							\
    }

bool psphotRadialAperturesSortFlux (psVector *radius, psVector *pixFlux, psVector *pixVar) {

    // sort the vector set by the radius
    PSSORT (radius->n, COMPARE_VECT, SWAP_VECT, NONE);
    return true;
}

