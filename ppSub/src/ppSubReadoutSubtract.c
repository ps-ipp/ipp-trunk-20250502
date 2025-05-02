/** @file ppSubReadoutSubtract.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

bool ppSubReadoutSubtract(pmConfig *config)
{
    psAssert(config, "Require configuration");

    // Look up recipe values
    bool mdok = false;                  // Status of MD lookup
    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSUB_RECIPE); // Recipe for ppSub
    psAssert(recipe, "We checked this earlier, so it should be here.");

    bool reverse = psMetadataLookupBool(&mdok, config->arguments, "REVERSE"); // Reverse sense of subtraction?
    bool noConvolve = psMetadataLookupBool(&mdok, recipe, "NOCONVOLVE"); // Do not use convolved images.
    bool addPair = psMetadataLookupBool(&mdok, recipe, "ADD.NOT.SUBTRACT"); // add instead of subtracting
    bool doApplyMaskNaN = psMetadataLookupBool(&mdok, recipe, "APPLY.PIXELNAN"); // NaN the pixels underneath masks

    pmFPAview *view = ppSubViewReadout(); // View to readout

    // Subtraction is: minuend - subtrahend
    pmReadout *minuend = NULL;          // Positive image
    pmReadout *subtrahend = NULL;       // Negative image
    if (!noConvolve) {
      printf("Using Convolved images because NOCONVOLVE is FALSE\n");
      if (reverse) {
        minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV");
        subtrahend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV");
      } else {
        minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV");
        subtrahend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV");
      }
    }
    else {
      psWarning("Not using Convolved images because NOCONVOLVE  is TRUE\n");
      if (reverse) {
        minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF");
        subtrahend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT");
      } else {
        minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT");
        subtrahend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF");
      }
    }

    // Do the actual subtraction
    pmReadout *outRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");

    if (addPair) {
	outRO->image = (psImage*)psBinaryOp(outRO->image, minuend->image, "+", subtrahend->image);
    } else {
	outRO->image = (psImage*)psBinaryOp(outRO->image, minuend->image, "-", subtrahend->image);
    }
    outRO->mask = (psImage*)psBinaryOp(outRO->mask, minuend->mask, "|", subtrahend->mask);
    outRO->variance = (psImage*)psBinaryOp(outRO->variance, minuend->variance, "+", subtrahend->variance);

    // NAN the masked pixels in the diff image (pixels masked in A are not yet NAN'ed in B)
    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config) | pmConfigMaskGet("BLANK", config); // Bits to mask in inputs
    if(doApplyMaskNaN) {
      for (int iy = 0; iy < outRO->image->numRows; iy++) {
	for (int ix = 0; ix < outRO->image->numCols; ix++) {
	    if ((outRO->mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) == 0) continue;
	    outRO->image->data.F32[iy][ix] = NAN;
	}
      }
    }

    // Measure the variance scales
    psStats *varStats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for variance images
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);           // Random number generator
    psImageBackground(varStats, NULL, minuend->variance, minuend->mask, maskVal, rng);
    float minuendVar = varStats->robustMedian; // Mean variance for minuend
    psImageBackground(varStats, NULL, subtrahend->variance, subtrahend->mask, maskVal, rng);
    float subtrahendVar = varStats->robustMedian; // Mean variance for subtrahend
    psFree(varStats);
    psFree(rng);

    // XXX EAM : I suspect that the weighted averaging is giving the wrong answer for
    // single-direction PSF matching (and maybe DUAL as well).  I am testing the difference by
    // generating A+B images instead of A-B images and then checking the significance of the
    // sources in the image

    // Combine the covariances
    // These are weighted by the appropriate mean variance.  This is probably not perfectly correct, but it
    // does seem to reproduce the correct magnitude limit in psphot.
    psArray *covars = psArrayAlloc(2);  // Covariance pseudo-matrices
    psVector *covarWeights = psVectorAlloc(2, PS_TYPE_F32); // Weights for covariances
    covars->data[0] = psMemIncrRefCounter(minuend->covariance);
    covars->data[1] = psMemIncrRefCounter(subtrahend->covariance);
    covarWeights->data.F32[0] = minuendVar;
    covarWeights->data.F32[1] = subtrahendVar;
# if (0)    
    outRO->covariance = psImageCovarianceAverageWeighted(covars, covarWeights);
# else
    outRO->covariance = psImageCovarianceAverage(covars);
# endif
    psFree(covars);
    psFree(covarWeights);

    psImageCovarianceTransfer(outRO->variance, outRO->covariance);

    outRO->data_exists = true;
    outRO->parent->data_exists = true;
    outRO->parent->parent->data_exists = true;

    pmSubtractionVisualShowSubtraction(minuend->image, subtrahend->image, outRO->image);

    // Copy concepts from the input to the output
    pmFPAfile *inFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.INPUT"); // Input file
    pmFPA *inFPA = inFile->fpa;         // Input FPA
    pmFPAfile *outFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.OUTPUT"); // Output file
    pmFPA *outFPA = outFile->fpa;       // Output FPA
    if (!pmConceptsCopyFPA(outFPA, inFPA, true, true)) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to copy concepts from input to output.");
        psFree(outRO);
        psFree(view);
        return false;
    }

    // Copy astrometry over
    // It should find its way into the output images and photometry
    pmHDU *inHDU = inFPA->hdu;          // Input HDU
    pmHDU *outHDU = outFPA->hdu;        // Output HDU
    pmChip *outChip = pmFPAfileThisChip(config->files, view, "PPSUB.OUTPUT"); // Output chip
    psFree(view);

    if (!outHDU || !inHDU) {
        psError(PPSUB_ERR_PROG, true, "Unable to find HDU at FPA level to copy astrometry.");
        return false;
    }
    if (!pmAstromReadWCS(outFPA, outChip, inHDU->header, 1.0)) {
        psError(psErrorCodeLast(), false, "Unable to read WCS astrometry from input FPA.");
        return false;
    }
    if (!pmAstromWriteWCS(outHDU->header, outFPA, outChip, WCS_TOLERANCE)) {
        psError(psErrorCodeLast(), false, "Unable to write WCS astrometry to output FPA.");
        return false;
    }

    return true;
}
