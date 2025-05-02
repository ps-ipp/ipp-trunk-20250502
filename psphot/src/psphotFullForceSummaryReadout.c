#include "psphotInternal.h"

typedef struct {
    psF32   Q;
    psF32   NSigma;
    psF32   clampSN;
#ifdef notdef
    // these are per object now
    int     numTrials;
    psF64   fRmajorMin;
    psF64   fRmajorMax;
    psF64   fRmajorDel;
    psF64   fRminorMin;
    psF64   fRminorMax;
    psF64   fRminorDel;
    psVector    *fRmajor;
    psVector    *fRminor;
#endif
    psArray *zeroPt;    // zero points for each input used with exptime to scale flux
    psArray *exptime;   // exposure times for each input
    psArray *cffTables; // one for each model type index is (model_type + 1) entry 0 is no extended model (star)
} galaxyShapeSummaryOptions;



static pmSource *psphotFullForceSummarizeObject(pmConfig *config, pmPhotObj *obj, psVector *fluxScaleFactor, galaxyShapeSummaryOptions *options);
static pmPhotObj *findObjectForSource(psArray **pObjects, pmSource *source);

static bool setOptions(galaxyShapeSummaryOptions *options, pmReadout *readout, psMetadata *recipe, bool saveVectors);
static bool checkOptions(galaxyShapeSummaryOptions *options, pmReadout *readout, psMetadata *recipe);


bool psphotFullForceSummaryReadout (pmConfig *config, const pmFPAview *view) {

    bool status;
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, "PSPHOT.INPUT.CMF");

    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSPHOT.FULLFORCE.OUTPUT");
    pmCell *outputCell = pmFPAviewThisCell(view, output->fpa);
    pmReadout *outputReadout = pmFPAviewThisReadout(view, output->fpa);
    if (!outputReadout) {
        outputReadout = pmReadoutAlloc(outputCell);
    }

    // Get the exposure parameters for the output from recipe and set them on the output
    psF32 outputZeroPoint = psMetadataLookupF32(&status, recipe, "PSPHOT.FULLFORCE.ZERO_PT");
    psF32 outputExptime = psMetadataLookupF32(&status, recipe, "PSPHOT.FULLFORCE.EXPTIME");

    psMetadataAddF32(output->fpa->concepts, PS_LIST_TAIL, "FPA.ZP", PS_META_REPLACE, "Magnitude zero point",
        outputZeroPoint);
    psMetadataAddF32(outputCell->concepts, PS_LIST_TAIL, "CELL.EXPOSURE", PS_META_REPLACE, "Exposure time (sec)",
        outputExptime);

    // Create objects from the various input's sources
    // loop over the available readouts

    psVector *fluxScaleFactor = psVectorAlloc(num, PS_TYPE_F32);
    galaxyShapeSummaryOptions options;
    psArray *objects = NULL;
    for (int index = 0; index < num; index++) {
        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PSPHOT.INPUT.CMF", index); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        if (index == 0) {
            // Get the galaxy shape recipe values, from the analysis if present
            // or from the recipe if not
            if (!setOptions(&options, readout, recipe, true)) {
                psError (PS_ERR_UNKNOWN, false, "problem determining galaxy shape options.");
                return false;
            }
            options.cffTables = psMetadataLookupPtr(NULL, config->arguments, "CFF_TABLES");
            if (!options.cffTables) {
                psError (PS_ERR_UNKNOWN, true, "Cannot find cff table in arguments.");
                return false;
            }
        } else { 
            // Make sure that this input was created with the same galaxy shapes recipe
            if (!checkOptions(&options, readout, recipe)) {
                psError (PS_ERR_UNKNOWN, false, "galaxy shape options do not match for input %d", index);
                return false;
            }
        }

        // look up zero point
        psF32 zero_point = psMetadataLookupF32(&status, readout->parent->parent->parent->concepts, "FPA.ZP");
        psF32 exptime = psMetadataLookupF32(&status, readout->parent->parent->parent->concepts, "FPA.EXPOSURE");

        psF32 scaleFactor = pow(10, 0.4 * (outputZeroPoint - zero_point)) * outputExptime / exptime;
        fluxScaleFactor->data.F32[index] = scaleFactor;

        // find detections
        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");
        psArray *sources = detections->allSources;
        psAssert (sources, "missing sources?");
        sources = psArraySort (sources, pmSourceSortBySeq);

        if (objects == NULL) {
            pmSource *lastSource = sources->data[sources->n - 1];
            psAssert(lastSource, "last source is null!");
            objects = psArrayAlloc(lastSource->seq + 1);
        }

        for (int i=0; i < sources->n; i++) {
            pmSource *source = sources->data[i];
            source->imageID = index;
            findObjectForSource(&objects, source);
        }
    }

    pmDetections *outputDetections = pmDetectionsAlloc();
    if (!psMetadataAddPtr (outputReadout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", 
            PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", outputDetections)) {
        psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
        return false;
    }

    psArray *outputSources = psArrayAllocEmpty (objects->n);

    // Loop over objects and compute the summaries
    long nObjects = 0;
    long nSources = 0;
    for (int i=0 ; i<objects->n; i++) {
        pmPhotObj *obj = objects->data[i];
        if (!obj) continue;

        ++nObjects;
        pmSource *source = psphotFullForceSummarizeObject(config, obj, fluxScaleFactor, &options);
        if (source) {
            psArrayAdd (outputSources, 100, source);
            ++nSources;
        }
    }

    psLogMsg("psphot", PS_LOG_INFO, "constructed %ld output sources, from %ld objects.", nSources, nObjects);

    psFree(fluxScaleFactor);

    if (nSources) {
        // We have data
        outputDetections->allSources = outputSources;
        outputReadout->data_exists = true;
        outputReadout->parent->data_exists = true;
        outputReadout->parent->parent->data_exists = true;
    } else {
        // XXX: tooo set a quality or fault code
        return false;
    }

    return true;
}

static pmPhotObj *findObjectForSource(psArray **pObjects, pmSource *source) {
    int seq = source->seq;

    psArray *objects = *pObjects;
    if (seq >= objects->n) {
        // We need to expand the object array. Kind of suprising.
        objects = *pObjects = psArrayRealloc(objects, seq+1);
    }

    // Look up object for this seq
    pmPhotObj *obj = objects->data[seq];
    if (!obj) {
        // not found allocate one
        obj = pmPhotObjAlloc();
        objects->data[seq] = obj;
    }
    pmPhotObjAddSource(obj, source);

    return obj;
}

static pmSource *psphotFullForceSummarizeObject(pmConfig *config, pmPhotObj *obj, psVector *fluxScaleFactor, galaxyShapeSummaryOptions *options) {

    pmSource *outSrc = NULL;


    pmSource *source0 = obj->sources->data[0];
    int ID = source0->seq;

    // Find the parameters from the cff tables. nModels is the number of extended model types that
    // this source has an entry for
    int nModels = 0;
    // skip table 0 which is modelType -1 not extended
    for (int i=1; i<options->cffTables->n; i++) {
        psArray *table = options->cffTables->data[i];
        if (!table) continue;
        if (ID >= table->n) continue;
        if (table->data[ID]) {
            nModels++;
        }
    }

    // Loop over sources for this object. Start output source for first input that has
    // good pixel frac
    for (int i=0; i < obj->sources->n; i++) {
        pmSource *source = obj->sources->data[i];

        // XXX: get cut from recipe
        if (source->pixWeightNotPoor < .9) continue;

        // For now just start the output source as a copy of the first input source that makes cuts
        if (!outSrc) {
            outSrc = pmSourceCopy(source);
            // This copies 
            //  the peak 
            //  the moments which are mostly nan except for Mrf Mx, My, and some of the kron parameters
            //
            // type, mode, flags
            // magnitudes
            outSrc->imageID = 0;
            outSrc->seq = source->seq;

            if (source->modelPSF) {
                outSrc->modelPSF = psMemIncrRefCounter(source->modelPSF);
            }
            if (source->extpars) {
                outSrc->extpars =  psMemIncrRefCounter(source->extpars);
            }
        }
        if (nModels && source->modelFits && source->modelFits->n) {
            if (!outSrc->modelFits) {
                outSrc->modelFits = psArrayAllocEmpty(nModels);
            }
            for (int iModel = 0; iModel < source->modelFits->n; iModel++) {
                pmModel *newModel = source->modelFits->data[iModel];
                pmModel *outModel = NULL;
                for (int jModel = 0; jModel < outSrc->modelFits->n; jModel++) {
                    outModel = outSrc->modelFits->data[jModel];
                    if (newModel->type == outModel->type)  {
                        // already have an entry for this type
                        break;
                    }
                    outModel = NULL;
                }
                if (!outModel) {
                    // no model of this type in output source yet. Add this one.
                    // Note: the parameters that we need (position, angle, index, if applicable)
                    // are the same for all sources so copying any of them is fine
                    psArrayAdd(outSrc->modelFits, 1, newModel);
                }
            }
        }
    }

    if (!outSrc) {
        // no good measurements for this source (how?)
        return NULL;
    }

    // now loop over the model fits and summarize the galaxy shape measurements for each
    for (int iModel=0; outSrc->modelFits && iModel < outSrc->modelFits->n; iModel++) {
        pmModel *model = outSrc->modelFits->data[iModel];
        int modelType = model->type;
        psAssert(modelType >= 0 && modelType+1 < options->cffTables->n, "model type out of range");

        psArray *cffTable = options->cffTables->data[modelType+1];
        psAssert(cffTable, "missing cff table?");

        psMetadata *cffRow = cffTable->data[outSrc->seq];
        if (!cffRow) {
            psError (PS_ERR_PROGRAMMING, true, "NO cff data for object %d model %d", outSrc->seq, modelType);
            psFree(outSrc);
            return NULL;
        }
        bool mdok;
        psF32 R_MAJOR = psMetadataLookupF32(&mdok, cffRow, "R_MAJOR");
        if (!mdok) {
            psError (PS_ERR_PROGRAMMING, true, "can't find R_MAJOR for object %d type %d", outSrc->seq, modelType);
            psFree(outSrc);
            return NULL;
        }
        psF32 R_MINOR = psMetadataLookupF32(&mdok, cffRow, "R_MINOR");
        if (!mdok) {
            psError (PS_ERR_PROGRAMMING, true, "can't find R_MINOR for object %d type %d", outSrc->seq, modelType);
            psFree(outSrc);
            return NULL;
        }

        psVector *sumWeightedFlux = NULL;
        psVector *sumInvSig2 = NULL;
        psVector *numerator   = NULL;
        psVector *fRmajorVec = NULL;
        psVector *fRminorVec = NULL;
        psF32   totalNPix = 0;
        long    vectorLength = 0;
        psF32   fRmajorMin = 0;
        psF32   fRmajorMax = 0;
        psF32   fRmajorDel = 0;
        psF32   fRminorMin = 0;
        psF32   fRminorMax = 0;
        psF32   fRminorDel = 0;
        for (int i=0; i < obj->sources->n; i++) {
            pmSource *source = obj->sources->data[i];

            if (source->pixWeightNotPoor < .9) continue;

            // The only parameters that we are summarizing currently are galaxy shapes. 
            if (!source->modelFits) continue;
            if (!source->galaxyFits) continue;

            // XXX: put this into a function
            pmSourceGalaxyFits *galaxyFits = NULL;
            for (int j=0; j<source->galaxyFits->n; j++) {
                galaxyFits = source->galaxyFits->data[j];
                if (galaxyFits && galaxyFits->modelType == modelType) break;
                galaxyFits = NULL;
            }

            if (galaxyFits && galaxyFits->nPix && galaxyFits->chisq->n) {
                if (numerator == NULL) {
                    // first source with galaxyFits allocate accumulators
                    vectorLength = galaxyFits->chisq->n;
                    sumWeightedFlux = psVectorAlloc(vectorLength, PS_TYPE_F32);
                    psVectorInit(sumWeightedFlux, 0.0);
                    sumInvSig2 = psVectorAlloc(vectorLength, PS_TYPE_F32);
                    psVectorInit(sumInvSig2, 0.0);
                    numerator   = psVectorAlloc(vectorLength, PS_TYPE_F32);
                    psVectorInit(numerator, 0.0);

                    // Initialize the fractional radius vectors
                    // save these to initialize the output galaxy fits
                    fRmajorMin = galaxyFits->fRmajorMin;
                    fRmajorMax = galaxyFits->fRmajorMax;
                    fRmajorDel = galaxyFits->fRmajorDel;
                    fRminorMin = galaxyFits->fRminorMin;
                    fRminorMax = galaxyFits->fRminorMax;
                    fRminorDel = galaxyFits->fRminorDel;
                    fRmajorVec = psVectorAlloc(vectorLength, PS_TYPE_F32);
                    fRminorVec = psVectorAlloc(vectorLength, PS_TYPE_F32);
                    int k = 0;
                    for (float fRmajor = fRmajorMin; fRmajor < fRmajorMax + 0.5*fRmajorDel; fRmajor += fRmajorDel) {
                        for (float fRminor = fRminorMin; fRminor < fRminorMax + 0.5*fRminorDel; fRminor += fRminorDel) {
                            fRminorVec->data.F32[k] = fRminor;
                            fRmajorVec->data.F32[k] = fRmajor;
                            k++;
                        }
                    }
                    psAssert(k == vectorLength, "mismatched vectors");
                }

                // Die if the lengths of the vectors is not the same in all sources
#ifdef notdef
                psAssert(vectorLength == options->numTrials, "length of chisq vector %ld does not match options %d",
                    vectorLength, options->numTrials);
#endif
                psAssert(galaxyFits->chisq->n == vectorLength, "length of chisq vectors do not match %ld %ld",
                             galaxyFits->chisq->n, vectorLength);

                psF32 scaleFactor = fluxScaleFactor->data.F32[source->imageID];

                totalNPix += galaxyFits->nPix;

                for (int k = 0; k < vectorLength; k++) {
                    psF32 chisq = galaxyFits->chisq->data.F32[k];
                    psF32 flux  = galaxyFits->Flux->data.F32[k]  * scaleFactor;
                    psF32 dFlux = galaxyFits->dFlux->data.F32[k] * scaleFactor;

                    numerator->data.F32[k] += chisq * galaxyFits->nPix;

                    psF32 invSig2 = 1.0 / (dFlux * dFlux);
                    sumInvSig2->data.F32[k] += invSig2;

                    sumWeightedFlux->data.F32[k] += flux * invSig2;
                }
            }
        }

        if (vectorLength) {
            // allocate galaxyFits for the output source
            if (!outSrc->galaxyFits) {
                outSrc->galaxyFits = psArrayAllocEmpty(1);
            }
            pmSourceGalaxyFits *galaxyFits = pmSourceGalaxyFitsAlloc();
            psArrayAdd(outSrc->galaxyFits, 1, galaxyFits);
            psFree(galaxyFits);

            galaxyFits->nPix  = totalNPix;
            galaxyFits->modelType = modelType;
            psVector *fluxVec = 
                galaxyFits->Flux  = psVectorRecycle(galaxyFits->Flux,  vectorLength, PS_TYPE_F32);
            psVector *dFluxVec = 
                galaxyFits->dFlux = psVectorRecycle(galaxyFits->dFlux, vectorLength, PS_TYPE_F32);
            psVector *chisqVec = 
                galaxyFits->chisq = psVectorRecycle(galaxyFits->chisq, vectorLength, PS_TYPE_F32);

            galaxyFits->fRmajorMin = fRmajorMin;
            galaxyFits->fRmajorMax = fRmajorMax;
            galaxyFits->fRmajorDel = fRmajorDel;
            galaxyFits->fRminorMin = fRminorMin;
            galaxyFits->fRminorMax = fRminorMax;
            galaxyFits->fRminorDel = fRminorDel;

            // fill the summary galaxyFits vectors and find the trial with the minimum value for chisq

            int min_k = -1;
            psF32 minChisq = NAN;

            for (int k = 0; k < vectorLength; k++) {
                fluxVec->data.F32[k]  = sumWeightedFlux->data.F32[k] / sumInvSig2->data.F32[k];
                dFluxVec->data.F32[k] = 1.0 / sumInvSig2->data.F32[k];

                psF32 thischisq = chisqVec->data.F32[k] = numerator->data.F32[k] / totalNPix;

                if (isfinite(thischisq)  && (!isfinite(minChisq) || thischisq < minChisq)) {
                    min_k = k;
                    minChisq = thischisq;
                }
            }

            psFree(numerator);
            psFree(sumInvSig2);
            psFree(sumWeightedFlux);

            if (min_k >= 0 && isfinite(minChisq)) {
                // copy the best fit params to the model
                // fractional radii with the lowest chisq
                psEllipseAxes axes = pmPSF_ModelToAxes(model->params->data.F32, model->class->useReff);

                // examine the params for the trial with minimum chisq.
                bool fitMajor = true;
                bool fitMinor = true;
                psF64 fRmajorBest = fRmajorVec->data.F32[min_k];
                if ((fabs(fRmajorBest - galaxyFits->fRmajorMin) < galaxyFits->fRmajorDel) || 
                    (fabs(fRmajorBest - galaxyFits->fRmajorMax) < galaxyFits->fRmajorDel)) {
                    fitMajor = false;
                }
                psF64 fRminorBest = fRminorVec->data.F32[min_k];
                if ((fabs(fRminorBest - galaxyFits->fRminorMin) < galaxyFits->fRminorDel) || 
                    (fabs(fRminorBest - galaxyFits->fRminorMax) < galaxyFits->fRminorDel)) {
                    fitMinor = false;
                }
                // If either major or minor is at one of the limits do not fit report the minimum value
                bool useFit = fitMajor && fitMinor;

                // save the flux and dFlux values from entry with lowest chisq
                psF64 fluxBest = fluxVec->data.F32[min_k];
                psF64 dFluxBest = dFluxVec->data.F32[min_k];
                psF64 dFlux0   = NAN;
                psF64 flux0    = NAN;

                if (useFit) {
                    #define NUM_TRIALS_INIT 9
                    psVector *major = psVectorAllocEmpty(NUM_TRIALS_INIT, PS_TYPE_F64);
                    psVector *minor = psVectorAllocEmpty(NUM_TRIALS_INIT, PS_TYPE_F64);
                    psVector *chisq = psVectorAllocEmpty(NUM_TRIALS_INIT, PS_TYPE_F64);
                    psVector *flux = psVectorAllocEmpty(NUM_TRIALS_INIT, PS_TYPE_F64);
                    psVector *dFlux = psVectorAllocEmpty(NUM_TRIALS_INIT, PS_TYPE_F64);

                    // XXX: use a recipe parameter instead of 2.2
                    psF64 maxDeltaMaj = 2.2 * galaxyFits->fRmajorDel;
                    psF64 maxDeltaMin = 2.2 * galaxyFits->fRminorDel;

                    psF64 majorMin = NAN;
                    psF64 majorMax = NAN;
                    psF64 minorMin = NAN;
                    psF64 minorMax = NAN;
                    psF64 chisqMin = NAN;
                    psF64 chisqMax = NAN;
                    for (int k = 0; k < vectorLength; k++) {

                        if (fabs(fRmajorVec->data.F32[k] - fRmajorBest) < maxDeltaMaj &&
                            fabs(fRminorVec->data.F32[k] - fRminorBest) < maxDeltaMin) {

                            if (isfinite(chisqVec->data.F32[k]) && 
                                isfinite(fluxVec->data.F32[k])  &&
                                isfinite(dFluxVec->data.F32[k])) {

                                // compute major and minor radius vectors from nominal and trial fractions
                                // also find the ranges in the vectors
                                psF64 thisMajor = R_MAJOR * fRmajorVec->data.F32[k];
                                if (!isfinite(majorMin) || thisMajor < majorMin) {
                                    majorMin = thisMajor;
                                }
                                if (!isfinite(majorMax) || thisMajor > majorMax) {
                                    majorMax = thisMajor;
                                }
                                psVectorAppend(major, thisMajor);

                                psF64 thisMinor = R_MINOR * fRminorVec->data.F32[k];
                                if (!isfinite(minorMin) || thisMinor < minorMin) {
                                    minorMin = thisMinor;
                                }
                                if (!isfinite(minorMax) || thisMinor > minorMax) {
                                    minorMax = thisMinor;
                                }
                                psVectorAppend(minor, thisMinor);

                                psF64 thisChisq = chisqVec->data.F32[k];
                                if (!isfinite(chisqMin) || thisChisq < chisqMin) {
                                    chisqMin = thisChisq;
                                }
                                if (!isfinite(chisqMax) || thisChisq > chisqMax) {
                                    chisqMax = thisChisq;
                                }
                                psVectorAppend(chisq, thisChisq);

                                psVectorAppend(flux,  fluxVec->data.F32[k]);
                                psVectorAppend(dFlux, dFluxVec->data.F32[k]);
                            }
                        }
                    }


                    // see if we ever get too few good values (haven't seen this happen)
                    if (major->n < NUM_TRIALS_INIT) {
                        fprintf(stderr, "only found %ld good points wanted %d for seq: %d\n",
                            major->n, NUM_TRIALS_INIT, outSrc->seq); 
                    }

                    // rescale data - this helps avoid precision errors.
                    psF64 majorRange = majorMax - majorMin;
                    if (majorRange == 0) {
                        majorRange = 1;
                    }
                    psF64 minorRange = minorMax - minorMin;
                    if (minorRange == 0) {
                        minorRange = 1;
                    }
                    psF64 chisqRange = chisqMax - chisqMin;
                    if (chisqRange == 0) {
                        chisqRange = 1;
                    }
                    for (int k = 0; k < major->n; k++) {
                        major->data.F64[k] = (major->data.F64[k] - majorMin) / majorRange;
                        minor->data.F64[k] = (minor->data.F64[k] - minorMin) / minorRange;
                        chisq->data.F64[k] = (chisq->data.F64[k] - chisqMin) / chisqRange;
                    }

                    // Fit chisq versus rMajor and rMinor
                    psPolynomial2D *chisqFit = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
                    psPolynomial2D *fluxFit = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
                    psPolynomial2D *fluxErrorFit = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
                    chisqFit->coeffMask[2][2] = PS_POLY_MASK_SET;
                    chisqFit->coeffMask[2][1] = PS_POLY_MASK_SET;
                    chisqFit->coeffMask[1][2] = PS_POLY_MASK_SET;
                    fluxFit->coeffMask[2][2] = PS_POLY_MASK_SET;
                    fluxFit->coeffMask[2][1] = PS_POLY_MASK_SET;
                    fluxFit->coeffMask[1][2] = PS_POLY_MASK_SET;
                    fluxErrorFit->coeffMask[2][2] = PS_POLY_MASK_SET;
                    fluxErrorFit->coeffMask[2][1] = PS_POLY_MASK_SET;
                    fluxErrorFit->coeffMask[1][2] = PS_POLY_MASK_SET;

                    bool goodFit = psVectorFitPolynomial2D (chisqFit, NULL, 0xff, chisq, NULL, major, minor);

                    if (goodFit) {
                        // find the axes for the minimum chisq from the formula
                        psF64 **coeff = chisqFit->coeff;
                        psF64 D = 4*coeff[2][0]*coeff[0][2] - coeff[1][1]*coeff[1][1];
                        psF64 major0 = ((coeff[1][1]*coeff[0][1] - 2*coeff[0][2]*coeff[1][0]) / D) * majorRange + majorMin;
                        psF64 minor0 = ((coeff[1][1]*coeff[1][0] - 2*coeff[2][0]*coeff[0][1]) / D) * minorRange + minorMin;

                        // estimated chisq at the minimum
                        psF64 chisq0 = psPolynomial2DEval(chisqFit, major0, minor0) * chisqRange + chisqMin;

                        // now fit the flux ...
                        bool goodFluxFit = psVectorFitPolynomial2D(fluxFit, NULL, 0xFF, flux, NULL, major, minor);

                        // .. and compute flux at the minimum chisq position
                        if (goodFluxFit) {
                            flux0  = psPolynomial2DEval(fluxFit, major0, minor0);
                        } else {
                            flux0 = fluxBest;
                        }

                        // .. and compute dFlux at the minimum chisq position
                        bool goodFluxErrorFit = psVectorFitPolynomial2D(fluxErrorFit, NULL, 0xFF, dFlux, NULL, major, minor);
                        if (goodFluxErrorFit) {
                            dFlux0  = psPolynomial2DEval(fluxErrorFit, major0, minor0);
                        } else {
                            dFlux0 = dFluxBest;
                        }

#ifdef PRINTVALS
                        fprintf (stderr, "%4d %3d %3ld | %6.3f %6.3f %6.3f %4.2f |  %6.3f %6.3f %6.3f %4.2f | %7.4f %7.4f %7.1f %7.1f\n", 
                            outSrc->seq, min_k, major->n, R_MAJOR, fRmajorBest*R_MAJOR, major0, major0/R_MAJOR, R_MINOR, fRminorBest*R_MINOR, minor0, minor0/R_MINOR, minChisq, chisq0, fluxBest, flux0);
#endif 


                        axes.major = major0;
                        axes.minor = minor0;
                        model->chisq = chisq0;
#ifdef DUMPVECTORS
                        char fn[80];

                        sprintf(fn, "vectors/%s.%05d.txt", goodFit ? "g" : "b", outSrc->seq);

                        FILE *f = fopen(fn, "w");
                        fprintf(f, "#major minor chisq i\n");
                        for (int k = 0; k < chisq->n; k++) {
                            fprintf(f, "%10.6f %10.6f %10.6f %5d\n", 
                                major->data.F64[k]/R_MAJOR, minor->data.F64[k]/R_MINOR, chisq->data.F64[k], k);
                        }
                        fclose(f);
#endif
                    } else {
                        model->flags |= PM_MODEL_STATUS_NONCONVERGE;

#ifdef PRINTVALS
                        fprintf(stderr, "%4d %3d %3ld | %6.3f %6.3f %6.3f %6.3f bad fit\n", 
                            outSrc->seq, min_k, major->n, R_MAJOR, fRmajorBest, R_MINOR, fRminorBest);
#endif
                        psErrorClear();
                        // psFree(outSrc->galaxyFits);

                    }
                    psFree(chisqFit);
                    psFree(fluxFit);
                    psFree(fluxErrorFit);
                    psFree(major);
                    psFree(minor);
                    psFree(chisq);
                    psFree(flux);
                    psFree(dFlux);
                } else {
                    // No fit
                    // Set the values based on the trial with the best chisq
                    axes.major = R_MAJOR * fRmajorBest;
                    axes.minor = R_MINOR * fRminorBest;
                    model->chisq = minChisq; 
                    flux0 = fluxBest;
                    dFlux0 = dFluxBest;
#ifdef PRINTVALS
                    fprintf(stderr, "%4d %3d %3ld | %6.3f %6.3f %6.3f %6.3f skip fit\n", 
                            outSrc->seq, min_k, 0L, R_MAJOR, fRmajorBest, R_MINOR, fRminorBest);
#endif
                }
                // now save the model parameters in the model structure

		// a crazy model can raise an assert in psEllise
	        if ((fabs(axes.major) > 10000 * fabs(axes.minor)) || (fabs(axes.minor) > 10000 * fabs(axes.major))) {
		  model->params->data.F32[PM_PAR_SXX] = NAN;
		  model->params->data.F32[PM_PAR_SYY] = NAN;
		  model->params->data.F32[PM_PAR_SXY] = NAN;
		} else {
		  pmPSF_AxesToModel (model->params->data.F32, axes, model->class->useReff);
		}

                model->mag = -2.5 * log10(flux0);
                model->magErr = dFlux0 / flux0; // 1 / SN
                // XXX: should there be a different flag if we didn't do a fit of the chisq
                model->flags |= PM_MODEL_STATUS_FITTED;
            }
        }
        psFree(fRminorVec);
        psFree(fRmajorVec);
    }

    return outSrc;
}

#define GETVAL(member, key) \
    opt->member = psMetadataLookupF32(&status, md, key); \
    if (!status) { \
        psError (PSPHOT_ERR_CONFIG, true, "failed to looup value for %s in %s", key, \
            useAnalysis ? "readout->analysis" : "recipe"); \
    }

static bool setOptions(galaxyShapeSummaryOptions *opt, pmReadout *readout, psMetadata *recipe, bool makeVectors) {
    bool status;
    bool useAnalysis;   // fall back to recipe if we dont' find values in analysis. Probably should no longer do this

#ifdef notdef
    psMetadataLookupF32(&useAnalysis, readout->analysis, "GALAXY_SHAPES_FR_MAJOR_MIN");
#endif
    psMetadataLookupF32(&useAnalysis, readout->analysis, "GALAXY_SHAPES_Q");
    psMetadata *md = useAnalysis ? readout->analysis : recipe;

    GETVAL(Q, "GALAXY_SHAPES_Q");
    GETVAL(NSigma, "GALAXY_SHAPES_NSIGMA");
    GETVAL(clampSN, "GALAXY_SHAPES_CLAMP_SN");

#ifdef notdef
    // these are per object now
    GETVAL(fRmajorMin, "GALAXY_SHAPES_FR_MAJOR_MIN");
    GETVAL(fRmajorMax, "GALAXY_SHAPES_FR_MAJOR_MAX");
    GETVAL(fRmajorDel, "GALAXY_SHAPES_FR_MAJOR_DEL");
    GETVAL(fRminorMin, "GALAXY_SHAPES_FR_MINOR_MIN");
    GETVAL(fRminorMax, "GALAXY_SHAPES_FR_MINOR_MAX");
    GETVAL(fRminorDel, "GALAXY_SHAPES_FR_MINOR_DEL");

    opt->numTrials = ceil((opt->fRmajorMax - opt->fRmajorMin + 0.5*opt->fRmajorDel) / opt->fRmajorDel) *
                         ceil((opt->fRminorMax - opt->fRminorMin + 0.5*opt->fRminorDel) / opt->fRminorDel) ;

    if (makeVectors) {
        opt->fRminor = psVectorAlloc(opt->numTrials, PS_TYPE_F32);
        opt->fRmajor = psVectorAlloc(opt->numTrials, PS_TYPE_F32);
        int i = 0;
        for (float fRmajor = opt->fRmajorMin; fRmajor < opt->fRmajorMax + 0.5*opt->fRmajorDel;
                fRmajor += opt->fRmajorDel) {
            for (float fRminor = opt->fRminorMin; fRminor < opt->fRminorMax + 0.5*opt->fRminorDel;
                    fRminor += opt->fRminorDel) {
                opt->fRminor->data.F32[i] = fRminor;
                opt->fRmajor->data.F32[i] = fRmajor;
                i++;
            }
        }
        psAssert(i == opt->numTrials, "Something's wrong with my loop got %d entries expected %d", i, opt->numTrials);
    } else {
        opt->fRminor = NULL;
        opt->fRmajor = NULL;
    }
#endif
        
    return true;
}

#define CHECKVAL(left, right, val, message) \
    if (left->val != right.val) { \
        psError (PSPHOT_ERR_CONFIG, true, message); \
        return false; \
    }

static bool checkOptions(galaxyShapeSummaryOptions *options, pmReadout *readout, psMetadata *recipe) {
    galaxyShapeSummaryOptions thisReadoutsOptions;
    
    if (!setOptions(&thisReadoutsOptions, readout, recipe, false)) {
        psError (PS_ERR_UNKNOWN, false, "problem determining galaxy shape options for readout");
        return false;
    }
    CHECKVAL(options, thisReadoutsOptions, Q, "mismatched Q");
    CHECKVAL(options, thisReadoutsOptions, NSigma, "mismatched NSIGMA");
    CHECKVAL(options, thisReadoutsOptions, clampSN, "mismatched cleampSN");

#ifdef notdef
    // these are per object now
    CHECKVAL(options, thisReadoutsOptions, numTrials, "mismatched number of trials")
    CHECKVAL(options, thisReadoutsOptions, fRmajorMin, "mismatched fRmajorMin")
    CHECKVAL(options, thisReadoutsOptions, fRmajorMax, "mismatched fRmajorMax")
    CHECKVAL(options, thisReadoutsOptions, fRmajorDel, "mismatched fRmajorDel")
    CHECKVAL(options, thisReadoutsOptions, fRminorMin, "mismatched fRminorMin")
    CHECKVAL(options, thisReadoutsOptions, fRminorMax, "mismatched fRminorMax")
    CHECKVAL(options, thisReadoutsOptions, fRminorDel, "mismatched fRminorDel")
#endif

    return true;
}
