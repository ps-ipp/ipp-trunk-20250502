/** @file psastroMosaicZeroPoint.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroZeroPoint (pmConfig *config) {

    bool status;
    float zeropt, exptime;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float refminMag,refmaxMag;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, false, "Can't find PSASTRO recipe!\n");
        return false;
    }

    // recipe options
    bool byExposure = psMetadataLookupBool (&status, recipe, "ZERO.POINT.BY.EXPOSURE");

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, false, "Can't find input data!\n");
        return false;
    }
    pmFPA *fpa = input->fpa;

    pmFPAview *view = pmFPAviewAlloc (0);

    // given the existing per-chip astrometry, determine matches between raw and ref stars
    // is this needed? yes, if we didn't do SingleChip astrometry first
    // if (!psastroMosaicSetMatch (fpa, recipe, 0)) {
    //     psError(PSASTRO_ERR_UNKNOWN, false, "failed to match raw and ref stars for mosaic");
    //     return false;
    // }

    // XXX eventually: look up the photcode info from the recipe, use the ZEROPT and A_0 terms
    // to determine transparency = Mref - Mraw - 2.5*log10(exptime) - ZEROPT - A_0*Color_ref
    // to get there, we need: a) getstar to write out the color-term magnitudes and b)  the
    // pmAstromObj to carry a color value as well.

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    // MEH -- RefMagLimit for ZP with photcode option (merr probably better but not an option) 
    // -- defualt is to use general config value (no hardcode..)
    if (!psastroZeroPointRefMagLimitFromRecipe (&refminMag, &refmaxMag, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load ref min,maxMag data from recipe");
	return false;
    }

    // if we measure the zero point by exposure, accumulate the dMag values here:
    psVector *dMag = NULL;
    // MEH -- also the number of refcat sources used
    int zpt_nref = 0;

    float fpaZP = 0.0;                  // Average zero point
    int numZP = 0.0;                    // Number of measurements

    // this loop selects the matched stars for all chips
    // XXX optionally measure zero point for entire exposure in a single statistic
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            // XXX there can only be one readout per chip, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // calculate dMag for the matched stars
                //dMag = psastroZeroPointReadoutAccum (dMag, readout, exptime);
                // MEH -- additional limits should be available for ZP if just robust mean and w/ stack skycal 
		dMag = psastroZeroPointReadoutAccum (dMag, readout, exptime, refminMag, refmaxMag);
                if (!byExposure) {
                    // calculate dMag for the matched stars just for this readout (well, chip)
                    psMetadata *header = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
                    psastroZeroPointAnalysis (header, dMag, zeropt, recipe);
		    // MEH -- per readout N refcat, min/max mag  -- added here, not part of median/edge process
		    zpt_nref += dMag->n;
		    psMetadataAddS32 (header, PS_LIST_TAIL, "ZPT_NREF", PS_META_REPLACE, "N refcat sources for zero point", dMag->n);
		    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MIN", PS_META_REPLACE, "min refcat mag for zero point", refminMag);
		    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MAX", PS_META_REPLACE, "max refcat mag for zero point", refmaxMag);
                    psFree (dMag);
                    dMag = NULL;

                    float zp = psMetadataLookupF32(NULL, header, "ZPT_OBS");
                    if (isfinite(zp)) {
                        fpaZP += zp;
                        numZP++;
                    }



                }
            }
        }
    }

    if (byExposure) {
        psMetadata *header = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
        if (!header) {
            header = psMetadataAlloc ();
            psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", header);
            psFree (header);
        }
        psastroZeroPointAnalysis (header, dMag, zeropt, recipe);
	//MEH -- all N refcat sources here
	zpt_nref = dMag->n;
        psFree (dMag);
        dMag = NULL;

        float zptObs = psMetadataLookupF32 (&status, header, "ZPT_OBS");
        float zptRef = psMetadataLookupF32 (&status, header, "ZPT_REF");
        float zptErr = psMetadataLookupF32 (&status, header, "ZPT_ERR");
        float zptOff = psMetadataLookupF32 (&status, header, "ZPT_OFF");

        fpaZP = zptObs;
        numZP = 1;

        // copy the zero point metadata from the fpa header to the chip headers
        for (int i = 0; i < fpa->chips->n; i++) {
            pmChip *chip = fpa->chips->data[i];
            if (!chip) continue;
            if (!chip->process) continue;
            if (!chip->file_exists) continue;

            for (int j = 0; j < chip->cells->n; j++) {
                pmCell *cell = chip->cells->data[j];
                if (!cell) continue;
                if (!cell->process) continue;
                if (!cell->file_exists) continue;

                for (int k = 0; k < cell->readouts->n; k++) {
                    pmReadout *readout = cell->readouts->data[k];
                    if (!readout) continue;
                    if (!readout->data_exists) continue;

                    // calculate dMag for the matched stars just for this readout (well, chip)
                    psMetadata *header = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
                    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_OBS", PS_META_REPLACE, "measured zero point",  zptObs);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_REF", PS_META_REPLACE, "reference zero point", zptRef);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_ERR", PS_META_REPLACE, "error on zero point",  zptErr);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_OFF", PS_META_REPLACE, "zero point offset",    zptOff);
                    //MEH -- per readout  N refcat, MIN/MAX for consistency
                    psMetadataAddS32 (header, PS_LIST_TAIL, "ZPT_NREF", PS_META_REPLACE, "N refcat sources for zero point", zpt_nref);
		    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MIN", PS_META_REPLACE, "min refcat mag for zero point", refminMag);
		    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MAX", PS_META_REPLACE, "max refcat mag for zero point", refmaxMag);
                }
            }
        }
    }

    // MEH add N refcat, min/max mag to primary header 
    psMetadata *header = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
    if (!header) {
	header = psMetadataAlloc ();
	psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", header);
	psFree (header);
    }
    psMetadataAddS32 (header, PS_LIST_TAIL, "ZPT_NREF", PS_META_REPLACE, "Total N refcat sources for zero point", zpt_nref);
    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MIN", PS_META_REPLACE, "min refcat mag for zero point", refminMag);
    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_MAX", PS_META_REPLACE, "max refcat mag for zero point", refmaxMag);

    psMetadataItem *item = psMetadataLookup(fpa->concepts, "FPA.ZP");
    item->data.F32 = fpaZP / (float)numZP;

    psFree (view);
    return true;
}

/**
 * accumulate the dMag values from this readout
 */
psVector *psastroZeroPointReadoutAccum(psVector *dMag, pmReadout *readout, float exptime, float refminMag, float refmaxMag) {

    bool status;

    // select the raw objects for this readout
    psArray *rawstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
    if (rawstars == NULL) return dMag;

    // select the raw objects for this readout
    psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
    if (refstars == NULL) return dMag;

    psArray *matches = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.MATCH");
    if (matches == NULL) return dMag;

    if (!dMag) {
        dMag  = psVectorAllocEmpty (100, PS_TYPE_F32);
    }

    for (int i = 0; i < matches->n; i++) {

      pmAstromMatch *match = matches->data[i];

      pmAstromObj *raw = rawstars->data[match->raw];

      pmAstromObj *ref = refstars->data[match->ref];

      // XXX I should be applying the airmass term here or in psastroLoadRefstars
      // (ie, ref->Mag would be a prediction for a given star.
      // MEH -- only append if in allowed ref mag range -- may want merr but not available..
      if ( ref->Mag >= refminMag && ref->Mag <= refmaxMag ){
        float value = ref->Mag - raw->Mag - 2.5*log10(exptime);
        psVectorAppend (dMag, value);
      }
    }
    return dMag;
}

bool psastroZeroPointAnalysis (psMetadata *header, psVector *dMag, float zeropt, psMetadata *recipe) {

    // XXX make this depend on the mode?
    if (!dMag || dMag->n < 3) {
      fprintf (stderr, "zero point NaN +/- NaN\n");
      return false;
    }

    bool status;
    bool useMean = psMetadataLookupBool (&status, recipe, "ZERO.POINT.USE.MEAN");

    // the zero point analysis depends on the type of desired statistic.  For comparisons
    // against a high-quality reference catalog with a good match to the actual filter used, it
    // is best to use a standard clipped mean or global mean statistic.  If the reference
    // catalog has some unmodeled extra parameter, as is the case for the synthetic grizy
    // database vs PS1, then it is best to use some consistent feature in the color
    // distribution.

    psStats *stats = NULL;
    float zeroPtObs, zeroPtErr;
    if (useMean) {
        // stats structure and mask for use in measuring the clipping statistic
        // this analysis has too few data points to use the robust median method
        stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
        if (!psVectorStats (stats, dMag, NULL, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "failure to measure zero point by mean");
            return false;
        }
	zeroPtObs = stats->robustMedian;
	zeroPtErr = stats->robustStdev;
    } else {
        stats = psastroStatsPercentile (dMag, recipe);
        if (!stats) {
            psError(PS_ERR_UNKNOWN, false, "failure to measure zero point by edge");
            return false;
        }
	zeroPtObs = stats->clippedMean;
	zeroPtErr = stats->clippedStdev;
    }
    fprintf (stderr, "zero point %f +/- %f using %ld stars; transparency %f\n", zeroPtObs, zeroPtErr, dMag->n, zeropt - zeroPtObs);

    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_OBS", PS_META_REPLACE, "measured zero point", zeroPtObs);
    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_REF", PS_META_REPLACE, "reference zero point", zeropt);
    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_ERR", PS_META_REPLACE, "error on zero point",  zeroPtErr);
    psMetadataAddF32 (header, PS_LIST_TAIL, "ZPT_OFF", PS_META_REPLACE, "zero point offset",    zeropt - zeroPtObs);

    psFree (stats);
    return true;
}

#define MAG_RESOLUTION 0.0002

// set the bin closest to the corresponding value.  if USE_END is +/- 1,
// out-of-range saturates on lower/upper bin REGARDLESS of actual value
#define PS_BIN_FOR_VALUE(RESULT, VECTOR, VALUE, USE_END) { \
        psVectorBinaryDisectResult result; \
        psScalar tmpScalar; \
        tmpScalar.type.type = PS_TYPE_F32; \
        tmpScalar.data.F32 = (VALUE); \
        RESULT = psVectorBinaryDisect (&result, VECTOR, &tmpScalar); \
        switch (result) { \
          case PS_BINARY_DISECT_PASS: \
            break; \
          case PS_BINARY_DISECT_OUTSIDE_RANGE: \
            psTrace("psastro", 6, "selected bin outside range"); \
            if (USE_END == -1) { RESULT = 0; } \
            if (USE_END == +1) { RESULT = VECTOR->n - 1; } \
            break; \
          case PS_BINARY_DISECT_INVALID_INPUT: \
          case PS_BINARY_DISECT_INVALID_TYPE: \
            psAbort ("programming error"); \
            break; \
        } }

# define PS_BIN_INTERPOLATE(RESULT, VECTOR, BOUNDS, BIN, VALUE) { \
        float dX, dY, Xo, Yo, Xt; \
        if (BIN == BOUNDS->n - 1) { \
            dX = 0.5*(BOUNDS->data.F32[BIN+1] - BOUNDS->data.F32[BIN-1]); \
            dY = VECTOR->data.F32[BIN] - VECTOR->data.F32[BIN-1]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } else { \
            dX = 0.5*(BOUNDS->data.F32[BIN+2] - BOUNDS->data.F32[BIN]); \
            dY = VECTOR->data.F32[BIN+1] - VECTOR->data.F32[BIN]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } \
        if (dY != 0) { \
            Xt = (VALUE - Yo)*dX/dY + Xo; \
        } else { \
            Xt = Xo; \
        } \
        Xt = PS_MIN (BOUNDS->data.F32[BIN+1], PS_MAX(BOUNDS->data.F32[BIN], Xt)); \
        psTrace("psastro", 6, "(Xo, Yo, dX, dY, Xt, Yt) is (%.2f %.2f %.2f %.2f %.2f %.2f)\n", \
                Xo, Yo, dX, dY, Xt, VALUE); \
        RESULT = Xt; }

// measure the edge of the sample at flimit
// return results in stats->sampleMean, sampleStdev
// XXX this is a misuse of psStats -- make our own structure?
psStats *psastroStatsPercentile (psVector *myVector, psMetadata *recipe) {

    // search for the 'blue' edge of the dMag distribution:
    // the distribution is not a normal population, but instead has a broad range with fairly hard edges.
    // construct a histogram and look for the

    bool status;
    float edgeFraction = psMetadataLookupF32 (&status, recipe, "ZERO.POINT.EDGE.FRACTION");
    int edgeSample = psMetadataLookupS32 (&status, recipe, "ZERO.POINT.EDGE.SAMPLE");
    float edgeSampleFraction = psMetadataLookupF32 (&status, recipe, "ZERO.POINT.EDGE.SAMPLE.FRACTION");

    // stats is first used to find the data range
    psStats *stats = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX); // Statistics for min and max
    psHistogram *histogram = NULL;      // Histogram of the data
    psHistogram *cumulative = NULL;     // Cumulative histogram of the data
    float min = NAN, max = NAN;         // Mimimum and maximum values

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);

    // Get the minimum and maximum values
    if (!psVectorStats(stats, myVector, NULL, NULL, 0)) goto escape;
    min = stats->min;
    max = stats->max;
    if (isnan(min) || isnan(max)) goto escape;
    psTrace("psastro", 5, "Data min/max is (%.2f, %.2f)\n", min, max);

    // If all data points have the same value, then we set the appropriate members of stats and return.
    if (fabs(max - min) <= FLT_EPSILON) {
        stats->clippedMean = min;
        stats->clippedStdev = NAN;
        stats->results |= PS_STAT_CLIPPED_MEAN;
        stats->results |= PS_STAT_CLIPPED_STDEV;
        return stats;
    }

    // Define the histogram bin size.
    float binSize = MAG_RESOLUTION;
    long numBins = (max - min) / binSize; // Number of bins
    psTrace("psastro", 5, "Numbins is %ld\n", numBins);
    psTrace("psastro", 5, "Creating a robust histogram from data range (%.2f - %.2f)\n", min, max);

    // allocate the histogram containers
    histogram = psHistogramAlloc(min, max, numBins);
    cumulative = psHistogramAlloc(min, max, numBins);

    // find the mean value:
    stats->clippedMean = psastroStatsPercentileValue (histogram, cumulative, myVector, edgeFraction);

    int nSubset = myVector->n * edgeSampleFraction;
    psVector *subset = psVectorAlloc (nSubset, PS_TYPE_F32);

    float Sum = 0.0;
    float S2 = 0.0;
    for (int i = 0; i < edgeSample; i++) {

        // generate the subset vector
        for (long i = 0; i < nSubset; i++) {
            double frnd = psRandomUniform(rng);
            int entry = PS_MIN(myVector->n - 1, PS_MAX(0, myVector->n * frnd));

            subset->data.F32[i] = myVector->data.F32[entry];
        }

        float value = psastroStatsPercentileValue (histogram, cumulative, subset, edgeFraction);

        Sum += value;
        S2 += value*value;
    }
    psTrace("psastro", 6, "subset stats: Sum: %f, S2: %f, Npts: %d\n", Sum, S2, edgeSample);

    stats->clippedStdev = PS_MAX (sqrt(S2 / edgeSample - PS_SQR(Sum/edgeSample)), MAG_RESOLUTION);
    psTrace("psastro", 5, "percentile stats %f +/- %f\n", stats->clippedMean, stats->clippedStdev);

    stats->results |= PS_STAT_CLIPPED_MEAN;
    stats->results |= PS_STAT_CLIPPED_STDEV;
    stats->options = PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV;

    // Clean up
    psFree(histogram);
    psFree(cumulative);
    psFree(subset);
    psFree(rng);
    return stats;

escape:
    stats->clippedMean = NAN;
    stats->clippedStdev = NAN;
    stats->results |= PS_STAT_CLIPPED_MEAN;
    stats->results |= PS_STAT_CLIPPED_STDEV;
    stats->options = PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV;

    psFree(histogram);
    psFree(cumulative);

    return stats;
}


// measure the edge of the sample at flimit
// return results in stats->sampleMean, sampleStdev
// XXX this is a misuse of psStats -- make our own structure?
float psastroStatsPercentileValue (psHistogram *histogram, psHistogram *cumulative, psVector *myVector, float flimit) {

    // need to initialize the histogram on each pass
    psVectorInit (histogram->nums, 0);
    if (!psVectorHistogram(histogram, myVector, NULL, NULL, 0)) {
        // if psVectorHistogram returns false, we have a programming error
        psAbort ("Unable to generate histogram for psastroZeroPointAnalysis");
    }
    if (psTraceGetLevel("psastro") >= 8) {
        PS_VECTOR_PRINT_F32(histogram->bounds);
        PS_VECTOR_PRINT_F32(histogram->nums);
    }

    // Convert the specific histogram to a cumulative histogram
    // The cumulative histogram data points correspond to the UPPER bound value (N < Bin[i+1])
    cumulative->nums->data.F32[0] = histogram->nums->data.F32[0];
    for (long i = 1; i < histogram->nums->n; i++) {
        cumulative->nums->data.F32[i] = cumulative->nums->data.F32[i-1] + histogram->nums->data.F32[i];
        cumulative->bounds->data.F32[i-1] = histogram->bounds->data.F32[i];
    }
    if (psTraceGetLevel("psastro") >= 8) {
        PS_VECTOR_PRINT_F32(cumulative->bounds);
        PS_VECTOR_PRINT_F32(cumulative->nums);
    }

    // Find the bin which contains the first data point above the limit
    long numBins = cumulative->nums->n;
    long totalDataPoints = cumulative->nums->data.F32[numBins - 1];
    psTrace("psastro", 6, "Total data points is %ld\n", totalDataPoints);

    // find bin which is the lower bound of the limit value (value[bin] < f < value[bin+1]
    long bin;
    PS_BIN_FOR_VALUE(bin, cumulative->nums, flimit * totalDataPoints, 0);
    psTrace("psastro", 6, "The bin is %ld (%.4f to %.4f)\n", bin, cumulative->bounds->data.F32[bin], cumulative->bounds->data.F32[bin+1]);

    // Linear interpolation to the limit value in bin units
    float value;
    PS_BIN_INTERPOLATE (value, cumulative->nums, cumulative->bounds, bin, totalDataPoints * flimit);
    psTrace("psastro", 6, "limit value is %f\n", value);

    return value;
}

# define ESCAPE(MSG) { \
  psLogMsg ("psastro", PS_LOG_INFO, MSG); \
  return false; }

bool psastroZeroPointFromRecipe (float *zeropt, float *exptime, float *ghostMaxMag, double *glintMaxMag, pmFPA *fpa, psMetadata *recipe) {

    bool status;

    // select the filter; default to fixed photcode and mag limit otherwise
    char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");
    if (!status) ESCAPE ("missing FPA.FILTER in concepts");

    *exptime = psMetadataLookupF32 (&status, fpa->concepts, "FPA.EXPOSURE");
    if (!status) ESCAPE ("missing FPA.EXPOSURE in concepts");

    // we need to select the PHOTCODE.DATA folder that matches our filter
    psMetadataItem *item = psMetadataLookup (recipe, "PHOTCODE.DATA");
    if (!item) ESCAPE ("PHOTCODE.DATA folders missing");
    if (item->type != PS_DATA_METADATA_MULTI) ESCAPE ("PHOTCODE.DATA not a multi");

    // PHOTCODE.DATA is a multi of metadata items
    psListIterator *iter = psListIteratorAlloc(item->data.list, PS_LIST_HEAD, false);

    psMetadataItem *refItem = NULL;
    while ((refItem = psListGetAndIncrement (iter))) {
        if (refItem->type != PS_DATA_METADATA) ESCAPE ("PHOTCODE.DATA entry is not a metadata folder");

        char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
        if (!status) {
            // psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }

        // does this entry match the current filter?
        if (strcmp (refFilter, filter)) continue;

        psLogMsg ("psastro", PS_LOG_DETAIL, "PHOTCODE.DATA found for filter %s", filter);

        *zeropt = psMetadataLookupF32 (&status, refItem->data.md, "ZEROPT");
        if (!status) {
            psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing ZEROPT");
            continue;
        }
        if (ghostMaxMag) {
            *ghostMaxMag = psMetadataLookupF32 (&status, refItem->data.md, "GHOST_MAX_MAG");
            if (!status) {
                psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing GHOST_MAX_MAG");
                continue;
            }
	    //MEH null is a pain.. so only log if set
            psLogMsg ("psastro", PS_LOG_INFO, "found GHOST_MAX_MAG %f",*ghostMaxMag);
        }
        if (glintMaxMag) {
            float MaxMag = psMetadataLookupF32 (&status, refItem->data.md, "GLINT_MAX_MAG");
            if (status) {
                *glintMaxMag = MaxMag ;
	        //MEH null is a pain.. so only log if set
                psLogMsg ("psastro", PS_LOG_INFO, "found GLINT_MAX_MAG %f",*glintMaxMag);
            }
        }
	//MEH what zpt is set to 
        psLogMsg ("psastro", PS_LOG_INFO, "found ZEROPT  %f",*zeropt);
        psFree (iter);
        return true;
    }
    psFree (iter);
    return false;
}



bool psastroZeroPointRefMagLimitFromRecipe (float *refminMag, float *refmaxMag, pmFPA *fpa, psMetadata *recipe) {

    bool status;
    float tmpmin,tmpmax;

    // MEH -- duplicate psastroZeroPointFromRecipe for ZeroPoint adding RefMag limits with photcode option for extra restrictions
    // must have non-photcode dependent setting -- use as default rather than a hardcoded value 
    *refminMag = psMetadataLookupF32 (&status, recipe, "REFSTAR.ZP.MIN.MAG");
    //if (!status) *refminMag = -5.0;
    if (!status) {
        psLogMsg ("psastro", PS_LOG_INFO, "RefMagLimit is missing REFSTAR.ZP.MIN.MAG");
        return false;
    }
    *refmaxMag = psMetadataLookupF32 (&status, recipe, "REFSTAR.ZP.MAX.MAG");
    //if (!status) *refmaxMag = 30.0;
    if (!status) {
        psLogMsg ("psastro", PS_LOG_INFO, "RefMagLimit is missing REFSTAR.ZP.MAX.MAG");
        return false;
    }

    // select the filter; default to any photcode and mag limit otherwise
    char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");
    if (!status) ESCAPE ("RefMagLimit missing FPA.FILTER in concepts");

    // need to select the PHOTCODE.DATA folder that matches our filter
    psMetadataItem *item = psMetadataLookup (recipe, "PHOTCODE.DATA");
    if (!item) ESCAPE ("RefMagLimit PHOTCODE.DATA folders missing");
    if (item->type != PS_DATA_METADATA_MULTI) ESCAPE ("RefMagLimit PHOTCODE.DATA not a multi");

    // PHOTCODE.DATA is a multi of metadata items
    psListIterator *iter = psListIteratorAlloc(item->data.list, PS_LIST_HEAD, false);

    psMetadataItem *refItem = NULL;
    while ((refItem = psListGetAndIncrement (iter))) {
        if (refItem->type != PS_DATA_METADATA) ESCAPE ("RefMagLimit PHOTCODE.DATA entry is not a metadata folder");

        char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
        if (!status) {
	    // always seems to happen once, commented out in ZeroPoint above as well
            //psLogMsg ("psastro", PS_LOG_INFO, "RefMagLimit PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }

        // does this entry match the current filter?
        if (strcmp (refFilter, filter)) continue;

        psLogMsg ("psastro", PS_LOG_DETAIL, "RefMagLimit PHOTCODE.DATA found for filter %s", filter);

        // -- may set one or other or both -- no reason to log if found or often not
        tmpmin = psMetadataLookupF32 (&status, refItem->data.md, "REFSTAR.ZP.MIN.MAG");
	if (status) {
	    *refminMag = tmpmin;
	} 
        tmpmax = psMetadataLookupF32 (&status, refItem->data.md, "REFSTAR.ZP.MAX.MAG");
	if (status) {
	    *refmaxMag = tmpmax;
	}
        // once finds filter, should free and return true but multiple groups cause issue so just go through all groups now
        //psFree (iter);
        //return true;
    }

    // log out set values
    psLogMsg ("psastro", PS_LOG_INFO, "using REFSTAR.ZP.MIN.MAG %f",*refminMag);
    psLogMsg ("psastro", PS_LOG_INFO, "using REFSTAR.ZP.MAX.MAG %f",*refmaxMag);

    psFree (iter);
    return true;
}

