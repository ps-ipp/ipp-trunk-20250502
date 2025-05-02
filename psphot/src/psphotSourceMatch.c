# include "psphotInternal.h" 

// Structure for storing the contents of the PSPHOT.STACK.MATCH.FILTERS list from recipe
#define MATCH_INFO_FILTER_STR_LEN 16
typedef struct {
    int     inputNum;
    int     order;
    bool    matchAll;
    long    nSources;
    float   yRatioMax;
    char    filterID[MATCH_INFO_FILTER_STR_LEN];
} psphotStackMatchInfo;

static psphotStackMatchInfo * psphotStackGetMatchInfo (pmConfig *config, const pmFPAview *view, const char *filerule);

// functions for sorting the match info array
static int compareMatchInfoByInputNum(const void *a, const void *b);
static int compareMatchInfoByOrder(const void *a, const void *b);

bool psphotMatchSourcesAddMissing (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects, psphotStackMatchInfo *matchInfo);
bool psphotMatchSourcesSetIDs (psArray *objects);

static psArray     *psphotMatchFootprintCacheAlloc (int nImages);
static pmFootprint *psphotMatchLookupFootprint (psArray *cache, int footprintID, int imageID);
static pmFootprint *psphotMatchCopyFootprint (psArray *cache, pmFootprint *footprint, int imageID, psImage *image, psArray *footprints);
 
psArray *psphotMatchSources (pmConfig *config, const pmFPAview *view, const char *filerule) 
{
    psArray *objects = psArrayAllocEmpty(100);

    int num = psphotFileruleCount(config, filerule);

    psphotStackMatchInfo *matchInfo = psphotStackGetMatchInfo(config, view, filerule);
    
    // loop over the available readouts matching sources found to objects. The inputs are processed
    // in the order specified by the recipe
    for (int j = 0; j < num; j++) {
        int i = matchInfo[j].inputNum;
        if (!psphotMatchSourcesReadout (objects, config, view, filerule, i)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to merge sources for %s entry %d", filerule, i);
	    psFree (objects);
            return NULL;
        }
    }

    // Now re-order the matchInfo array by input number so we can find each inputs entry easily
    qsort(matchInfo, num, sizeof(psphotStackMatchInfo), compareMatchInfoByInputNum);

    // create sources for images where an object has been detected in the other images
    psphotMatchSourcesAddMissing (config, view, filerule, objects, matchInfo);

    // choose a consistent position; set common sequence values
    psphotMatchSourcesSetIDs (objects);

    psFree(matchInfo);

    return objects;
}

bool psphotMatchSourcesReadout (psArray *objects, pmConfig *config, const pmFPAview *view, const char *filerule, int index) {
 
    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int RADIUS = psMetadataLookupF32 (&status, recipe, "PSPHOT.STACK.MATCH.RADIUS");
    psAssert (status, "programming error: must define PSPHOT.STACK.MATCH.RADIUS");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");
    psAssert (detections->allSources, "all sources not defined?");

    psphotMatchSourcesToObjects(objects, detections->allSources, RADIUS);

    return true;
}

# define NEXT1 { i++; continue; } 
# define NEXT2 { j++; continue; } 
bool psphotMatchSourcesToObjects (psArray *objects, psArray *sources, float RADIUS) {
 
    float dx, dy; 
 
    float RADIUS2 = RADIUS*RADIUS;

    // sort the source list by X 
    sources = psArraySort (sources, pmSourceSortByX); 
    objects = psArraySort (objects, pmPhotObjSortByX); 

    psVector *foundSrc = psVectorAlloc(sources->n, PS_TYPE_U8);
    psVectorInit (foundSrc, 0);

    psVector *foundObj = psVectorAlloc(objects->n, PS_TYPE_U8);
    psVectorInit (foundObj, 0);

    // match sources to existing objects

    psLogMsg ("psphot", PS_LOG_DETAIL, "attempt to match sources (%ld vs %ld)", sources->n, objects->n);

    int i, j; 
    for (i = j = 0; (i < sources->n) && (j < objects->n); ) { 
 
        pmSource  *src = sources->data[i]; 
        pmPhotObj *obj = objects->data[j]; 
 
        if (!src) NEXT1;
        if (!src->peak) NEXT1; 
        if (!isfinite(src->peak->xf)) NEXT1; 
        if (!isfinite(src->peak->yf)) NEXT1; 

        if (!obj) NEXT2; 
        if (!isfinite(obj->x)) NEXT2; 
        if (!isfinite(obj->y)) NEXT2; 
 
        dx = src->peak->xf - obj->x; 
        if (dx < -1.02*RADIUS) NEXT1; 
        if (dx > +1.02*RADIUS) NEXT2; 
 
	/* this block will match a given detection to the closest object within range of that detection.
	   XXX note that this matches ALL detections within range of the single object to that same object 
	   this is bad, but I cannot just go in linear order (ie, mark off each object as they are
	   used).  I should make a list of all Nobj * Ndet pairs in range and choose the matches
	   based on their separations.  UGH
	*/
    
        // we are within match range, look for matches: 
	int Jmin = -1;
	float Rmin = RADIUS2;
        for (int J = j; (dx > -1.02*RADIUS) && (J < objects->n); J++) { 
 
	    // skip objects that are already assigned:
	    if (foundObj->data.U8[J]) continue;
	    obj = objects->data[J]; 
	    
	    dx = src->peak->xf - obj->x; 
            dy = src->peak->yf - obj->y; 
 
            float dr = dx*dx + dy*dy; 
            if (dr > RADIUS2) continue; 
	    if (dr < Rmin) {
		Rmin = dr;
		Jmin  = J;
	    }
	}

	// no match, try next source
	if (Jmin == -1) {
	    i++;
	    continue;
	}
	obj = objects->data[Jmin]; 
	foundObj->data.U8[Jmin] = 1;

	// add to object
	pmPhotObjAddSource (obj, src);
	foundSrc->data.U8[i] = 1;
        i++; 
    } 

    // create new objects for unmatched sources
    for (i = 0; i < sources->n; i++) {

        if (foundSrc->data.U8[i]) continue;

        pmSource *src = sources->data[i]; 

        pmPhotObj *obj = pmPhotObjAlloc();
        pmPhotObjAddSource(obj, src);
        psArrayAdd (objects, 100, obj);
        psFree (obj);
    }
    psLogMsg ("psphot", PS_LOG_DETAIL, "matched sources (%ld vs %ld)", sources->n, objects->n);

    psFree (foundSrc);
    psFree (foundObj);
    return true;
} 

bool psphotMatchSourcesAddMissing (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects, psphotStackMatchInfo *matchInfo) {

    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int minDetectionsForForced = psMetadataLookupS32 (&status, recipe, "PSPHOT.STACK.MIN.DETECT.FOR.FORCED");
    if (!status) {
        minDetectionsForForced = 2;
    }

    // determine properties (sky, moments) of initial sources
    float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    psAssert (status, "missing SKY_OUTER_RADIUS in recipe?");

    int nImages = psphotFileruleCount(config, filerule);

    // generate look-up arrays for detections and readouts
    psArray *detArrays = psArrayAlloc(nImages);
    psArray *readouts = psArrayAlloc(nImages);

    for (int i = 0; i < nImages; i++) {

	// find the currently selected readout
	pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
	psAssert (readout, "missing readout?");

	pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
	psAssert (detections, "missing detections?");
        psAssert (detections->allSources, "all sources not defined?");

	detArrays->data[i] = psMemIncrRefCounter(detections);
	readouts->data[i] = psMemIncrRefCounter(readout);
    }

    // We are having a problem with large number of false y band detections causing large numbers of
    // matched detections to be created.
    // The matchInfo recipe each entry contains a value for Y.RATIO.MAX. For each filter with a non-zero value
    // we compare the ratio of the number of y detections to the number of detections in that filter to Y.RATIO.MAX.
    // If the ratio in all filters with a non-zero is above its limit we consider the y band stack to
    // be suspect and so disable the y band matchAll recipe (if set)

    int iy = -1;
    // note now we are looping over matchInfo
    for (int i = 0; i < nImages; i++) {
        // Remember which input is y
        if (!strcmp(matchInfo[i].filterID, "y")) {
            iy = i;
        }
	pmDetections *detections = detArrays->data[matchInfo[i].inputNum];
        matchInfo[i].nSources = detections->allSources->n;
    }

    if (iy >= 0) {
        int nCut = 0;
        int nTry = 0;
        for (int i = 0; i < nImages; i++) {
            if (i == iy) continue;
            if (matchInfo[i].yRatioMax != 0.0 && matchInfo[i].nSources > 0) {
                ++nTry;
                float ratio = (float) matchInfo[iy].nSources / (float) matchInfo[i].nSources;
                psLogMsg ("psphot", PS_LOG_DETAIL, "nSrc_y / nSrc_%s: %6.3f max: %6.3f", matchInfo[i].filterID, ratio,
                    matchInfo[i].yRatioMax);
                if (ratio > matchInfo[i].yRatioMax) {
                    // This one is above it's threshold
                    ++nCut;
                }
            }
        }
        if (nTry > 0 && nCut == nTry) {
            // All filters with a cut were above the threshold. Disable matchAll for y band
            psLogMsg ("psphot", PS_LOG_INFO, "Clearing y band matchAll because y ratio was too high in %d filters", nTry);
            matchInfo[iy].matchAll = false;
        }
    }

    // vector to track if source for an image is found
    psVector *found = psVectorAlloc(nImages, PS_TYPE_U8);
    psArray *footprintCache = psphotMatchFootprintCacheAlloc(nImages);

    for (int i = 0; i < objects->n; i++) { 
        pmPhotObj *obj = objects->data[i]; 

	// we will find the input source with the max number of spans and reproduce that footprint
	int nSpansMax = 0;
	int iSpansMax = -1;

        bool matchAll = false;

	// mark the images for which sources have been found
	psVectorInit (found, 0);
	for (int j = 0; j < obj->sources->n; j++) {

	    pmSource *src = obj->sources->data[j]; 
	    int index = src->imageID;
	    psAssert (index >= 0, "invalid index");
	    psAssert (index < found->n, "invalid index");

	    if (src->peak && src->peak->footprint && src->peak->footprint->nspans > nSpansMax) {
		nSpansMax = src->peak->footprint->nspans;
		iSpansMax = j;
	    }
            // If this detection was on an input that has the matchAll flag set we create matched sources
            // irespective of the number of inputs in which the object was found.
            if (matchInfo[index].matchAll) {
                matchAll = true;
            }

	    found->data.U8[index] = 1;
	}

        // skip adding matched sources for this object if the number of detections for less than
        // the supplied mininum unless one of the detections was in a band for which the matchAll flag is set
        if (!matchAll && obj->sources->n < minDetectionsForForced) {
            continue;
        }

	// we make a copy of the largest footprint; this will be used for all new sources associated with this object
	pmFootprint *largestFootprint = NULL;
	if (iSpansMax != -1) { // copy the footprint info
	    pmSource *src = obj->sources->data[iSpansMax]; 
	    psAssert(src->peak, "source does not exist?");
	    psAssert(src->peak->footprint, "footprint does not exist");
	    psAssert(src->peak->footprint->nspans == nSpansMax, "wrong footprint?");
	    
            largestFootprint = src->peak->footprint;
	}

	// generate new sources for the image that are missing
	for (int index = 0; index < found->n; index++) {
	    if (found->data.U8[index]) continue;

	    pmDetections *detections = detArrays->data[index];
	    pmReadout *readout = readouts->data[index];
	    int row0 = readout->image->row0;
	    int col0 = readout->image->col0;

	    // The peak type is not used in psphot. PM_PEAK_LONE may be wrong, but irrelevant
	    float peakFlux = readout->image->data.F32[(int)(obj->y-row0-0.5)][(int)(obj->x-col0-0.5)];
	    pmPeak *peak = pmPeakAlloc(obj->x, obj->y, peakFlux, PM_PEAK_LONE);
	    peak->xf = obj->x;
	    peak->yf = obj->y;
	    peak->dx = NAN;
	    peak->dy = NAN;
	    
	    // assign to a footprint on this readout->image
	    if (largestFootprint) {
                // we save the copies that we make of the of the footprints in a hash so that we can reuse them
                // for all sources that share the fooprint. Without this we had serious memory explosion in
                // dense fields with lots of footprints (spans are small but when you have enough of them ...)
                peak->footprint = psphotMatchLookupFootprint(footprintCache, largestFootprint->id, index);
                if (!peak->footprint) {
                    // the peak does not claim ownership of the footprint (it does not free it). 
                    // psphotMatchCopyFootprint saves a copy of this 
                    // footprint on detections->footprints so we can free it later
                    peak->footprint = psphotMatchCopyFootprint(footprintCache, largestFootprint,
                                            index, readout->image, detections->footprints);
                }
	    }
	    
	    // create a new source
	    pmSource *source = pmSourceAlloc();
	    source->imageID = index;
	    source->mode2 |= PM_SOURCE_MODE2_MATCHED; // source is generated based on another image
	    source->type = PM_SOURCE_TYPE_STAR; // until we know more, assume a PSF fit

	    // add the peak
	    source->peak = peak;

	    // allocate space for moments
	    source->moments = pmMomentsAlloc();

	    // allocate image, weight, mask arrays for each peak (square of radius OUTER)
	    pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);

#if (0)
            fprintf(stderr, "Add mising source for obj: %5d %5d image: %d flux: %f size: %4d %4d\n",
                                                      i, obj->id, index, peakFlux, source->pixels->numRows, source->pixels->numCols);
#endif

	    peak->assigned = true;
	    pmPhotObjAddSource(obj, source);
	    psArrayAdd (detections->allSources, 100, source);
	    psFree (source);
	}
    }
    psFree(footprintCache);

    // how many sources do we have now?
    int nSources = 0;
    for (int i = 0; i < objects->n; i++) { 
        pmPhotObj *obj = objects->data[i]; 
	nSources += obj->sources->n;
        if (minDetectionsForForced <= 1) {
            psAssert (obj->sources->n == nImages, "failed to match sources?");
        }
    }
    psLogMsg ("psphot", PS_LOG_DETAIL, "total of %d sources for %d images", nSources, nImages);


    psFree (found);
    psFree (detArrays);
    psFree (readouts);
    return true;
}

bool psphotMatchSourcesSetIDs (psArray *objects) {

    for (int i = 0; i < objects->n; i++) { 
        pmPhotObj *obj = objects->data[i]; 

	// set the source->seq values 
	for (int j = 0; j < obj->sources->n; j++) {
	    pmSource *src = obj->sources->data[j]; 
	    src->seq = i;
	}
    }
    return true;
}

// Cache of footprints created for unmatched sources.
static psArray * psphotMatchFootprintCacheAlloc (int nImages) {
    psArray *cache = psArrayAlloc(nImages);
    for (int i = 0; i < nImages; i++) {
        psHash *hash = psHashAlloc(5000);
        cache->data[i] = hash;
    }
    return cache;
}

// Find copy of footprint with given ID made for a given image
static pmFootprint *psphotMatchLookupFootprint (psArray *cache, int footprintID, int imageID) {
    psHash *hash = (psHash *) cache->data[imageID];

    psAssert(hash != NULL, "missing hash for image %d", imageID);

    // footprintID is the id of the original footprint.
    char key[32];
    sprintf(key, "%d", footprintID); 

    // The footprints in our hashes are the copies of that footprint for the respective images
    pmFootprint *copy = psHashLookup(hash, key);

    return copy;
}

// Create a copy of a given footprint for a given image
static pmFootprint *psphotMatchCopyFootprint (psArray *cache, pmFootprint *footprint, int imageID, psImage *image, psArray *footprints) {

    psAssert((imageID >= 0 && imageID < cache->n), "invalid imageID %d", imageID);

    psHash *hash = (psHash *) cache->data[imageID];

    psAssert(hash != NULL, "missing hash for image %d", imageID);

    char key[32];
    sprintf(key, "%d", footprint->id); 

    pmFootprint *copy = pmFootprintCopyData (footprint, image);

    // save a copy in this image's hash
    psHashAdd(hash, key, copy);

    // save a copy of this footprint on the passed in footprints Array so we can free it later
    psArrayAdd(footprints, 100, copy);

    // drop our ref
    psFree(copy);

    return copy;
}

// qsort function to sort match info by order
static int compareMatchInfoByOrder(const void *a, const void *b) {
    psphotStackMatchInfo *infoA = (psphotStackMatchInfo *) a;
    psphotStackMatchInfo *infoB = (psphotStackMatchInfo *) b;

    int     result;
    if (infoA->order < infoB->order) {
        result = -1;
    } else if (infoA->order == infoB->order) {
        result = 0;
    } else {
        result = 1;
    }
    return result;
}

// qsort function to sort match info by input number
static int compareMatchInfoByInputNum(const void *a, const void *b) {
    psphotStackMatchInfo *infoA = (psphotStackMatchInfo *) a;
    psphotStackMatchInfo *infoB = (psphotStackMatchInfo *) b;

    int     result;
    if (infoA->inputNum < infoB->inputNum) {
        result = -1;
    } else if (infoA->inputNum == infoB->inputNum) {
        result = 0;
    } else {
        result = 1;
    }
    return result;
}

// psphotStackGetMatchInfo
// process the recipe PSPHOT.STACK.MATCH.FILTERS which controls the order that the inputs
// are processed for source matching and the rules for creating matched sources for sources that
// are only detected in a single band.
static psphotStackMatchInfo *psphotStackGetMatchInfo (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status = false;
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);

    psMetadata *filterInfo = psMetadataLookupPtr (&status, recipe, "PSPHOT.STACK.MATCH.FILTERS");
    if (!status || !filterInfo) {   
        psLogMsg ("psphot", PS_LOG_WARN, "PSPHOT.MATCH.STACK.FILTERS not found in the recipe. Will process inputs in order supplied.");
        filterInfo = NULL;
    }

    int num = psphotFileruleCount(config, filerule);
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // Find the filter for each of the inputs in fpa concepts
    psArray *inputFilters = psArrayAlloc(num);
    for (int i = 0 ; i < num; i++) {
        if (i != chisqNum) {
            pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i);
            psAssert (file, "missing file?");

            psString filterid = psMetadataLookupStr(&status, file->fpa->concepts, "FPA.FILTERID");
            psAssert (filterid, "missing FPA.FILTERID?");
            psArraySet(inputFilters, i, filterid);
        } else {
            // The chisq image inherits its filterid from the first input so we shouldn't use that.
            psString tmp  = psStringCopy("chisq");
            psArraySet(inputFilters, i, tmp);
            psFree(tmp);
        }
    }
    // Allocate the match info array
    psphotStackMatchInfo *matchInfo = psAlloc(num *sizeof(psphotStackMatchInfo));
    memset(matchInfo, 0, num*sizeof(psphotStackMatchInfo));
    int highest_order = -1;
    if (filterInfo) {
        // PSPHOT.STACK.MATCH.FILTERS is a metadata which contains a list of metadata objects each
        // entry pertaining to a filter.
        // The objects contained in each filter's metadata are strings.
      
        // Loop over the entries in the recipe and find the entry for each our our input readouts.
        // XXX: Will this work if more than one input with a given filter is supplied?
        // I think so.
        psMetadataIterator *iter = psMetadataIteratorAlloc(filterInfo, PS_LIST_HEAD, NULL);
        psMetadataItem *item = NULL;
        while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
            if (item->type != PS_DATA_METADATA) {
                psAbort ("Invalid type for PSPHOT.STACK.MATCH.FILTER: %s, not a metadata folder", item->name);
            }
            psString thisFilter = psMetadataLookupStr (&status, item->data.md, "FILTER.ID");
            psAssert(thisFilter, "missing FILTER.ID");

            psString orderStr = psMetadataLookupStr (&status, item->data.md, "ORDER");
            psAssert(orderStr, "missing ORDER");
            psS32 order = atoi(orderStr);

            psString matchAllStr = psMetadataLookupStr (&status, item->data.md, "MATCH.ALL");
            psAssert(matchAllStr, "missing MATCH.ALL");
            bool matchAll = (strcasecmp("true", matchAllStr) == 0);

            psString yRatioStr = psMetadataLookupStr (&status, item->data.md, "Y.RATIO.MAX");
            psF32 yRatioMax = atof(yRatioStr);

            // look for this filter in the inputs
            for (int i = 0; i < num; i++) {
                if (!strcmp((char *)inputFilters->data[i], thisFilter)) {
                    // We have an input for this one
                    matchInfo[i].inputNum = i;
                    strncpy(matchInfo[i].filterID, thisFilter, MATCH_INFO_FILTER_STR_LEN - 1 );
                    matchInfo[i].order = order;
                    matchInfo[i].matchAll = matchAll;
                    matchInfo[i].yRatioMax = yRatioMax;
                    matchInfo[i].nSources = 0;
                    psLogMsg ("psphot", PS_LOG_DETAIL, "input: %d %s match order: %d match all: %d\n",
                                                               i, thisFilter, order, matchAll);
                    if (order > highest_order) {
                        highest_order = order;
                    }
                    break;
                }
            }
        }
        psFree(iter);
    }

    // Make sure we have a matchInfo for all of the inputs. Fill in an entry for any that were not found.
    for (int i = 0; i < num; i++) {
        if (!*matchInfo[i].filterID) {
            matchInfo[i].inputNum = i;
            strncpy(matchInfo[i].filterID, inputFilters->data[i], MATCH_INFO_FILTER_STR_LEN - 1);
            matchInfo[i].order = ++highest_order;
            matchInfo[i].matchAll = false;
            psLogMsg ("psphot", PS_LOG_WARN, "Entry in PSPHOT.MATCH.STACK.FILTERS not found for input: %d filter: %s using order %d",
                i, (char *) inputFilters->data[i], highest_order);
        }
    }
    psFree(inputFilters);

    if (filterInfo) {
        // Sort the array by ORDER.
        qsort(matchInfo, num, sizeof(psphotStackMatchInfo), compareMatchInfoByOrder);
    } else {
        // no need to sort we just built the list in the order that the inputs were supplied
    }

    return matchInfo;
}

bool psphotFilterMatchedSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects) {

    bool status = false;

    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Filter Matched Sources ---");

    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);

    // select the appropriate recipe information
    bool keepBadMatches = psMetadataLookupBool (&status, recipe, "PSPHOT.STACK.KEEP.BAD.MATCHES");
    if (!status) {
        keepBadMatches = true;
    }

    if (keepBadMatches) {
        psLogMsg ("psphot", PS_LOG_INFO, "keeping bad matches");
        return true;
    }

    int numImages = psphotFileruleCount(config, filerule);
    psVector *dropped = psVectorAlloc(numImages, PS_TYPE_U32);
    psVectorInit(dropped, 0);

    int nDropped = 0;
    for (int i = 0; i < objects->n; i++) { 
        pmPhotObj *obj = objects->data[i]; 

        // traverse the array from the end so that sources don't move until after we've processed them
	for (int j = obj->sources->n - 1; j >= 0; j--) {

	    pmSource *source = obj->sources->data[j]; 
            // This applies only to matched sources
            if (!(source->mode2 & PM_SOURCE_MODE2_MATCHED)) continue;

            if (isfinite(source->apFlux)) continue;

            psTrace ("psphot", 7, "Dropping matched source from image %d at (%d, %d) no valid flux",
                source->imageID, source->peak->x, source->peak->y);

            psAssert(source->imageID >= 0 && source->imageID < numImages, "bad imageID %d", source->imageID);

            dropped->data.U32[source->imageID]++;
            
            nDropped++;

            psArrayRemoveIndex(obj->sources, j);
        }
    }

    psLogMsg ("psphot", PS_LOG_DETAIL, "Dropped %d matched sources with no valid flux", nDropped);
    psLogMsg ("psphot", PS_LOG_DETAIL, "       Input  Num Dropped");
    for (int i=0; i<numImages; i++) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "    %8d     %8d", i, dropped->data.U32[i]);
    }
    psFree(dropped);

    // find the "best" Mrf from the detected sources. 
    // Currently we use the smallest positive value
    for (int i=0; i< objects->n; i++) {
        pmPhotObj *obj = objects->data[i]; 

        float minMrf = 1000.;
        bool hasMatched = false;
	for (int j = 0; j < obj->sources->n; j++) {
	    pmSource *source = obj->sources->data[j]; 
            if (source->mode2 & PM_SOURCE_MODE2_MATCHED) {
                hasMatched = true;
                continue;
            }
            float Mrf = source->moments->Mrf;
            if (isfinite(Mrf) && Mrf < minMrf && Mrf > 0) {
                minMrf = Mrf;
            }
        }

        if (!hasMatched || minMrf > 120.) {
            continue;
        }

        // set Mrf for matched sources to the value found above
	for (int j = 0; j < obj->sources->n; j++) {
	    pmSource *source = obj->sources->data[j]; 
            if (source->mode2 & PM_SOURCE_MODE2_MATCHED) {
                source->moments->Mrf = minMrf;
            }
        }
    }

    return true;
}


psArray *psphotLinkSources (pmConfig *config, const pmFPAview *view, const char *filerule) 
{

    int num = psphotFileruleCount(config, filerule);

    psArray *sourcesArrays = psArrayAlloc(num);

    // loop over inputs find the maximum sequence number and save pointers to the arrays
    int seqMax = -1;
    for (int i = 0; i < num; i++) {

	// find the currently selected readout
	pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
	psAssert (readout, "missing readout?");

	pmDetections *detections = psMetadataLookupPtr (NULL, readout->analysis, "PSPHOT.DETECTIONS");
	psAssert (detections, "missing detections?");
        psAssert (detections->allSources, "all sources not defined?");

        // allSources value won't change but should be careful
        detections->allSources = psArraySort(detections->allSources, pmSourceSortBySeq);

        sourcesArrays->data[i] = psMemIncrRefCounter(detections->allSources);
        pmSource *lastSrc = detections->allSources->data[detections->allSources->n -1 ];
        int thisSeq = lastSrc->seq;
        if (thisSeq > seqMax) {
            seqMax = thisSeq;
        }
    }

    if (seqMax < 0) {
        psError (PSPHOT_ERR_UNKNOWN, true, "failed to find maximum sequence number\n");
        return NULL;
    }

    // allocate objects array
    psArray *objects = psArrayAlloc(seqMax + 1);
    // loop over inputs and create objects that reference all of the sources
    for (int i = 0; i < num; i++) {
        // sources for this input
        psArray *sources = sourcesArrays->data[i];
        for (int j = 0; j < sources->n ; j++) {
            pmSource *src = sources->data[j];
            //  XXX       This is no longer needed I think. I added it to work around something else. Check
            src->imageID = i;
            int seq = src->seq;
            pmPhotObj *obj = objects->data[seq];
            if (!obj) {
                // first source for this object
                obj = objects->data[seq] = pmPhotObjAlloc();
            }
            pmPhotObjAddSource(obj, src);
        }
    }
    psFree(sourcesArrays);

    return objects;
}
