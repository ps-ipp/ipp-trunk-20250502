#include "ppStack.h"

//#define TESTING

// Update the value of a concept
#define UPDATE_CONCEPT(SOURCE, NAME, VALUE) {				\
	psMetadataItem *item = psMetadataLookup(SOURCE->concepts, NAME); \
	psAssert(item, "Concept should be present");			\
	psAssert(item->type == PS_DATA_F32, "Concept should be F32");	\
	item->data.F32 = VALUE;						\
    }

bool ppStackConvolve(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psTimerStart("PPSTACK_CONVOLVE");
    
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    int num = options->num;             // Number of inputs
    options->cells = psArrayAlloc(num); // Cells for convolved images --- a handle for reading again
    options->kernels = psArrayAlloc(num); // PSF-matching kernels --- required in the stacking
    options->regions = psArrayAlloc(num); // PSF-matching regions --- required in the stacking
    int numGood = 0;                    // Number of good frames
    options->numCols = 0;
    options->numRows = 0;
    options->matchChi2 = psVectorAlloc(num, PS_TYPE_F32); // chi^2 for stamps when matching
    psVectorInit(options->matchChi2, NAN);
    options->weightings = psVectorAlloc(num, PS_TYPE_F32); // Combination weightings for images (1/noise^2)
    psVectorInit(options->weightings, 0.0);
    options->origCovars = psArrayAlloc(num);
    options->convCovars = psArrayAlloc(num); // Covariance matrices

    psImage *target = NULL;             // Target PSF image
    if (options->convolve) {
        target = ppStackTarget(options, config);
        if (!target) {
            psError(psErrorCodeLast(), false, "Unable to produce stack target image");
            return false;
        }
    }

    psVector *renorms = psVectorAlloc(num, PS_TYPE_F32); // Renormalisation values for variances
    psVectorInit(renorms, NAN);

    psVector *satValues = psVectorAllocEmpty(num, PS_TYPE_F32); // Renormalisation values for variances

    psList *fpaList = psListAlloc(NULL); // List of input FPAs, for concept averaging
    psList *cellList = psListAlloc(NULL); // List of input cells, for concept averaging
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    for (int i = 0; i < num; i++) {
        if (options->inputMask->data.U8[i]) {
            continue;
        }
        psTrace("ppStack", 2, "Convolving input %d of %d to target PSF....\n", i, num);
        pmFPAfileActivate(config->files, false, NULL);
        ppStackFileActivationSingle(config, PPSTACK_FILES_CONVOLVE, true, i);
        if (options->convolve) {
            // PPSTACK.CONV.KERNEL not defined unless convolve
            pmFPAfileActivateSingle(config->files, true, "PPSTACK.CONV.KERNEL", i); // Activated file
        }

        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i); // File of interest
        pmFPAview *view = ppStackFilesIterateDown(config);
        if (!view) {
            psFree(rng);
            psFree(fpaList);
            psFree(cellList);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa); // Input readout
        psFree(view);

        if (options->numCols == 0 && options->numRows == 0) {
            options->numCols = readout->image->numCols;
            options->numRows = readout->image->numRows;
        } else if (options->numCols != readout->image->numCols ||
                   options->numRows != readout->image->numRows) {
            psError(PPSTACK_ERR_ARGUMENTS, true, "Sizes of input images don't match: %dx%d vs %dx%d",
                    readout->image->numCols, readout->image->numRows, options->numCols, options->numRows);
            psFree(rng);
            psFree(fpaList);
            psFree(cellList);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }

        // Background subtraction, scaling and normalisation is performed automatically by the image matching
        psTimerStart("PPSTACK_MATCH");
        options->origCovars->data[i] = psMemIncrRefCounter(readout->covariance);
        if (!ppStackMatch(readout, target, options, i, config)) {
            // XXX many things can cause a failure of ppStackMatch -- should some be handled differently?

	    // gcc -Wswitch complains here if err is declared as type psErrorCode
	    // the collection of ps*ErrorCode values are enums defined separately for 
	    // each module (psphot, pswarp, etc).  the lowest type, psErrorCode is only the base set and does
	    // not include the possible psphot values

	    // for now, to get around this, we just use an int for the switch

	    // psErrorCode error = psErrorCodeLast(); // Error code
	    int error = psErrorCodeLast(); // Error code
            switch (error) {
                // Fatal errors
              case PM_ERR_CONFIG:
              case PPSTACK_ERR_CONFIG:
              case PPSTACK_ERR_IO:
                psError(error, false, "Unable to match image %d due to fatal error.", i);
                psFree(rng);
                psFree(fpaList);
                psFree(cellList);
                psFree(target);
		psFree(renorms);
		psFree(satValues);
                return false;
                // Non-fatal errors
              case PM_ERR_STAMPS:
              case PM_ERR_SMALL_AREA:
              case PPSTACK_ERR_DATA:
              default:
                psErrorStackPrint(stderr, "Unable to match image %d --- ignoring.", i);
                options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PPSTACK_MASK_MATCH;
                psErrorClear();
                if (!ppStackFilesIterateUp(config)) {
                    psFree(rng);
                    psFree(fpaList);
                    psFree(cellList);
                    psFree(target);
		    psFree(renorms);
		    psFree(satValues);
                    return false;
                }
                continue;
            }
        }
        options->convCovars->data[i] = psMemIncrRefCounter(readout->covariance);

        float renorm = psMetadataLookupF32(NULL, readout->analysis, PM_READOUT_ANALYSIS_RENORM);
        if (!isfinite(renorm)) {
            renorm = 1.0;
        }
        renorms->data.F32[i] = renorm;

        if (options->stats) {
            psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_MATCH", PS_META_DUPLICATE_OK,
                             "Time to match PSF", psTimerMark("PPSTACK_MATCH"));
            psMetadataAddF32(options->stats, PS_LIST_TAIL, "PPSTACK.WEIGHTING", PS_META_DUPLICATE_OK,
                             "Weighting for image", options->weightings->data.F32[i]);

            if (options->convolve) {
                // Pull parameters out of convolution kernel
                pmSubtractionKernels *kernels = psMetadataLookupPtr(NULL, readout->analysis,
                                                                    PM_SUBTRACTION_ANALYSIS_KERNEL);
                psMetadataAddF32(options->stats, PS_LIST_TAIL, "STAMP.MEAN", PS_META_DUPLICATE_OK,
                                 "Mean deviation for stamps", kernels->mean);
                psMetadataAddF32(options->stats, PS_LIST_TAIL, "STAMP.RMS", PS_META_DUPLICATE_OK,
                                 "RMS deviation for stamps", kernels->rms);
                psMetadataAddF32(options->stats, PS_LIST_TAIL, "STAMP.NUM", PS_META_DUPLICATE_OK,
                                 "Number of stamps", kernels->numStamps);
                float deconv = psMetadataLookupF32(NULL, readout->analysis,
                                                   PM_SUBTRACTION_ANALYSIS_DECONV_MAX);
                psMetadataAddF32(options->stats, PS_LIST_TAIL, "KERNEL.DECONV", PS_META_DUPLICATE_OK,
                                 "Deconvolution fraction for kernel", deconv);
            }
        }
        psLogMsg("ppStack", PS_LOG_INFO, "Time to match image %d: %f sec", i, psTimerClear("PPSTACK_MATCH"));

        // Write the temporary convolved files
        pmHDU *hdu = readout->parent->parent->parent->hdu; // HDU for convolved image
        assert(hdu);
        if (!ppStackWriteImage(options->convImages->data[i], hdu->header, readout->image, config)) {
            psError(PPSTACK_ERR_IO, false, "Unable to write convolved image %d", i);
            psFree(fpaList);
            psFree(cellList);
            psFree(rng);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }
        psMetadata *maskHeader = psMetadataCopy(NULL, hdu->header); // Copy of header, for mask
        pmConfigMaskWriteHeader(config, maskHeader);
        if (!ppStackWriteImage(options->convMasks->data[i], maskHeader, readout->mask, config)) {
            psError(PPSTACK_ERR_IO, false, "Unable to write convolved mask %d", i);
            psFree(fpaList);
            psFree(cellList);
            psFree(rng);
            psFree(maskHeader);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }
        psFree(maskHeader);
	psImage *covarImage = readout->covariance ? readout->covariance->image : NULL;
        if (!ppStackWriteVariance(options->convVariances->data[i], hdu->header, readout->variance, covarImage, config)) {
            psError(PPSTACK_ERR_IO, false, "Unable to write convolved variance %d", i);
            psFree(fpaList);
            psFree(cellList);
            psFree(rng);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }
#ifdef TESTING
        {
            psString name = NULL;
	    if (readout->covariance) {
	      psStringAppend(&name, "covariance_%d.fits", i);
	      ppStackWriteImage(name, hdu->header, readout->covariance->image, config);
	      pmStackVisualPlotTestImage(readout->covariance->image, name);
	      psFree(name);
	    }
        }
        {
            int numCols = readout->image->numCols, numRows = readout->image->numRows;
            psImage *sn = psImageAlloc(numCols, numRows, PS_TYPE_F32);
            for (int y = 0; y < numRows; y++) {
                for (int x = 0; x < numCols; x++) {
                    sn->data.F32[y][x] = readout->image->data.F32[y][x] /
                        sqrtf(readout->variance->data.F32[y][x]);
                }
            }
            psString name = NULL;
            psStringAppend(&name, "signoise_%d.fits", i);
            ppStackWriteImage(name, hdu->header, sn, config);
            psFree(name);
            psFree(sn);
        }
#endif

        pmCell *inCell = readout->parent; // Input cell

        psListAdd(cellList, PS_LIST_TAIL, inCell);
        psListAdd(fpaList, PS_LIST_TAIL, inCell->parent->parent);

        // Correct ZP
        if (options->matchZPs) {
            // I think I need to take off the exposure time because we're going to set the new exposure time
	    // Clarification: the zero point (ZP) in the header should be set such that:
	    // M_app = m_inst + ZP + 2.5*log(exptime), where exptime in the output is sumExposure
            psMetadataItem *zpItem = psMetadataLookup(inCell->parent->parent->concepts, "FPA.ZP");
	    float inZP = zpItem->data.F32;
            zpItem->data.F32 += options->norm->data.F32[i] + 2.5*log10(options->sumExposure);

            psMetadataItem *expItem = psMetadataLookup(inCell->parent->parent->concepts, "FPA.EXPOSURE");
            expItem->data.F32 = options->sumExposure;

            expItem = psMetadataLookup(inCell->concepts, "CELL.EXPOSURE");
	    float inExptime = expItem->data.F32;
            expItem->data.F32 = options->sumExposure;

	    // flux_out = flux_in * ten(-0.4*norm) -- save the individual saturation values
            psMetadataItem *satItem = psMetadataLookup(inCell->concepts, "CELL.SATURATION");
	    float inSat = satItem->data.F32;
	    satItem->data.F32 *= pow(10.0, -0.4*options->norm->data.F32[i]);
            psVectorAppend (satValues, satItem->data.F32);

	    psLogMsg("ppStack", PS_LOG_INFO, "image %d mods : zp %f -> %f, exptime %f -> %f, sat %f -> %f", 
		     i, inZP, zpItem->data.F32, inExptime, expItem->data.F32, inSat, satItem->data.F32);
        }

        options->cells->data[i] = psMemIncrRefCounter(inCell);
        if (!ppStackFilesIterateUp(config)) {
            psFree(fpaList);
            psFree(cellList);
            psFree(rng);
            psFree(target);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }
        numGood++;

        ppStackMemDump("match");
    }
    psFree(rng);
    psFree(target);

    psFree(options->sourceLists); options->sourceLists = NULL;
    psFree(options->psf); options->psf = NULL;


    if (numGood == 0) {
        options->quality = PPSTACK_ERR_REJECTED;
        psErrorStackPrint(stderr, "No good images survived convolution stage.");
        psErrorClear();
        psWarning("No good images survived convolution stage.");
        psFree(fpaList);
        psFree(cellList);
	psFree(renorms);
	psFree(satValues);
        return true;
    }

    // Update concepts for output
    // XXX This should probably be placed later, so that it's not influenced
    // by images that get rejected later.
    {
        pmFPAview view;                 // View for output
        view.chip = view.cell = view.readout = 0;

        pmCell *outCell = pmFPAfileThisCell(config->files, &view, "PPSTACK.OUTPUT"); // Output cell
        pmFPA *outFPA = outCell->parent->parent; // Output FPA

        pmCell *unconvCell = pmFPAfileThisCell(config->files, &view, "PPSTACK.UNCONV"); // Unconvolved cell
        pmFPA *unconvFPA = unconvCell->parent->parent;                                  // Unconvolved FPA

        pmConceptsAverageFPAs(outFPA, fpaList);
        pmConceptsAverageCells(outCell, cellList, NULL, NULL, true);

        pmConceptsAverageFPAs(unconvFPA, fpaList);
        pmConceptsAverageCells(unconvCell, cellList, NULL, NULL, true);

        psFree(fpaList);
        psFree(cellList);
	
	// The best guess for an output saturation value depends on the recipe.  If we have
	// 'safe' on, the we require at least 2 pixels to generate a valid output pixel.  In
	// this case, the best value for CELL.SATURATION is the 2nd highest value in the list.
	// If not, it should be the higest value in the list
	bool mdok = false;
	bool safe = psMetadataLookupBool(&mdok, recipe, "SAFE"); // Be safe when combining small numbers of pixels
	psVectorSortInPlace(satValues);
	float satBest = safe && satValues->n > 1 ? satValues->data.F32[1] : satValues->data.F32[0];

	// UPDATE CELL.SATURATION here
        UPDATE_CONCEPT(outFPA,  "FPA.EXPOSURE",    options->sumExposure);
        UPDATE_CONCEPT(outCell, "CELL.EXPOSURE",   options->sumExposure);
        UPDATE_CONCEPT(outCell, "CELL.DARKTIME",   NAN);
        UPDATE_CONCEPT(outCell, "CELL.SATURATION", satBest);
        UPDATE_CONCEPT(outFPA,  "FPA.ZP",          options->zp);
        UPDATE_CONCEPT(outFPA,  "FPA.AIRMASS",     options->airmass);

        UPDATE_CONCEPT(unconvFPA,  "FPA.EXPOSURE",    options->sumExposure);
        UPDATE_CONCEPT(unconvCell, "CELL.EXPOSURE",   options->sumExposure);
        UPDATE_CONCEPT(unconvCell, "CELL.DARKTIME",   NAN);
        UPDATE_CONCEPT(unconvCell, "CELL.SATURATION", satBest);
        UPDATE_CONCEPT(unconvFPA,  "FPA.ZP",          options->zp);
        UPDATE_CONCEPT(unconvFPA,  "FPA.AIRMASS",     options->airmass);

	psLogMsg("ppStack", PS_LOG_INFO, "stack adjust metadata values : zp %f, exptime %f, sat %f", options->zp, options->sumExposure, satBest);

        if (options->stats) {
            psTime *fpaTime = psMetadataLookupPtr(NULL, outFPA->concepts, "FPA.TIME");
            psTimeConvert(fpaTime, PS_TIME_TAI); // is this necessary??
            double time = psTimeToMJD(fpaTime);
            psMetadataAddF64(options->stats, PS_LIST_TAIL, "MJD_OBS", PS_META_DUPLICATE_OK,
			     "Average MJD_OBS of inputs", time);

        }
    }

    // XXX EAM : this may be overly harsh -- or at least it would be if I (EAM) hadn't changed
    // the values of matchChi2 I modified pmSubtraction.c to fit a 2nd order polynomial to the
    // star chisq distribution (because of systematic errors in the model being matched).  This
    // fit forces the mean value to be 0.0.  Perhaps we can / should exclude images which have
    // an excessively high value for the rms?

    // Reject images out-of-hand on the basis of their match chi^2
    if (options->convolve) {
        psVector *values = psVectorAllocEmpty(num, PS_TYPE_F32); // Values to sort
        for (int i = 0; i < num; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_ALL) {
                continue;
            }
            values->data.F32[values->n++] = options->matchChi2->data.F32[i];
        }
        assert(values->n == numGood);
        if (!psVectorSortInPlace(values)) {
            psError(PPSTACK_ERR_PROG, false, "Unable to sort vector.");
            psFree(values);
	    psFree(renorms);
	    psFree(satValues);
            return false;
        }
        float median = numGood % 2 ? values->data.F32[numGood / 2] :
            0.5 * (values->data.F32[numGood / 2 - 1] + values->data.F32[numGood / 2]);

	// EAM/MEH ensure RMS is positive
        float rms = 0.74 * fabsf(values->data.F32[numGood * 3 / 4] -
                            values->data.F32[numGood / 4]); // Estimated RMS from interquartile range
        psFree(values);

        float rej = psMetadataLookupF32(NULL, recipe, "MATCH.REJ"); // Rejection threshold (stdevs)
        if (isfinite(rej)) {
            float thresh = median + rej * rms; // Threshold for rejection
            psLogMsg("ppStack", PS_LOG_INFO, "chi^2 rejection threshold = %f + %f * %f = %f",
                     median, rej, rms, thresh);

            int numRej = 0;                 // Number rejected
            numGood = 0;                    // Number of good images
            for (int i = 0; i < num; i++) {
                if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_ALL) {
                    continue;
                }
                if ((options->matchChi2->data.F32[i] > thresh) ||
		    ! isfinite(options->matchChi2->data.F32[i])) {
                    numRej++;
                    options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PPSTACK_MASK_CHI2;
                    psLogMsg("ppStack", PS_LOG_INFO, "Rejecting image %d because of large matching chi^2: %f",
                             i, options->matchChi2->data.F32[i]);
                } else {
                    psLogMsg("ppStack", PS_LOG_INFO, "Image %d has matching chi^2: %f",
                             i, options->matchChi2->data.F32[i]);
                    numGood++;
                }
            }
        }
    }

    if (numGood == 0) {
        options->quality = PPSTACK_ERR_REJECTED;
        psErrorStackPrint(stderr, "No good images survived convolution stage.");
        psErrorClear();
        psWarning("No good images survived convolution stage.");
	psFree(renorms);
	psFree(satValues);
        return true;
    }

    // Correct chi^2 for renormalisation
    psBinaryOp(options->matchChi2, options->matchChi2, "/", renorms);
    for (int i = 0; i < num; i++) {
        psLogMsg("ppStack", PS_LOG_INFO, "Additional variance for image %d: %f\n",
                 i, options->matchChi2->data.F32[i]);
    }
    psFree(renorms);
    psFree(satValues);

    if (options->stats) {
      psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_CONVOLVE", PS_META_REPLACE,
		       "Time to convolve input images", psTimerMark("PPSTACK_CONVOLVE"));
    }
    
    return true;
}
