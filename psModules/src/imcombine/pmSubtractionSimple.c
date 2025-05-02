#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionParams.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionEquation.h"
#include "pmSubtractionAnalysis.h"
#include "pmSubtractionMask.h"
#include "pmSubtractionThreads.h"
#include "pmSubtractionVisual.h"
#include "pmErrorCodes.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"

#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"

#include "pmSourceLensing.h"
#include "pmSource.h"

#include "pmSubtractionSimple.h"


bool simple_do_boxphot(int *nPix,
		       float *flux,
		       pmSource *source,
		       psImage *image,
		       psImage *mask,
		       psImageMaskType maskVal,
		       int size) {
  *nPix = 0;
  *flux = 0.0;
  for (int y = source->peak->yf - size;y <= source->peak->yf + size;y++) {
    for (int x = source->peak->xf - size; x <= source->peak->xf + size; x++) {
      if ((y > 0)&&(y < image->numRows)&&
	  (x > 0)&&(x < image->numCols)) {
	if (!(mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) {
	  *nPix += 1;
	  *flux += image->data.F32[y][x];
	}
      }
    }
  }
  *flux = log10(*flux);
  return(true);
}  

bool simple_apply_mask(psImage *image, psImage *weight, psImage *mask,
		       psImageMaskType maskVal) {
  for (int y = 0; y < mask->numRows; y++) {
    for (int x = 0; x < mask->numCols; x++) {
      if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
	image->data.F32[y][x] = NAN;
	if (weight) {
	  weight->data.F32[y][x] = NAN;
	}
      }
    }
  }
  return(true);
}
		       

// Copied from pmSubtraction
static void solvedKernelPreCalc(psKernel *kernel, // Kernel, updated
				const pmSubtractionKernels *kernels, // Kernel basis functions
				float value,                         // Normalisation value for basis function
				int index                  // Index of basis function of interest
				)
{
  int size = kernels->size;           // Kernel half-size
  pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[index]; // Precalculated values
  for (int v = -size; v <= size; v++) {
    for (int u = -size; u <= size; u++) {
      kernel->kernel[v][u] +=  value * preCalc->kernel->kernel[v][u];
    }
  }

  return;
}
//End copy

bool pmSubtractionSimpleMatch(pmReadout *conv1,
			      pmReadout *conv2,
			      const pmReadout *ro1,
			      const pmReadout *ro2,
			      const psArray *sources,
			      int size,
			      psImageMaskType maskVal,
			      psImageMaskType maskBad,
			      psImageMaskType maskPoor,
			      float deconvolveThreshold
			      ) {
  //
  // We've already validated the input values at this level
  float sig2fwhm = 2.0 * sqrt(2.0 * log(2.0));
  float fwhm1 = 0,fwhm2 = 0;
  float sigma1 = 0,sigma2 = 0,sigmaKern = 0;
  float chisq = 1.0;
  int convolution_direction = 0;
  psImage *image1 = NULL;
  psImage *mask1 = NULL;
  psImage *var1 = NULL;

  psImage *image2 = NULL;
  psImage *mask2 = NULL;
  psImage *var2 = NULL;

  psImage *imageC1 = NULL;
  psImage *maskC1 = NULL;
  psImage *varC1 = NULL;

  psImage *imageC2 = NULL;
  psImage *maskC2 = NULL;
  psImage *varC2 = NULL;

  psImage *maskTemp = NULL;
  
  // Allocate images, as this is usually done by subtractionMatchAlloc after this function is called.  
  int numCols = ro1->image->numCols;
  int numRows = ro1->image->numRows;

  if (conv1) {
    //    conv1->covariance = psMemIncrRefCounter(ro1->covariance);
    if (!conv1->image) {
      conv1->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    }
    psImageInit(conv1->image, NAN);
    if (ro1->variance) {
      if (!conv1->variance) {
	conv1->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
      }
      psImageInit(conv1->variance, NAN);
    }
    if (!conv1->mask) {
      conv1->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }
    psImageInit(conv1->mask, maskBad);
  }
  if (conv2) {
    //    
    if (!conv2->image) {
      conv2->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    }
    psImageInit(conv2->image, NAN);
    if (ro2->variance) {
      if (!conv2->variance) {
	conv2->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
      }
      psImageInit(conv2->variance, NAN);
    }
    if (!conv2->mask) {
      conv2->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }
    psImageInit(conv2->mask, maskBad);
  }
  
  // Assign local aliases to images
  image1 = ro1->image;
  mask1  = ro1->mask;
  var1   = ro1->variance;

  image2 = ro2->image;
  mask2  = ro2->mask;
  var2   = ro2->variance;

  if (conv1) {
    imageC1 = conv1->image;
    maskC1  = conv1->mask;
    varC1   = conv1->variance;
  }
  
  if (conv2) {
    imageC2 = conv2->image;
    maskC2  = conv2->mask;
    varC2   = conv2->variance;
  }
 
  //
  // Determine Gaussian widths
  pmSubtractionGetFWHMs(&fwhm1,&fwhm2);
  sigma1 = fwhm1 / sig2fwhm;
  sigma2 = fwhm2 / sig2fwhm;
  if (sigma1 > sigma2) {
    convolution_direction = 2;
    sigmaKern = sqrt(PS_SQR(sigma1) - PS_SQR(sigma2));
  }
  else if (sigma1 < sigma2) {
    convolution_direction = 1;
    sigmaKern = sqrt(PS_SQR(sigma2) - PS_SQR(sigma1));
  }
  if (!conv1) {
    if (convolution_direction == 1) {
      if (sigma1 - sigma2 > deconvolveThreshold) {
	chisq = 100;
      }
    }
    //    convolution_direction = 2;
  }
  if (!conv2) {
    if (convolution_direction == 2) {
      if (sigma2 - sigma1 > deconvolveThreshold) {
	chisq = 100;
      }
    }
    //    convolution_direction = 1;
  }
  
  bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve
  
  int maskBox = (int) ceil(sigmaKern * 1.1774); // diameter is 1/2 FWHM
  int maskBlank = 8;  // I should be able to get this from a reference, right?
  int maskPoorVal = 16384; // Another value that should be found elsewhere.
  //
  // Make a fake pmSubtractionKernels element so we can add it appropriately.
  // Defining everything here is a bit clunky, but it's necessary to get the covariance
  // correct.
  psVector *fwhms = psVectorAlloc(1,PS_TYPE_F32);
  fwhms->data.F32[0] = sigmaKern * sig2fwhm;
  psVector *orders = psVectorAlloc(1,PS_TYPE_S32);
  orders->data.S32[0] = 0;
  psRegion bounds;
  bounds.x0 = 0;
  bounds.y0 = 0;
  bounds.x1 = numCols;
  bounds.y1 = numRows;
  pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(1,PM_SUBTRACTION_KERNEL_SIMPLE,
							    size,fwhms,orders,0,0.0,bounds,
							    convolution_direction);
  pmSubtractionKernelsMakeDescription(kernels); // Need this defined
  pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_SIMPLE,0,0,size,sigmaKern);
  kernels->widths->data.F32[0] = sigmaKern * sig2fwhm;
  kernels->u->data.S32[0] = 0;
  kernels->v->data.S32[0] = 0;
  if (kernels->preCalc->data[0]) {
    psFree(kernels->preCalc->data[0]);
  }
  kernels->preCalc->data[0] = psMemIncrRefCounter(preCalc);
  kernels->solution1 = psVectorAlloc(3,PS_TYPE_F64);
  kernels->solution1->data.F32[0] = 1.0;
  kernels->solution1->data.F32[1] = 0.0;
  kernels->solution1->data.F32[2] = 0.0;
  kernels->solution1err = psVectorAlloc(3,PS_TYPE_F32);
  kernels->solution1err->data.F32[0] = 0.0;
  kernels->solution1err->data.F32[1] = 0.0;
  kernels->solution1err->data.F32[2] = 0.0;
  kernels->mean = 0.0;
  kernels->rms = chisq; // This is the chi^2 value that's passed to ppStack
  kernels->numStamps = sources->n;
  
  psKernel *kernel = psKernelAlloc(-size,size,-size,size);
  solvedKernelPreCalc(kernel,kernels,1.0,0);
  
  //
  // Do convolutions
  if (convolution_direction == 1) {
    if (conv1) {
      psImageSmoothMask_Threaded(imageC1,image1,mask1,maskVal,sigmaKern,6,1e-6);
      psImageSmoothMask_Threaded(varC1,var1,mask1,maskVal,sigmaKern * M_SQRT1_2,6,1e-6);

      maskTemp = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
      maskTemp = psImageConvolveMask(maskTemp,mask1,maskVal,maskBad,      // Mask bad values
				   -maskBox,maskBox,-maskBox,maskBox);
      maskC1 = psImageConvolveMask(maskC1,maskTemp,maskPoorVal,maskPoor,  // Mask poor values
				   -maskBox,maskBox,-maskBox,maskBox);
      psFree(maskTemp);

      conv1->covariance = psImageCovarianceCalculate(kernel,ro1->covariance);
      pmSubtractionBorder(imageC1,varC1,maskC1,maskBox,maskBlank);
      simple_apply_mask(imageC1,varC1,maskC1,maskBad);
    }
    if (conv2) {
      imageC2 = psImageCopy(imageC2,image2,PS_TYPE_F32);
      varC2   = psImageCopy(varC2,var2,PS_TYPE_F32);
      maskC2  = psImageCopy(maskC2,mask2,PS_TYPE_IMAGE_MASK);
      conv2->covariance = psMemIncrRefCounter(ro2->covariance);
    }
  }
  else if (convolution_direction == 2) {
    if (conv2) {
      psImageSmoothMask_Threaded(imageC2,image2,mask2,maskVal,sigmaKern,6,1e-6);
      psImageSmoothMask_Threaded(varC2,var2,mask2,maskVal,sigmaKern * M_SQRT1_2,6,1e-6);

      maskTemp = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
      maskTemp = psImageConvolveMask(maskTemp,mask2,maskVal,maskBad,      // Mask bad values
				   -maskBox,maskBox,-maskBox,maskBox);
      maskC2 = psImageConvolveMask(maskC2,maskTemp,maskPoorVal,maskPoor,  // Mask poor values
				   -maskBox,maskBox,-maskBox,maskBox);
      psFree(maskTemp);

      conv2->covariance = psImageCovarianceCalculate(kernel,ro2->covariance);
      pmSubtractionBorder(imageC2,varC2,maskC2,maskBox,maskBlank);
      simple_apply_mask(imageC2,varC2,maskC2,maskBad);
    }
    if (conv1) {
      imageC1 = psImageCopy(imageC1,image1,PS_TYPE_F32);
      varC1   = psImageCopy(varC1,var1,PS_TYPE_F32);
      maskC1  = psImageCopy(maskC1,mask1,PS_TYPE_IMAGE_MASK);
      conv1->covariance = psMemIncrRefCounter(ro1->covariance);
    }
  }    

  psFree(kernel); // No longer needed after doing covariance calculation

  //
  // Do normalization
  float normalization = 1.0;

  // Scan source list, do box photometry on peaks, and then solve the linear relation.
  int photRadius = (int) floor(PS_MAX(sigma1,sigma2) * 2.0 * sqrt(2.0 * log(2.0))); // Go out a FWHM diameter from the center.
  psVector *logFluxDifferences = psVectorAlloc(sources->n,PS_TYPE_F32);
  psVector *fitMask = psVectorAlloc(sources->n,PS_TYPE_VECTOR_MASK);
  for (int i = 0; i < sources->n; i++) {
    pmSource *source = sources->data[i];
    int nPix1,nPix2;
    float flux1,flux2;

    if (conv1) {
      simple_do_boxphot(&nPix1,&flux1,source,imageC1,maskC1,maskBad,photRadius);
    }
    else {
      simple_do_boxphot(&nPix1,&flux1,source,image1,mask1,maskBad,photRadius);
    }

    if (conv2) {
      simple_do_boxphot(&nPix2,&flux2,source,imageC2,maskC2,maskBad,photRadius);
    }
    else {
      simple_do_boxphot(&nPix2,&flux2,source,image2,mask2,maskBad,photRadius);
    }

    logFluxDifferences->data.F32[i] = flux2 - flux1;
    fitMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0;
    if ((PS_MIN(nPix1,nPix2) <= 0.75 * PS_MAX(nPix1,nPix2))||
	(!isfinite(flux1))||(!isfinite(flux2))) {
      fitMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
    }

    //    fprintf(stderr,"SOURCES: %d %g %g %g -> %d %d %g %g %d %g\n",i,source->peak->xf,source->peak->yf,source->psfMag,
    //	    nPix1,nPix2,flux1,flux2,fitMask->data.PS_TYPE_VECTOR_MASK_DATA[i],logFluxDifferences->data.F32[i]);
  }

  // Given the differences in log-flux space, the normalization factor is just the exponential of the median difference
  psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
  if (!psVectorStats(stats,logFluxDifferences,NULL,fitMask,0xff)) {
    // This should complain.
    normalization = 1.0;
  }

  normalization = pow(10,stats->robustMedian);
  // fprintf(stderr,"NORM: %g+/-%g\n",stats->robustMedian,stats->robustStdev);
  
  psFree(stats);
  psFree(logFluxDifferences);
  psFree(fitMask);

  // Apply normalization
  if (normalization != 1.0) {
    if ((conv1)&&((convolution_direction == 1))) {
      psBinaryOp(imageC1,imageC1,"*",psScalarAlloc((float) normalization, PS_TYPE_F32));
      psBinaryOp(varC1,varC1,"*",psScalarAlloc((float) PS_SQR(normalization), PS_TYPE_F32));
    }
    else if ((conv2)&&(convolution_direction == 2)) {
      normalization = 1.0 / normalization; // Because we fit one way, but are using it in the other.
      psBinaryOp(imageC2,imageC2,"*",psScalarAlloc((float) normalization, PS_TYPE_F32));
      psBinaryOp(varC2,varC2,"*",psScalarAlloc((float) PS_SQR(normalization), PS_TYPE_F32));
    }
  }
  

  //
  
  //
  // Actually add it to the headers
  psMetadata *outAnalysis = psMetadataAlloc();
  psMetadata *outHeader   = psMetadataAlloc();
  if (!pmSubtractionAnalysis(outAnalysis,outHeader,kernels,NULL,numCols,numRows)) {
    psError(psErrorCodeLast(),false,"Unable to generate QA data");
    psFree(fwhms);
    psFree(orders);
    psFree(preCalc);
    psFree(kernels);
  }
  // This is a hack.  Yes, I know.  pmSAnalysis doesn't get the normalization correct, so I just do it here.
  psMetadataRemoveKey(outAnalysis,PM_SUBTRACTION_ANALYSIS_NORM);
  psMetadataRemoveKey(outHeader,PM_SUBTRACTION_ANALYSIS_NORM);
  psMetadataAddF32(outAnalysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_NORM, PS_META_REPLACE, "Normalisation", normalization);
  psMetadataAddF32(outHeader,   PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_NORM, PS_META_REPLACE, "Normalisation", normalization);

  psFree(fwhms);
  psFree(orders);
  psFree(preCalc);
  psFree(kernels);

  if (conv1) {
    conv1->analysis = psMetadataCopy(conv1->analysis, outAnalysis);
    conv1->data_exists = true;
    if (conv1->parent) {
      conv1->parent->data_exists = true;
      
      if (conv1->parent->parent) {
	conv1->parent->parent->data_exists = true;
      }
    }
  }
  if (conv2) {
    conv2->analysis = psMetadataCopy(conv2->analysis, outAnalysis);
    conv2->data_exists = true;
    if (conv2->parent) {
      conv2->parent->data_exists = true;
      if (conv2->parent->parent) {
	conv2->parent->parent->data_exists = true;
      }
    }
  }

  if (conv1 && conv1->parent) {
    pmHDU *hdu = pmHDUFromCell(conv1->parent);
    if (hdu) {
      hdu->header = psMetadataCopy(hdu->header, outHeader);
    }
  }
  if (conv2 && conv2->parent) {
    pmHDU *hdu = pmHDUFromCell(conv2->parent);
    if (hdu) {
      hdu->header = psMetadataCopy(hdu->header, outHeader);
    }
  }
  psFree(outAnalysis);
  psFree(outHeader);

  psImageConvolveSetThreads(oldThreads);
  
  //
  // Return
  return(true);
}
