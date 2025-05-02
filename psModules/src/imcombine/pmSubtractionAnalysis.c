#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionEquation.h"

#include "pmSubtractionAnalysis.h"
#include "pmSubtractionVisual.h"

//#define TESTING

// save information about the kernel in the output header.  this function also generates the
// image normalization, used by ppSubMatchPSFs.c to rescale the output image.
bool pmSubtractionAnalysis(psMetadata *analysis, psMetadata *header,
                           pmSubtractionKernels *kernels, psRegion *region,
                           int numCols, int numRows)
{
    PS_ASSERT_METADATA_NON_NULL(analysis, false);
    PS_ASSERT_METADATA_NON_NULL(header, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, false);

    // Record region for subtraction
    {
        psRegion *subRegion;    // Region over which subtraction was performed
        if (region) {
            subRegion = psMemIncrRefCounter(region);
        } else {
            subRegion = psRegionAlloc(0, numCols, 0, numRows);
        }

        psMetadataAddPtr(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_REGION, PS_DATA_REGION | PS_META_DUPLICATE_OK, "Region over which subtraction was performed", subRegion);

        psString string = psRegionToString(*subRegion);
        psFree(subRegion);

        psMetadataAddStr(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_REGION, PS_META_DUPLICATE_OK, "Region over which subtraction was performed", string);
        psFree(string);
    }

    // Record kernel
    psMetadataAddPtr(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_KERNEL, PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK, "Subtraction kernels", kernels);
    psMetadataAddS32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MODE, PS_META_DUPLICATE_OK, "Subtraction mode", kernels->mode);
    psMetadataAddS32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MODE, PS_META_DUPLICATE_OK, "Subtraction mode", kernels->mode);

    // Realisations of kernel
    psImage *convKernels = pmSubtractionKernelsImageMosaic(kernels);
    psMetadataAddImage(analysis, PS_LIST_TAIL, "SUBTRACTION.KERNEL.IMAGE", PS_META_DUPLICATE_OK, "Realisations of kernel", convKernels);
    psFree(convKernels);

    // sample difference images
    {
        psMetadataAddArray(analysis, PS_LIST_TAIL, "SUBTRACTION.SAMPLE.STAMP.SET", PS_META_DUPLICATE_OK, "Sample Difference Stamps", kernels->sampleStamps);
    }

#ifdef TESTING
    // Generate images of the kernel components
    {
        psMetadata *header = psMetadataAlloc(); // Header
        for (int i = 0; i < kernels->solution1->n; i++) {
            psString name = NULL;       // Header keyword
            psStringAppend(&name, "SOLN%04d", i);
            psMetadataAddF64(header, PS_LIST_TAIL, name, 0, NULL, kernels->solution1->data.F64[i]);
            psFree(name);
        }
        psArray *kernelImages = pmSubtractionKernelSolutions(kernels, 0.0, 0.0, false);
        psFits *kernelFile = psFitsOpen("kernels1.fits", "w");
        (void)psFitsWriteImageCube(kernelFile, header, kernelImages, NULL);
        psFitsClose(kernelFile);
        psFree(kernelImages);
        psFree(header);
    }
    if (kernels->solution2) {
        psMetadata *header = psMetadataAlloc(); // Header
        for (int i = 0; i < kernels->solution2->n; i++) {
            psString name = NULL;       // Header keyword
            psStringAppend(&name, "SOLN%04d", i);
            psMetadataAddF64(header, PS_LIST_TAIL, name, 0, NULL, kernels->solution2->data.F64[i]);
            psFree(name);
        }
        psArray *kernelImages = pmSubtractionKernelSolutions(kernels, 0.0, 0.0, true);
        psFits *kernelFile = psFitsOpen("kernels2.fits", "w");
        (void)psFitsWriteImageCube(kernelFile, header, kernelImages, NULL);
        psFitsClose(kernelFile);
        psFree(kernelImages);
        psFree(header);
    }
#endif


    // Kernel shape
    {
        psImage *image = pmSubtractionKernelImage(kernels, 0.5, 0.5, false); // Image of the kernel
        if (!image) {
            psError(psErrorCodeLast(), false, "Unable to generate image of kernel.");
            return false;
        }
        int size = kernels->size;       // Half-size of kernel
        int fullSize = 2 * size + 1;    // Full size of kernel

        float norm = 0.0;               // Normalisation (kernel sum)
        for (int y = 0; y < fullSize; y++) {
            for (int x = 0; x < fullSize; x++) {
                norm += image->data.F32[y][x];
            }
        }
	psLogMsg("psModules.imcombine", PS_LOG_INFO, "Kernel Integral: %f", norm);

        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_NORM, PS_META_DUPLICATE_OK, "Normalisation", norm);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_NORM, PS_META_DUPLICATE_OK, "Normalisation", norm);

        float maxCon =  0.0;          // Maximum +fraction > 0.0
        float maxDec =  0.0;          // Maximum -fraction < 0.0
        for (int r = 1; r < size; r++) {
            unsigned long r2 = PS_SQR(r); // r^2
            float sum = 0.0;            // Sum within circle
            for (int y = 0, v = -size; y < fullSize; y++, v++) {
                unsigned long v2 = PS_SQR(v); // y^2
                for (int x = 0, u = -size; x < fullSize; x++, u++) {
                    unsigned long u2 = PS_SQR(u); // u^2
                    if (u2 + v2 <= r2) {
                        sum += image->data.F32[y][x];
                    }
                }
            }
            float frac = sum / norm;    // Fraction of flux moving towards centre
            psTrace("psModules.imcombine", 5, "(De)Convolution fraction at %d: %f\n", r, frac);
            maxCon = PS_MAX(maxCon, +frac);
            maxDec = PS_MAX(maxDec, -frac);
        }
        psFree(image);

        {
            psMetadataItem *item = NULL;
	    item = psMetadataLookup(analysis, PM_SUBTRACTION_ANALYSIS_DECONV_MAX); // Previous
            if (item) {
                maxDec = item->data.F32 = PS_MAX(item->data.F32, maxDec);
            } else {
                psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DECONV_MAX, 0, "Maximum deconvolution fraction", maxDec);
            }
            item = psMetadataLookup(header, PM_SUBTRACTION_ANALYSIS_DECONV_MAX); // Previous
            if (item) {
                item->data.F32 = maxDec;
            } else {
                psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DECONV_MAX, 0, "Maximum deconvolution fraction", maxDec);
            }
        }
        {
            psMetadataItem *item = NULL;
	    item = psMetadataLookup(analysis, PM_SUBTRACTION_ANALYSIS_CONVOL_MAX); // Previous
            if (item) {
                maxCon = item->data.F32 = PS_MAX(item->data.F32, maxCon);
            } else {
                psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_CONVOL_MAX, 0, "Maximum convolution fraction", maxCon);
            }
            item = psMetadataLookup(header, PM_SUBTRACTION_ANALYSIS_CONVOL_MAX); // Previous
            if (item) {
                item->data.F32 = maxCon;
            } else {
                psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_CONVOL_MAX, 0, "Maximum convolution fraction", maxCon);
            }
        }
    }

    // Kernel moments : since the kernel can be negative, calculate the absolute flux moments,
    // eg \Sum(x|f|) / Sum(|f|), \Sum(x^2|f|) / Sum(|f|)
    {
        psImage *image = pmSubtractionKernelImage(kernels, 0.5, 0.5, false); // Image of the kernel
        if (!image) {
            psError(psErrorCodeLast(), false, "Unable to generate image of kernel.");
            return false;
        }
        double m00 = 0, m10 = 0, m01 = 0, m20 = 0, m11 = 0, m02 = 0; // Moments to calculate
        int size = kernels->size;       // Half-size of kernel
        int fullSize = 2 * size + 1;    // Full size of kernel
        for (int y = 0, v = -size; y < fullSize; y++, v++) {
            for (int x = 0, u = -size; x < fullSize; x++, u++) {
                float value = fabs(image->data.F32[y][x]); // Value of kernel
                m00 += value;
                m10 += u * value;
                m01 += v * value;
                m20 += u * u * value;
                m11 += u * v * value;
                m02 += v * v * value;
            }
        }
        psFree(image);

        // Convert first moments to centroids
        m10 /= m00;
        m01 /= m00;

        // Convert second moments to covariance
        m20 = m20 / m00 - PS_SQR(m10);
        m02 = m02 / m00 - PS_SQR(m01);
        m11 = m11 / m00 - m10 * m01;

        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MX,
                         PS_META_DUPLICATE_OK, "Moment in x", m10);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MY,
                         PS_META_DUPLICATE_OK, "Moment in y", m01);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MXX,
                         PS_META_DUPLICATE_OK, "Moment in xx", m20);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MXY,
                         PS_META_DUPLICATE_OK, "Moment in xy", m11);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MYY,
                         PS_META_DUPLICATE_OK, "Moment in yy", m02);

        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MX,
                         PS_META_DUPLICATE_OK, "Moment in x", m10);
        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MY,
                         PS_META_DUPLICATE_OK, "Moment in y", m01);
        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MXX,
                         PS_META_DUPLICATE_OK, "Moment in xx", m20);
        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MXY,
                         PS_META_DUPLICATE_OK, "Moment in xy", m11);
        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_MYY,
                         PS_META_DUPLICATE_OK, "Moment in yy", m02);
    }

    // Difference in background
    {
        psImage *polyValues = p_pmSubtractionPolynomial(NULL, kernels->spatialOrder, 0.0, 0.0); // Polynomial
        float bg = p_pmSubtractionSolutionBackground(kernels, polyValues); // Background difference

        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_BGDIFF,
                         PS_META_DUPLICATE_OK, "Background difference", bg);
        psMetadataAddF32(header, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_BGDIFF,
                         PS_META_DUPLICATE_OK, "Background difference", bg);
        psFree(polyValues);
    }

    // Quality of fit
    {
        psMetadataAddS32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_STAMPS,   PS_META_REPLACE, "Number of stamps", kernels->numStamps);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DEV_MEAN, PS_META_REPLACE, "Mean stamp deviation", kernels->mean);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DEV_RMS,  PS_META_REPLACE, "RMS stamp deviation", kernels->rms);
        psMetadataAddS32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_STAMPS,   PS_META_REPLACE, "Number of stamps", kernels->numStamps);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DEV_MEAN, PS_META_REPLACE, "Mean stamp deviation", kernels->mean);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_DEV_RMS,  PS_META_REPLACE, "RMS stamp deviation", kernels->rms);

        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_MEAN,  PS_META_REPLACE, "Fractional Sigma of Residuals (Mean)", kernels->fResSigmaMean);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_STDEV, PS_META_REPLACE, "Fractional Sigma of Residuals (Stdev)", kernels->fResSigmaStdev);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_OUTER_MEAN,  PS_META_REPLACE, "Fractional Residual Flux (Mean, R > 2 pix)", kernels->fResOuterMean);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_OUTER_STDEV, PS_META_REPLACE, "Fractional Residual Flux (Stdev, R > 2 pix)", kernels->fResOuterStdev);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_MEAN,  PS_META_REPLACE, "Fractional Residual Flux (Mean, R > 0 pix)", kernels->fResTotalMean);
        psMetadataAddF32(analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_STDEV, PS_META_REPLACE, "Fractional Residual Flux (Stdev, R > 0 pix)", kernels->fResTotalStdev);

        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_MEAN,  PS_META_REPLACE, "Fractional Sigma of Residuals (Mean)", kernels->fResSigmaMean);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_STDEV, PS_META_REPLACE, "Fractional Sigma of Residuals (Stdev)", kernels->fResSigmaStdev);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_OUTER_MEAN,  PS_META_REPLACE, "Fractional Residual Flux (Mean, R > 2 pix)", kernels->fResOuterMean);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_OUTER_STDEV, PS_META_REPLACE, "Fractional Residual Flux (Stdev, R > 2 pix)", kernels->fResOuterStdev);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_MEAN,  PS_META_REPLACE, "Fractional Residual Flux (Mean, R > 0 pix)", kernels->fResTotalMean);
        psMetadataAddF32(header,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_STDEV, PS_META_REPLACE, "Fractional Residual Flux (Stdev, R > 0 pix)", kernels->fResTotalStdev);
    }

    return true;
}
