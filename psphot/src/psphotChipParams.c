# include "psphotInternal.h"

bool psphotChipParamsReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char *filerule, pmReadout *readout, psArray *sources);
bool psphotChipParams_Threaded (psThreadJob *job);
bool psphotChipParamsSource (pmSource *source, psArray *backmaps);

typedef struct {
    int         index;
    short       chipNum;
    psString    name;
    float       C0x;
    float       C1x;
    float       C2x;
    float       C0y;
    float       C1y;
    float       C2y;
    int         xMin;
    int         xMax;
    int         yMin;
    int         yMax;
} psphotChipBackmap ;

psArray * psphotChipParamsParseBackmaps(pmReadout *readout);

bool psphotChipParams (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Chip Parameters ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    //MEH -- shouldn't do if input is stack... will need to be a config option for now
    // -- return true method like for psphotExtendedSourceFits to be consistent...
    if (!psMetadataLookupBool (&status, recipe, "CHIP_PARAMS")) {
        psLogMsg ("psphot", PS_LOG_INFO, "stack -- skipping backward chip mapping \n");
        return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->newSources ? detections->newSources : detections->allSources;
        psAssert (sources, "missing sources?");

        if (!psphotChipParamsReadout (config, recipe, view, filerule, readout, sources)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
             return false;
        }
    }

    return true;
}

bool psphotChipParamsReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char *filerule, pmReadout *readout, psArray *sources) {

    bool status = false;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping chipParams");
        return true;
    }

    psTimerStart ("psphot.chipparams");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }
    psArray *backmaps = psphotChipParamsParseBackmaps(readout);
    if (!backmaps) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to parse chip backmaps  for %s", filerule);
        return false;
    }

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_CHIP_PARAMS");

            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, backmaps); // sources

// set this to 0 to run without threading
# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                psFree(backmaps);
                return false;
            }
# else
	    if (!psphotChipParams_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		// psFree(AnalysisRegion);
                psFree(backmaps);
		return false;
	    }
	    psFree(job);
# endif
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
            psFree(backmaps);
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            }
            psFree(job);
        }
    }
    psFree (cellGroups);
    psFree(backmaps);

    psLogMsg ("psphot.chip.params", PS_LOG_WARN, "measure chip paramters : %f sec for %ld objects\n", psTimerMark ("psphot.chipparams"), sources->n);
    return true;
}

bool psphotChipParams_Threaded (psThreadJob *job) {

    psArray *sources                = job->args->data[0];
    psArray *backmaps               = job->args->data[1];

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (!source->peak) continue; // XXX how can we have a peak-less source?

        psphotChipParamsSource(source, backmaps);
    }

    return true;
}


bool psphotChipParamsSource (pmSource *source, psArray *backmaps) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);

    float warp_x = source->peak->xf;
    float warp_y = source->peak->yf;

    int chipForSource = -1;
    for (int i=0; i < backmaps->n; i++) {
        psphotChipBackmap *thisChip = backmaps->data[i];

        float chip_x = thisChip->C0x + thisChip->C1x * warp_x + thisChip->C2x * warp_y;
        float chip_y = thisChip->C0y + thisChip->C1y * warp_x + thisChip->C2y * warp_y;
        if (chip_x >= thisChip->xMin && chip_x <= thisChip->xMax &&
            chip_y >= thisChip->yMin && chip_y <= thisChip->yMax) {

            chipForSource = thisChip->chipNum;
            if (0) {
                printf("source %8d on chip: %s %2d at %6.1f, %6.1f\n", source->chipNum, thisChip->name, 
                    chipForSource, chip_x, chip_y);
            }
            source->chipNum = chipForSource;
            source->chipX = chip_x;
            source->chipY = chip_y;
            // Since it is on this chip we are done for this source
            break;
        }
    }
    if (chipForSource < 0) return false;

    return true;
}


psArray * psphotChipParamsParseBackmaps(pmReadout *readout) {

    // XXX: I'm assuming the structure of the FPA here. Find a more usual way
    // to find header
    psMetadata *header = readout->parent->parent->parent->hdu->header;
    if (!header) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find header for readout.");
        return NULL;
    }

    psArray *backmaps = psArrayAllocEmpty(6);

    // find the keywords that define the warp to chip transformations stored by
    // pswarpUpdateMetadata()

    for (int i = 0; ; i++) {
        bool mdok;
        char keyword[16];

        sprintf(keyword, "SRC_%04d", i);
        psString chipName = psMetadataLookupStr(&mdok, header, keyword);
        if (!chipName) {
            break;
        }
        psphotChipBackmap *thisChip = psAlloc(sizeof(psphotChipBackmap));
        psArrayAdd(backmaps, 1, thisChip);
        psFree(thisChip);

        thisChip->index = i;
        thisChip->name = chipName; // not incrementing the reference this gets freed before header

        // XXX: If chipName begins with XY assume gpc1 style class_id and use number as ID
        if (!strncmp(chipName, "XY", 2)) {
            thisChip->chipNum = atoi(chipName+2);
        } else {
            // otherwise just use the index
            thisChip->chipNum = thisChip->index;
        }

        sprintf(keyword, "SEC_%04d", i);
        psString chipRegionStr = psMetadataLookupStr(&mdok, header, keyword);
        if (!chipRegionStr) {
            psError(PS_ERR_UNKNOWN, true, "failed to find required keyword %s", keyword);
            psFree(backmaps);
            return NULL;
        }
        int nvals = sscanf(chipRegionStr, "[%d:%d,%d:%d]", &thisChip->xMin, &thisChip->xMax, &thisChip->yMin, &thisChip->yMax);
        if (nvals != 4) {
            psError(PS_ERR_UNKNOWN, true, "failed to parse %s: %s", keyword, chipRegionStr);
            psFree(backmaps);
            return NULL;
        }
        // convert from fits extent convention to zero offset array
        #define COORD_SLACK 2
        thisChip->xMin -= COORD_SLACK; thisChip->xMax += COORD_SLACK; 
        thisChip->yMin -= COORD_SLACK; thisChip->yMax += COORD_SLACK;

        sprintf(keyword, "MPX_%04d", i);
        psString mpx = psMetadataLookupStr(&mdok, header, keyword);
        if (!mpx) {
            psError(PS_ERR_UNKNOWN, true, "failed to find %s", keyword);
            psFree(backmaps);
            return NULL;
        }
        nvals = sscanf(mpx, "[%f,%f,%f]", &thisChip->C0x, &thisChip->C1x, &thisChip->C2x);
        if (nvals != 3) {
            psError(PS_ERR_UNKNOWN, true, "failed to parse %s: %s", keyword, mpx);
            psFree(backmaps);
            return NULL;
        }
        sprintf(keyword, "MPY_%04d", i);
        psString mpy = psMetadataLookupStr(&mdok, header, keyword);
        if (!mpy) {
            psError(PS_ERR_UNKNOWN, true, "failed to find %s", keyword);
            psFree(backmaps);
            return NULL;
        }
        nvals = sscanf(mpy, "[%f,%f,%f]", &thisChip->C0y, &thisChip->C1y, &thisChip->C2y);
        if (nvals != 3) {
            psError(PS_ERR_UNKNOWN, true, "failed to parse %s: %s", keyword, mpx);
            psFree(backmaps);
            return NULL;
        }
    }

    if (!backmaps->n) {
        psFree(backmaps);
        return NULL;
    }

    return backmaps;
}
