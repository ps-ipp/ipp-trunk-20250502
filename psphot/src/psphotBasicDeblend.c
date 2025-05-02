# include "psphotInternal.h"

// for now, let's store the detections on the readout->analysis for each readout
bool psphotBasicDeblend (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotBasicDeblendReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on basic deblend analysis for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotBasicDeblendReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int fileIndex) {

    int N;
    bool status;
    float threshold;
    pmSource *source, *testSource;

    psTimerStart ("psphot.deblend.basic");

    int Nblend = 0;

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
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping basic deblend");
	return true;
    }

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    float FRACTION = psMetadataLookupF32 (&status, recipe, "DEBLEND_PEAK_FRACTION");
    if (!status) FRACTION = 0.25;

    float NSIGMA   = psMetadataLookupF32 (&status, recipe, "DEBLEND_SKY_NSIGMA");
    if (!status) NSIGMA = 5.0;

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

        if (source->mode & PM_SOURCE_MODE_BLEND) continue;

        // temporary array for overlapping objects we find
        psArray *overlap = psArrayAllocEmpty (100);

        // search backwards for overlapping sources
        for (int j = N - 1; j >= 0; j--) {
            testSource = sources->data[j];
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
            testSource = sources->data[j];
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

        // this source has overlapping neighbors, check for actual blends
        // generate source contour (1/4 peak counts)
        // set the threshold based on user inputs

        // threshold is fraction of the source peak flux
        // image is background subtracted; source->moments->Sky should always be 0.0
        threshold = FRACTION * sqrt(source->peak->detValue);
        // threshold is no less than NSIGMA
        threshold = PS_MAX (threshold, NSIGMA);

        // generate a basic contour (set of x,y coordinates at-or-below flux level)
        psArray *contour = pmSourceContour (source->pixels, source->peak->x, source->peak->y, threshold);
        if (contour == NULL) {
            psFree (overlap);
            continue;
        }

        // the source contour consists of two vectors, xv and yv.  the contour is
        // a series of coordinate pairs, (xv[i],yv[i]) & (xv[i+1],yv[i+1]).  both
        // coordinate pairs have the same yv[] value, with xv[i] corresponding to
        // the left boundary and xv[i+1] corresponding to the right boundary

	// a blend can only be associated with one primary source

        psVector *xv = contour->data[0];
        psVector *yv = contour->data[1];
        for (int k = 0; k < overlap->n; k++) {
            testSource = overlap->data[k];
	    if (testSource->mode & PM_SOURCE_MODE_BLEND) continue;
            if (testSource->peak->rawFlux > source->peak->rawFlux) continue;
            for (int j = 0; j < xv->n; j+=2) {
                if (fabs(yv->data.F32[j] - testSource->peak->y) > 0.5) continue;
                if (xv->data.F32[j+0] > testSource->peak->x) break;
                if (xv->data.F32[j+1] < testSource->peak->x) break;

                testSource->mode |= PM_SOURCE_MODE_BLEND;

                // add this to the list of source->blends
                if (source->blends == NULL) {
                    source->blends = psArrayAllocEmpty (16);
                }
                psArrayAdd (source->blends, 16, testSource);

                Nblend ++;
                j = xv->n;
            }
        }
        psFree (overlap);
        psFree (contour);
    }
    psLogMsg ("psphot.deblend", PS_LOG_INFO, "identified %d blended objects: %f sec\n", Nblend, psTimerMark ("psphot.deblend.basic"));

    psFree (SN);
    psFree (index);
    return true;
}
