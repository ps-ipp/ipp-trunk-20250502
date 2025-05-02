/** @file pswarpTransformReadout.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.17 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 02:58:59 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

// Structure to hold the properties of a mask value
typedef struct {
    char *badMaskName;                  // name for "bad" (i.e., mask me please) pixels
    char *fallbackName;                 // Fallback name in case a bad mask name is not defined
    psImageMaskType defaultMaskValue;   // Default value in case a bad mask name and its fallback are not defined
    bool isBad; // include this value as part of the MASK.VALUE entry (generically bad)
} pmConfigMaskInfo;

static pmConfigMaskInfo warp_convolve_masks[] = {
    // Features of the detector
    { "DETECTOR",  NULL,       0x01, true }, // Something is wrong with the detector
    { "FLAT",      "DETECTOR", 0x01, true }, // Pixel doesn't flat-field properly
    { "DARK",      "DETECTOR", 0x01, true }, // Pixel doesn't dark-subtract properly
    { "BLANK",     "DETECTOR", 0x01, true }, // Pixel doesn't contain valid data
    { "CTE",       "DETECTOR", 0x01, false }, // Pixel has poor CTE
    { "BURNTOOL",  NULL,       0x04, false }, // Pixel has been touched by burntool
    // Invalid signal ranges
    { "SAT",       NULL,       0x02, true  }, // Pixel is saturated or non-linear
    { "LOW",       "SAT",      0x02, true  }, // Pixel is low
    { "SUSPECT",   NULL,       0x04, false }, // Pixel is suspected of being bad
    // Non-astronomical structures
    { "CR",        NULL,       0x08, true  }, // Pixel contains a cosmic ray
    { "SPIKE",     NULL,       0x08, false  }, // Pixel contains a diffraction spike
    { "GHOST",     NULL,       0x08, false  }, // Pixel contains an optical ghost
    { "STREAK",    NULL,       0x08, false  }, // Pixel contains a streak
    { "CROSSTALK", NULL,       0x08, false  }, // Pixel contains crosstalk data
    { "STARCORE",  NULL,       0x08, false  }, // Pixel contains a bright star core
    // Effects of convolution and interpolation
    { "CONV.BAD",  NULL,       0x02, true  }, // Pixel is bad after convolution with a bad pixel
    { "CONV.POOR", NULL,       0x04, false }, // Pixel is poor after convolution with a bad pixel
};

/**
 * NOTE: in this function, the coordinates are transformed from the OUTPUT to the INPUT
 */
bool pswarpTransformReadout(pmReadout *output, pmReadout *input, pmConfig *config, bool backgroundWarp)
{
    // XXX this implementation currently ignores the use of the region
    psImage *region = NULL;             ///< Region to transform

    psTimerStart("warp");

    // Get warp parameters
    bool mdok;                          ///< Status of MD lookup
    int nGridX = psMetadataLookupS32(NULL, config->arguments, "GRID.NX"); ///< Number of grid points in x
    int nGridY = psMetadataLookupS32(NULL, config->arguments, "GRID.NY"); ///< Number of grid points in y
    bool doApplyMaskNaN = psMetadataLookupBool(NULL, config->arguments, "APPLY.PIXELNAN"); ///NaN the pixels underneath masks

    psImageInterpolateMode interpolationMode = psMetadataLookupS32(NULL, config->arguments,
                                                                   "INTERPOLATION.MODE"); ///< Mode for interp

    int numKernels = psMetadataLookupS32(NULL, config->arguments, "INTERPOLATION.NUM"); ///< Number of kernels

    // load the recipe
    psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, PSWARP_RECIPE);
    psAssert (recipe, "missing recipe %s", PSWARP_RECIPE);

    // output mask bits
    psImageMaskType maskIn   = 0 ;
    if (doApplyMaskNaN) {
      maskIn   = psMetadataLookupImageMask(&mdok, recipe, "MASK.INPUT");
      psAssert(mdok, "MASK.INPUT was not defined");
    } 
    else {
      psMetadata *maskrecipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
      if (!maskrecipe) {
          psError(psErrorCodeLast(), false, "Unable to find MASKS recipe.");
          return false;
      }
      if (!pswarpMaskSetInMetadata(&maskIn, NULL, maskrecipe)) {
          psError(psErrorCodeLast(), false, "Unable to determine mask value.");
          return false;
      }
    }
    psImageMaskType maskPoor = pmConfigMaskGet("CONV.POOR", config);
    if (!maskPoor) {
        maskPoor = pmConfigMaskGet("POOR.WARP", config);
    }
    psImageMaskType maskBad  = pmConfigMaskGet("CONV.BAD", config);
    if (!maskBad) {
        maskBad  = pmConfigMaskGet("BAD.WARP", config);
    }

    if (!input->covariance) {
        input->covariance = psImageCovarianceNone();
    }

    float poorFrac = psMetadataLookupF32(NULL, config->arguments, "POOR.FRAC"); ///< Flux fraction for "poor"

    // pswarpMapGridFromImage builds a set of locally-linear maps which convert the
    // output coordinates to input coordinates
    pswarpMapGrid *grid = pswarpMapGridFromImage(input, output, nGridX, nGridY);
    
    // XXX optionally modify the grid based on this result and force the maxError < XXX
    double maxError = pswarpMapGridMaxError(grid); // Maximum (positional) error from using grid
    psLogMsg("pswarp", 3, "maximum error using this grid sampling: %lf\n", maxError);

    // Get range of interest
    int xOutMin, xOutMax, yOutMin, yOutMax; ///< Output pixel range
    pswarpMatchRange(&xOutMin, &yOutMin, &xOutMax, &yOutMax, input, output);

    // Check the range of the grid coordinates
#define CHECK_GRID_RANGE() { \
        xGridMin = PS_MIN(xGridMin, xGrid); \
        xGridMax = PS_MAX(xGridMax, xGrid); \
        yGridMin = PS_MIN(yGridMin, yGrid); \
        yGridMax = PS_MAX(yGridMax, yGrid); \
    }

    int xGridMin = nGridX, xGridMax = 0, yGridMin = nGridY, yGridMax = 0; ///< Grid range
    int xGrid, yGrid;                   ///< Grid coordinates
    pswarpMapGridSetGrid(grid, xOutMin, yOutMin, &xGrid, &yGrid);
    CHECK_GRID_RANGE();
    pswarpMapGridSetGrid(grid, xOutMin, yOutMax, &xGrid, &yGrid);
    CHECK_GRID_RANGE();
    pswarpMapGridSetGrid(grid, xOutMax, yOutMin, &xGrid, &yGrid);
    CHECK_GRID_RANGE();
    pswarpMapGridSetGrid(grid, xOutMax, yOutMax, &xGrid, &yGrid);
    CHECK_GRID_RANGE();

    if (!pmReadoutMaskNonfinite(input, pmConfigMaskGet("SAT", config))) {
        psError(psErrorCodeLast(), false, "Unable to mask non-finite pixels in input.");
        return false;
    }

    // Interpolation options : move these from the arguments to explicit assignments
    psImageInterpolation *interp = psImageInterpolationAlloc(interpolationMode, input->image,
                                                             input->variance, input->mask, maskIn,
                                                             NAN, NAN, maskBad, maskPoor, poorFrac,
                                                             numKernels);

    if (input->variance && !output->variance) {
        output->variance = psImageAlloc(output->image->numCols, output->image->numRows, PS_TYPE_F32);
        psImageInit(output->variance, NAN);
    }
    if ((input->mask || maskPoor || maskBad) && !output->mask) {
        output->mask = psImageAlloc(output->image->numCols, output->image->numRows, PS_TYPE_IMAGE_MASK);
        psImageInit(output->mask, maskBad);
    }

    // Ensure threading is off for the covariance calculation, since we are threading on a different level.
    psImageCovarianceSetThreads(false);

    psAssert (xGridMin >= 0, "xGridMin too small\n");
    psAssert (yGridMin >= 0, "yGridMin too small\n");
    psAssert (xGridMax < grid->nXpts, "xGridMax too big\n");
    psAssert (yGridMax < grid->nYpts, "yGridMax too big\n");

    fprintf (stderr, "warp %d,%d - %d,%d\n", xGridMin, yGridMin, xGridMax, yGridMax);

    // create jobs and supply them to the threads
    for (int gridY = yGridMin; gridY <= yGridMax; gridY++) {
        for (int gridX = xGridMin; gridX <= xGridMax; gridX++) {
            pswarpTransformTileArgs *args = pswarpTransformTileArgsAlloc();
            args->input = psMemIncrRefCounter(input);
            args->output = psMemIncrRefCounter(output);
            args->grid = psMemIncrRefCounter(grid);
            args->interp = psMemIncrRefCounter(interp);
            args->region = psMemIncrRefCounter(region);

            args->gridX = gridX;
            args->gridY = gridY;
            args->goodPixels = 0;

	    if (backgroundWarp) {
	      args->background_warping = true;
	      args->offset_x = psMetadataLookupF32(NULL,config->arguments,"BKG_WARP_XOFFSET");
	      args->offset_y = psMetadataLookupF32(NULL,config->arguments,"BKG_WARP_YOFFSET");
	    }
	    
            // allocate a job
            psThreadJob *job = psThreadJobAlloc ("PSWARP_TRANSFORM_TILE");
            psArrayAdd(job->args, 1, args);
            if (!psThreadJobAddPending(job)) {
                psError(psErrorCodeLast(), false, "Unable to warp image.");
                return false;
            }
            psFree(args);
        }
    }

    // wait for the threads to finish and manage results
    // wait here for the threaded jobs to finish
    if (!psThreadPoolWait (false, true)) {
        psError(psErrorCodeLast(), false, "Unable to interpolate image.");
        return false;
    }

    // each job records its own goodPixel values; sum them here
    // we have only supplied one type of job, so we can assume the types here
    psThreadJob *job = NULL;
    int xMin = output->image->numCols, xMax = 0, yMin = output->image->numRows, yMax = 0; // Bounds
    int goodPixels = psMetadataLookupS32(&mdok, output->analysis, PSWARP_ANALYSIS_GOODPIX); // Number of pixels
    psList *covariances = psMetadataLookupPtr(&mdok, output->analysis,
                                              PSWARP_ANALYSIS_COVARIANCES); // Collection of covar. matrices
    if (!covariances) {
        covariances = psListAlloc(NULL);
        psMetadataAddList(output->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_COVARIANCES, 0,
                          "Collection of covariance matrices", covariances);
        psFree(covariances);            // Drop reference; still have the copy on the analysis metadata
    }
    double jacobian = psMetadataLookupF64(&mdok, output->analysis, PSWARP_ANALYSIS_JACOBIAN); // Jacobian
    if (!isfinite(jacobian)) {
        jacobian = 0.0;
    }

    while ((job = psThreadJobGetDone()) != NULL) {
        if (job->args->n < 1) {
            fprintf (stderr, "error with job\n");
        } else {
            pswarpTransformTileArgs *args = job->args->data[0];
            goodPixels += args->goodPixels;
            xMin = PS_MIN(args->xMin, xMin);
            xMax = PS_MAX(args->xMax, xMax);
            yMin = PS_MIN(args->yMin, yMin);
            yMax = PS_MAX(args->yMax, yMax);
            if (args->covariance) {
                psListAdd(covariances, PS_LIST_TAIL, args->covariance);
            }
            if (args->goodPixels > 0 && isfinite(args->jacobian)) {
                jacobian += args->jacobian * args->goodPixels;
            }

	    
        }
        psFree(job);
    }
    psFree(grid);

    psMetadataAddS32(output->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_GOODPIX, PS_META_REPLACE,
                     "Number of good pixels", goodPixels);
    psMetadataAddF64(output->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_JACOBIAN, PS_META_REPLACE,
                     "Jacobian of transformation", jacobian);

    if (xMin < xMax && yMin < yMax) {
        psTrace("pswarp.transform", 1, "Bounds [%d:%d,%d:%d]\n", xMin, xMax, yMin, yMax);
    } else {
        psTrace("pswarp.transform", 1, "No overlap\n");
    }
    psFree(interp);

    if (goodPixels > 0 && !backgroundWarp && psMetadataLookupBool(&mdok, recipe, "SOURCES")) {
	if (!pswarpTransformSources(output, input, config)) {
	    psError(psErrorCodeLast(), false, "Unable to transform sources.");
	    return false;
        }
    }

    if (goodPixels > 0) {
        // Data is only written out if there are good pixels
        output->data_exists = true;
        output->parent->data_exists = true;
        output->parent->parent->data_exists = true;
    }

    psLogMsg("pswarp", 3, "warping analysis: %f sec\n", psTimerMark ("warp"));

    return true;
}

bool pswarpMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
    )
{
    PS_ASSERT_METADATA_NON_NULL(source, false);

    // Ensure all the bad mask names exist, and set the value to catch all bad pixels
    psImageMaskType maskValue = 0;           // Value to mask to catch all the bad pixels
    psImageMaskType allMasks = 0;            // Value to mask to catch all masked bits (to set MARK)

    int nMasks = sizeof (warp_convolve_masks) / sizeof (pmConfigMaskInfo);

    for (int i = 0; i < nMasks; i++) {
        bool mdok;                      // Status of MD lookup
        psImageMaskType value = psMetadataLookupImageMaskFromGeneric(&mdok, source, warp_convolve_masks[i].badMaskName); // Value of mask
        if (!mdok) {
            psWarning ("problem with mask value %s\n", warp_convolve_masks[i].badMaskName);
        }

        if (!value) {
            if (warp_convolve_masks[i].fallbackName) {
                value = psMetadataLookupImageMaskFromGeneric(&mdok, source, warp_convolve_masks[i].fallbackName);
            }
            if (!value) {
                value = warp_convolve_masks[i].defaultMaskValue;
            }
            psMetadataAddImageMask(source, PS_LIST_TAIL, warp_convolve_masks[i].badMaskName, PS_META_REPLACE, NULL, value);
        }
        if (warp_convolve_masks[i].isBad) {
            maskValue |= value;
        }
        allMasks |= value;
    }

    // search for an unset bit to use for MARK:
    psImageMaskType markValue = 0x00;
    psImageMaskType markTrial = 0x01;

    int nBits = sizeof(psImageMaskType) * 8;
    for (int i = 0; !markValue && (i < nBits); i++) {
        if (allMasks & markTrial) {
            markTrial <<= 1;
        } else {
            markValue = markTrial;
        }
    }
    if (!markValue) {
        psError (PS_ERR_UNKNOWN, true, "Unable to define the MARK bit mask: all bits taken!");
        return false;
    }

    // update the list with the results
    psMetadataAddImageMask(source, PS_LIST_TAIL, "MASK.VALUE", PS_META_REPLACE, NULL, maskValue);
    psMetadataAddImageMask(source, PS_LIST_TAIL, "MARK.VALUE", PS_META_REPLACE, NULL, markValue);

    if (outMaskValue) {
        *outMaskValue = maskValue;
    }
    if (outMarkValue) {
        *outMarkValue = markValue;
    }

    return true;
}

