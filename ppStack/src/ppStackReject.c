#include "ppStack.h"

// This is the doomsday switch. 
// #define TESTING

bool ppStackReject(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

/*     if (!options->convolve) { */
/*         // No need to do complicated rejection when we haven't convolved */
/*         return true; */
/*     } */

    
    int num = options->num;             // Number of inputs

    // Construct a small convolution kernel to aid in rejection
    if (!options->convolve) {
      for (int i = 0; i < num; i++) {
	psArray *regions = psArrayAllocEmpty(1);
	psRegion *region = psRegionAlloc(0,options->numCols - 1, 0, options->numRows - 1);
	regions = psArrayAdd(regions,1, region);
	
	psArray *kernels = psArrayAllocEmpty(1);
	psVector *fwhms = psVectorAllocEmpty(1, PS_TYPE_F32);
	psVectorAppend(fwhms,5.0); // Should be a parameter
	psVector *orders = psVectorAllocEmpty(1, PS_TYPE_S32);
	psVectorAppend(orders,0);
	pmSubtractionKernels *kernel = pmSubtractionKernelsISIS(15,0,fwhms,orders,0,*region,PM_SUBTRACTION_MODE_2);
	kernels = psArrayAdd(kernels, 1, kernel);
	
	kernel->solution1 = psVectorAlloc(3, PS_TYPE_F64);
	psVectorSet(kernel->solution1, 0, 1.0);
	psVectorSet(kernel->solution1, 1, 1.0);
	psVectorSet(kernel->solution1, 2, 1.0);
	kernel->solution2 = psVectorAlloc(3, PS_TYPE_F64);
	psVectorSet(kernel->solution2, 0, 1.0);
	psVectorSet(kernel->solution2, 1, 1.0);
	psVectorSet(kernel->solution2, 2, 1.0);
	
	options->kernels->data[i] = kernels;
	options->regions->data[i] = regions;
      }
    }
	

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    psMetadata *ppsub = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    psAssert(ppsub, "We've thrown an error on this before.");

    float threshold = psMetadataLookupF32(NULL, recipe, "THRESHOLD.MASK"); // Threshold for mask deconvolution
    float imageRej = psMetadataLookupF32(NULL, recipe, "IMAGE.REJ"); // Maximum fraction of image to reject
                                                                     // before rejecting entire image
    int stride = psMetadataLookupS32(NULL, ppsub, "STRIDE"); // Size of convolution patches

    // Count images rejected out of hand
    int numRejected = 0;        // Number of inputs rejected completely
    for (int i = 0; i < num; i++) {
        if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            numRejected++;
        }
    }

    // Concatenate inspection lists
    for (int i = 0; i < num; i++) {
        if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            continue;
        }

        psThreadJob *job = psThreadJobAlloc("PPSTACK_INSPECT"); // Job to start
        psArrayAdd(job->args, 1, options->inspect);
        psArrayAdd(job->args, 1, options->rejected);
        PS_ARRAY_ADD_SCALAR(job->args, i, PS_TYPE_S32);
        if (!psThreadJobAddPending(job)) {
            return false;
        }
    }
    if (!psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Unable to concatenate inspection lists.");
        return false;
    }


    // Reject bad pixels
    if (psMetadataLookupS32(NULL, config->arguments, "NTHREADS") > 0) {
        pmStackRejectThreadsInit();
    }
    for (int i = 0; i < num; i++) {
        if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            continue;
        }
        psTimerStart("PPSTACK_REJECT");

#ifdef TESTING
        {
            psImage *mask = psPixelsToMask(NULL, options->inspect->data[i],
                                           psRegionSet(0, options->numCols - 1, 0, options->numRows - 1),
                                           0xff); // Mask image
            psString name = NULL;           // Name of image
            psStringAppend(&name, "inspect_%03d.fits", i);
            pmStackVisualPlotTestImage(mask, name);
            psFits *fits = psFitsOpen(name, "w");
            psFree(name);
            psFitsWriteImage(fits, NULL, mask, 0, NULL);
            psFree(mask);
            psFitsClose(fits);
        }
#endif

#ifdef TESTING
        {
            psImage *mask = psPixelsToMask(NULL, options->rejected->data[i],
                                           psRegionSet(0, options->numCols - 1, 0, options->numRows - 1),
                                           0xff); // Mask image
            psString name = NULL;           // Name of image
            psStringAppend(&name, "pre_reject_%03d.fits", i);
            pmStackVisualPlotTestImage(mask, name);
            psFits *fits = psFitsOpen(name, "w");
            psFree(name);
            psFitsWriteImage(fits, NULL, mask, 0, NULL);
            psFree(mask);
            psFitsClose(fits);
        }
#endif

        psPixels *reject = pmStackReject(options->inspect->data[i], options->numCols, options->numRows,
                                         threshold, stride, options->regions->data[i],
                                         options->kernels->data[i]); // Rejected pixels

        psFree(options->inspect->data[i]);
        options->inspect->data[i] = NULL;

        if (!reject) {
            psWarning("Rejection on image %d didn't work --- reject entire image.", i);
            numRejected++;
            options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_REJECT;
        } else {
            float frac = reject->n / (float)(options->numCols * options->numRows); // Pixel fraction
            psLogMsg("ppStack", PS_LOG_INFO, "%ld pixels rejected from image %d (%.1f%%)",
                     reject->n, i, frac * 100.0);
            if (frac > imageRej) {
                psWarning("Image %d rejected completely because rejection fraction (%.3f) "
                          "exceeds limit (%.3f)", i, frac, imageRej);
                psFree(reject);
                reject = NULL;
                options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PPSTACK_MASK_BAD;
                numRejected++;
            } else {
                // Add to list of pixels already rejected
                reject = psPixelsConcatenate(reject, options->rejected->data[i]);
                options->rejected->data[i] = psPixelsDuplicates(options->rejected->data[i], reject);
            }
        }

#ifdef TESTING
        {
            psImage *mask = psPixelsToMask(NULL, options->rejected->data[i],
                                           psRegionSet(0, options->numCols - 1, 0, options->numRows - 1),
                                           0xff); // Mask image
            psString name = NULL;           // Name of image
            psStringAppend(&name, "reject_%03d.fits", i);
            pmStackVisualPlotTestImage(mask, name);
            psFits *fits = psFitsOpen(name, "w");
            psFree(name);
            psFitsWriteImage(fits, NULL, mask, 0, NULL);
            psFree(mask);
            psFitsClose(fits);
        }
#endif

        if (options->stats) {
            psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_REJECT", PS_META_DUPLICATE_OK,
                             "Time to perform rejection", psTimerMark("PPSTACK_REJECT"));
            psMetadataAddS32(options->stats, PS_LIST_TAIL, "REJECT_PIXELS", PS_META_DUPLICATE_OK,
                             "Number of pixels rejected", reject ? reject->n : 0);
        }

        psFree(reject);
        psLogMsg("ppStack", PS_LOG_INFO, "Time to perform rejection on image %d: %f sec", i,
                 psTimerClear("PPSTACK_REJECT"));
    }

    psFree(options->inspect); options->inspect = NULL;

    if (options->stats) {
        psMetadataAddS32(options->stats, PS_LIST_TAIL, "REJECT_IMAGES", 0,
                         "Number of images rejected completely", numRejected);
    }

    return true;
}
