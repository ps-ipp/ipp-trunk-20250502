#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include "psAbort.h"
#include "psAssert.h"
#include "psError.h"
#include "psTrace.h"

#include "psImage.h"
#include "psFits.h"
#include "psStats.h"
#include "psImageStats.h"
#include "psImageBackground.h"
#include "psImageStructManip.h"

#include "psMemory.h"

#include "psFitsScale.h"

// Remember:
// TRUE(i.e., value in memory) = BZERO + BSCALE * FITS(i.e., value on disk)


// EAM 20160328 : I think ROBUST_MEDIAN & ROBUST_STDEV are too frequently wrong to use.  
// SAMPLE_MEDIAN and SAMPLE_QUARTILE are probably safer in general.  possible downsize: 
// sample_median may be slow (does a sort)

//#define MEAN_STAT PS_STAT_ROBUST_MEDIAN // Statistic to use for mean
//#define STDEV_STAT PS_STAT_ROBUST_STDEV // Statistic to use for stdev

#define MEAN_STAT PS_STAT_SAMPLE_MEDIAN // Statistic to use for mean
#define STDEV_STAT PS_STAT_SAMPLE_QUARTILE // Statistic to use for stdev

#define DESPERATE_MEAN_STAT PS_STAT_SAMPLE_MEDIAN // Statistic to use for mean when deperate
#define DESPERATE_STDEV_STAT PS_STAT_SAMPLE_QUARTILE // Statistic to use for stdev when desperate


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Determine appropriate BSCALE and BZERO for an image, preserving dynamic range
static bool scaleRange(double *bscale, // Scaling, to return
                       double *bzero, // Zero point, to return
                       const psImage *image, // Image to scale
                       const psFitsOptions *options // FITS options
    )
{
    psAssert(bscale, "impossible");
    psAssert(bzero, "impossible");
    psAssert(image, "impossible");
    psAssert(options, "impossible");

    psTrace("psLib.fits", 3, "Scaling image to preserve dynamic range");

    double range = pow(2.0, options->bitpix) - 1.0; // Range of values for target BITPIX, reduced by the BLANK
    double min = INFINITY, max = -INFINITY; // Minimum and maximum values
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    // Determine the minimum and maximum values
#define SCALE_DETERMINE_CASE(IN, INTYPE) \
    case PS_TYPE_##INTYPE: { \
        for (int y = 0; y < numRows; y++) { \
            for (int x = 0; x < numCols; x++) { \
                ps##INTYPE value = (IN)->data.INTYPE[y][x]; /* Value of interest */ \
                if (isfinite(value)) { \
                    if (value < min) { \
                        min = value; \
                    } \
                    if (value > max) { \
                        max = value; \
                    } \
                } \
            } \
        } \
        break; \
    }

    switch (image->type.type) {
        SCALE_DETERMINE_CASE(image, F32);
        SCALE_DETERMINE_CASE(image, F64);
      default:
        psAbort("Should be unreachable.");
    }

    if (!isfinite(min) || !isfinite(max)) {
        psWarning("No valid values in image to derive BSCALE,BZERO");
        *bscale = 1.0;
        *bzero = 0.0;
        return false;
    }
    if (min == max) {
        *bscale = 1.0;
        *bzero = min;
    } else {
        *bscale = (max - min) / range ;
        *bzero = min + 0.5 * range * (*bscale);
    }

    return true;
}

    
// Determine appropriate BSCALE and BZERO for an image, mapping the standard deviation to the nominated number
// of bits
static bool scaleStdev(double *bscale, // Scaling, to return
                       double *bzero, // Zero point, to return
                       const psImage *image, // Image to scale
                       const psImage *mask, // Mask image
                       psImageMaskType maskVal, // Value to mask
                       const psFitsOptions *options // FITS options
    )
{
    psAssert(bscale, "impossible");
    psAssert(bzero, "impossible");
    psAssert(image, "impossible");
    psAssert(options, "impossible");

    psTrace("psLib.fits", 3, "Scaling image by statistics");

    // Measure the mean and stdev
    // psImageBackground automatically excludes pixels that are non-finite, so we don't need to bother about a
    // mask.
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psStats *stats = psStatsAlloc(MEAN_STAT | STDEV_STAT); // Statistics object
    double mean, stdev;                                    // Mean and standard deviation

    if (!psImageBackground(stats, NULL, image, mask, maskVal, rng)) {
        // It could be because the image is entirely masked, in which case we don't want to error
        bool good = false;              // Any good pixels?


// Find good pixels in an image, by image type
#define GOOD_PIXELS_CASE(TYPE) \
      case PS_TYPE_##TYPE: \
        for (int y = 0; y < image->numRows && !good; y++) { \
            for (int x = 0; x < image->numCols && !good; x++) { \
                if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) { \
                    continue; \
                } \
                if (!isfinite(image->data.TYPE[y][x])) { \
                    continue; \
                } \
                good = true; \
            } \
        } \
        break;

        switch (image->type.type) {
            GOOD_PIXELS_CASE(F32);
            GOOD_PIXELS_CASE(F64);
          default:
            psAbort("Unsupported case: %x", image->type.type);
        }

        if (!good) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL, "Image has no good pixels, setting BSCALE = 1, BZERO = 0");
            psErrorClear();
            *bscale = 1.0;
            *bzero = 0.0;
            psFree(rng);
            psFree(stats);
            return true;
        }

        // There are some good pixels in there somewhere; psImageBackground just didn't find them
        psLogMsg("psLib.fits", PS_LOG_DETAIL,
                 "Couldn't measure background statistics for image quantisation; retrying.");
        psErrorClear();
        // Retry using all the available pixels
        stats->nSubsample = image->numCols * image->numRows + 1;
        if (!psImageStats(stats, image, mask, maskVal)) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL,
                     "Couldn't measure background statistics for image quantisation (attempt 2); retrying.");
            psErrorClear();
            // Retry with desperate statistic
            stats->options = DESPERATE_MEAN_STAT | DESPERATE_STDEV_STAT;
            if (!psImageStats(stats, image, mask, maskVal)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to measure background statistics for image");
                psFree(rng);
                psFree(stats);
                return false;
            } else {
                // Desperate retry
                mean = psStatsGetValue(stats, DESPERATE_MEAN_STAT);
		if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
		  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ);
		}
		else { 
		  stdev = psStatsGetValue(stats, DESPERATE_STDEV_STAT);
		}
            }
        } else {
            // Retry with all available pixels
            mean = psStatsGetValue(stats, MEAN_STAT);
	    if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	      stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ);
	    }
	    else {
	      stdev = psStatsGetValue(stats, STDEV_STAT);
	    }
        }
    } else {
        // First attempt
        mean = psStatsGetValue(stats, MEAN_STAT);
	if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ);
	}
	else {
	  stdev = psStatsGetValue(stats, STDEV_STAT);
	}
    }
    psFree(rng);
    psFree(stats);
    if (!isfinite(mean) || !isfinite(stdev)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Mean (%f) or stdev (%f) is non-finite.", mean, stdev);
        return false;
    }

    psTrace("psLib.fits", 5, "Mean: %lf Stdev: %lf", mean, stdev);

    long range = 1L << options->stdevBits;  // Range of values to carry standard deviation
    *bscale = stdev / (double) range;

    psTrace("psLib.fits", 5, "Number of bits: %ld BSCALE: %lf", range, *bscale);

    double imageVal;                    // Value on image
    long diskVal;                       // Corresponding value on disk
    switch (options->scaling) {
      case PS_FITS_SCALE_STDEV_POSITIVE:
      case PS_FITS_SCALE_LOG_STDEV_POSITIVE:
      case PS_FITS_SCALE_ASINH_STDEV_POSITIVE:
        // Put (mean - N sigma) at the lowest possible value: predominantly positive images
        imageVal = mean - options->stdevNum * stdev;
        // diskVal = - (1 << (options->bitpix - 1));
        diskVal = - (1L << (options->bitpix - 1));
        break;
      case PS_FITS_SCALE_STDEV_NEGATIVE:
      case PS_FITS_SCALE_LOG_STDEV_NEGATIVE:
      case PS_FITS_SCALE_ASINH_STDEV_NEGATIVE:
        // Put (mean + N sigma) at the highest possible value: predominantly negative images
        imageVal = mean + options->stdevNum * stdev;
        diskVal = (1L << (options->bitpix - 1)) - 1;
        break;
      case PS_FITS_SCALE_STDEV_BOTH:
      case PS_FITS_SCALE_LOG_STDEV_BOTH:
      case PS_FITS_SCALE_ASINH_STDEV_BOTH:
        // Put mean right in the middle: images with an equal abundance of positive and negative values
        imageVal = mean;
        diskVal = 0;
        break;
      default:
        psAbort("Should never get here.");
    }

    *bzero = imageVal - *bscale * diskVal;

    psTrace("psLib.fits", 5, "Image %lf corresponds to disk %ld --> BZERO: %lf", imageVal, diskVal, *bzero);

    return true;
}


    

static bool logscaleStdev(double *bscale, // Scaling, to return
			  double *bzero, // Zero point, to return
			  double *boffset, // Log offset, to return
			  const psImage *image, // Image to scale
			  const psImage *mask, // Mask image
			  psImageMaskType maskVal, // Value to mask
			  const psFitsOptions *options // FITS options
    )
{
    psAssert(bscale, "impossible");
    psAssert(bzero, "impossible");
    psAssert(boffset, "impossible");
    psAssert(image, "impossible");
    psAssert(options, "impossible");

    psTrace("psLib.fits", 3, "Scaling image by logarithm statistics");
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psImage *copy;
    
    *boffset = 99e99;

    // Make a copy of the image to pass to get the scaling parameters

    switch (image->type.type) {
    case PS_TYPE_F32:
      copy = psImageCopy(NULL,image,PS_TYPE_F32);
      break;
    case PS_TYPE_F64:
      copy = psImageCopy(NULL,image,PS_TYPE_F64);
      break;
    default:
      psError(PS_ERR_UNKNOWN, true, "Target type is not a float: %d", image->type.type);
      return NULL;
      break;
    }
    
    // Determine the minimum value on this image.
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  psF32 value = image->data.F32[y][x];
	  if (isfinite(value)) {
	    if (value < *boffset) {
	      *boffset = value;
	    }
	  }
	}
      }
      break;
    case PS_TYPE_F64:
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  psF64 value = image->data.F64[y][x];
	  if (isfinite(value)) {
	    if (value < *boffset) {
	      *boffset = value;
	    }
	  }
	}
      }
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
    // We only need to offset images that go negative.
    if (*boffset > 0.0) {
      *boffset = 0.0;
    }
    // Write offset to header
    // How?
    //    psMetadataAddF32(header,PS_LIST_TAIL,"LOGZERO",0,"Flux offset subtracted before taking logarithm.",offset);
    // Take the logarithm of the image, applying the offset
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
/* 	  if (x == 2331 && y == 2843) { */
/* 	    fprintf(stderr,"psFS32: %d %d %g %g %g\n",x,y,offset,image->data.F32[y][x],log10(image->data.F32[y][x] - offset)); */
/* 	  } */
	  copy->data.F32[y][x] = (log10( image->data.F32[y][x] - *boffset));
	}
      }
      break;
    case PS_TYPE_F64:
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    //	    fprintf(stderr,"psFS64: %d %d %g %g %g\n",x,y,offset,image->data.F64[y][x],log10(image->data.F64[y][x] - offset));
	    copy->data.F64[y][x] = (log10( image->data.F64[y][x] - *boffset));
	  }
	}
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
      
    // Do regular scaling on the logarithm image
    if (!scaleStdev(bscale, bzero, copy, mask, maskVal, options)) {
      psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
      return false;
    }
    psFree(copy);
    return true;
}

static bool logscaleRange(double *bscale, // Scaling, to return
			  double *bzero, // Zero point, to return
			  double *boffset, // Log offset, to return
			  const psImage *image, // Image to scale
			  const psFitsOptions *options // FITS options
    )
{
    psAssert(bscale, "impossible");
    psAssert(bzero, "impossible");
    psAssert(boffset, "impossible");
    psAssert(image, "impossible");
    psAssert(options, "impossible");

    psTrace("psLib.fits", 3, "Scaling image by logarithm statistics");
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psImage *copy;
    
    *boffset = 99e99;

    // Make a copy of the image to pass to get the scaling parameters

    switch (image->type.type) {
    case PS_TYPE_F32:
      copy = psImageCopy(NULL,image,PS_TYPE_F32);
      break;
    case PS_TYPE_F64:
      copy = psImageCopy(NULL,image,PS_TYPE_F64);
      break;
    default:
      psError(PS_ERR_UNKNOWN, true, "Target type is not a float: %d", image->type.type);
      return NULL;
      break;
    }
    
    // Determine the minimum value on this image.
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  psF32 value = image->data.F32[y][x];
	  if (!isfinite(value)) {
	    if (value < *boffset) {
	      *boffset = value;
	    }
	  }
	}
      }
      break;
    case PS_TYPE_F64:
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  psF64 value = image->data.F64[y][x];
	  if (!isfinite(value)) {
	    if (value < *boffset) {
	      *boffset = value;
	    }
	  }
	}
      }
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
    // We only need to offset images that go negative.
    if (*boffset > 0.0) {
      *boffset = 0.0;
    }
    // Write offset to header
    // How?
    //    psMetadataAddF32(header,PS_LIST_TAIL,"LOGZERO",0,"Flux offset subtracted before taking logarithm.",offset);
    // Take the logarithm of the image, applying the offset
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  copy->data.F32[y][x] = (log10( image->data.F32[y][x] - *boffset));
	}
      }
      break;
    case PS_TYPE_F64:
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    copy->data.F64[y][x] = (log10( image->data.F64[y][x] - *boffset));
	  }
	}
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
      
    // Do regular scaling on the logarithm image
    if (!scaleRange(bscale, bzero, copy, options)) {
      psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
      return false;
    }
    psFree(copy);
    return true;
}

static bool asinhStdev(double *bscale, // Scaling, to return
		       double *bzero,  // Zero point, to return
		       double *boffset, // asinh flux zeropoint, to return
		       double *bsoften, // asinh softening parameter, to return
		       const psImage *image, // Image to scale
		       const psImage *mask,  // Mask image
		       psImageMaskType maskVal, // value to mask
		       const psFitsOptions *options // FITS options
		       )
{
  psAssert(bscale, "impossible");
  psAssert(bzero, "impossible");
  psAssert(boffset, "impossible");
  psAssert(bsoften, "impossible");
  psAssert(image, "impossible");
  psAssert(options, "impossible");

  psTrace("psLib.fits", 3, "Scaling image by asinh method");
  int numCols = image->numCols, numRows = image->numRows; // Size of image
  
    // Measure the mean and stdev
    // psImageBackground automatically excludes pixels that are non-finite, so we don't need to bother about a
    // mask.
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psStats *stats = psStatsAlloc(MEAN_STAT | STDEV_STAT); // Statistics object
    double mean, stdev;                                    // Mean and standard deviation
    if (!psImageBackground(stats, NULL, image, mask, maskVal, rng)) {
        // It could be because the image is entirely masked, in which case we don't want to error
        bool good = false;              // Any good pixels?


// Find good pixels in an image, by image type
#define GOOD_PIXELS_CASE(TYPE) \
      case PS_TYPE_##TYPE: \
        for (int y = 0; y < image->numRows && !good; y++) { \
            for (int x = 0; x < image->numCols && !good; x++) { \
                if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) { \
                    continue; \
                } \
                if (!isfinite(image->data.TYPE[y][x])) { \
                    continue; \
                } \
                good = true; \
            } \
        } \
        break;

        switch (image->type.type) {
            GOOD_PIXELS_CASE(F32);
            GOOD_PIXELS_CASE(F64);
          default:
            psAbort("Unsupported case: %x", image->type.type);
        }

        if (!good) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL, "Image has no good pixels, setting BSCALE = 1, BZERO = 0");
            psErrorClear();
            *bscale = 1.0;
            *bzero = 0.0;
	    *bsoften = NAN;
            psFree(rng);
            psFree(stats);
            return true;
        }

        // There are some good pixels in there somewhere; psImageBackground just didn't find them
        psLogMsg("psLib.fits", PS_LOG_DETAIL,
                 "Couldn't measure background statistics for image quantisation; retrying.");
        psErrorClear();
        // Retry using all the available pixels
        stats->nSubsample = image->numCols * image->numRows + 1;
        if (!psImageStats(stats, image, mask, maskVal)) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL,
                     "Couldn't measure background statistics for image quantisation (attempt 2); retrying.");
            psErrorClear();
            // Retry with desperate statistic
            stats->options = DESPERATE_MEAN_STAT | DESPERATE_STDEV_STAT;
            if (!psImageStats(stats, image, mask, maskVal)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to measure background statistics for image");
                psFree(rng);
                psFree(stats);
                return false;
            } else {
                // Desperate retry
                mean = psStatsGetValue(stats, DESPERATE_MEAN_STAT);
		if ((DESPERATE_STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(DESPERATE_STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
		  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
		}
		else {
		  
		  stdev = psStatsGetValue(stats, DESPERATE_STDEV_STAT);
		}
	    }	
        } else {
            // Retry with all available pixels
            mean = psStatsGetValue(stats, MEAN_STAT);
	    if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	      stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
	    }
	    else {
	      stdev = psStatsGetValue(stats, STDEV_STAT);
	    }
        }
    } else {
        // First attempt
        mean = psStatsGetValue(stats, MEAN_STAT);
	if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
	}
	else { 
	  stdev = psStatsGetValue(stats, STDEV_STAT);
	}
    }
    psFree(rng);
    psFree(stats);
    if (!isfinite(mean) || !isfinite(stdev)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Mean (%f) or stdev (%f) is non-finite.", mean, stdev);
        return false;
    }

    psImage *copy;

    switch (image->type.type) {
    case PS_TYPE_F32:
      copy = psImageCopy(NULL,image,PS_TYPE_F32);
      break;
    case PS_TYPE_F64:
      copy = psImageCopy(NULL,image,PS_TYPE_F64);
      break;
    default:
      psError(PS_ERR_UNKNOWN, true, "Target type is not a float: %d", image->type.type);
      return NULL;
      break;
    }
    // Do scaling
    float a = 1.0857362; // 2.5 * log(e);
    *bsoften = sqrt(a) * stdev;
    *boffset = mean;
    //    float m0 = 0; // Can I just arbitrarily set this?
    
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
/* 	  if (x == 266 && y == 4584) { */
/* 	    fprintf(stderr,"psFS32: %d %d %g %g %g %g %g\n",x,y,*boffset,*bsoften,image->data.F32[y][x],log10(image->data.F32[y][x] - *boffset), */
/* 		    a * asinh((image->data.F32[y][x] - *boffset) / (2 * *bsoften))); */
/* 	  } */
	  if (isfinite(image->data.F32[y][x])) {
	    copy->data.F32[y][x] = a * asinh( (image->data.F32[y][x] - *boffset) / (2 * *bsoften));// - 2.5 * log10(b) + m0;
	  }
	  else {
	    copy->data.F32[y][x] = image->data.F32[y][x];
	  }
	}
      }
      break;
    case PS_TYPE_F64:
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    //	    fprintf(stderr,"psFS64: %d %d %g %g %g\n",x,y,offset,image->data.F64[y][x],log10(image->data.F64[y][x] - offset));
	    if (isfinite(image->data.F64[y][x])) {
	      copy->data.F64[y][x] = a * asinh( (image->data.F64[y][x] - *boffset) / (2 * *bsoften));// - 2.5 * log10(b) + m0;
	    }
	    else {
	      copy->data.F64[y][x] = image->data.F64[y][x];
	    }	  
	  }
	}
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
      
    // Do regular scaling on the logarithm image
    if (!scaleStdev(bscale, bzero, copy, mask, maskVal, options)) {
      psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
      return false;
    }
    psFree(copy);
    return true;
}

static bool asinhRange(double *bscale, // Scaling, to return
		       double *bzero,  // Zero point, to return
		       double *boffset, // asinh flux zeropoint, to return
		       double *bsoften, // asinh softening parameter, to return
		       const psImage *image, // Image to scale
		       const psImage *mask,  // Mask image
		       psImageMaskType maskVal, // value to mask
		       const psFitsOptions *options // FITS options
		       )
{
  psAssert(bscale, "impossible");
  psAssert(bzero, "impossible");
  psAssert(bsoften, "impossible");
  psAssert(image, "impossible");
  psAssert(options, "impossible");

  psTrace("psLib.fits", 3, "Scaling image by asinh method");
  int numCols = image->numCols, numRows = image->numRows; // Size of image
  
    // Measure the mean and stdev
    // psImageBackground automatically excludes pixels that are non-finite, so we don't need to bother about a
    // mask.
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psStats *stats = psStatsAlloc(MEAN_STAT | STDEV_STAT); // Statistics object
    double mean, stdev;                                    // Mean and standard deviation
    if (!psImageBackground(stats, NULL, image, mask, maskVal, rng)) {
        // It could be because the image is entirely masked, in which case we don't want to error
        bool good = false;              // Any good pixels?


// Find good pixels in an image, by image type
#define GOOD_PIXELS_CASE(TYPE) \
      case PS_TYPE_##TYPE: \
        for (int y = 0; y < image->numRows && !good; y++) { \
            for (int x = 0; x < image->numCols && !good; x++) { \
                if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) { \
                    continue; \
                } \
                if (!isfinite(image->data.TYPE[y][x])) { \
                    continue; \
                } \
                good = true; \
            } \
        } \
        break;

        switch (image->type.type) {
            GOOD_PIXELS_CASE(F32);
            GOOD_PIXELS_CASE(F64);
          default:
            psAbort("Unsupported case: %x", image->type.type);
        }

        if (!good) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL, "Image has no good pixels, setting BSCALE = 1, BZERO = 0");
            psErrorClear();
            *bscale = 1.0;
            *bzero = 0.0;
	    *bsoften = NAN;
            psFree(rng);
            psFree(stats);
            return true;
        }

        // There are some good pixels in there somewhere; psImageBackground just didn't find them
        psLogMsg("psLib.fits", PS_LOG_DETAIL,
                 "Couldn't measure background statistics for image quantisation; retrying.");
        psErrorClear();
        // Retry using all the available pixels
        stats->nSubsample = image->numCols * image->numRows + 1;
        if (!psImageStats(stats, image, mask, maskVal)) {
            psLogMsg("psLib.fits", PS_LOG_DETAIL,
                     "Couldn't measure background statistics for image quantisation (attempt 2); retrying.");
            psErrorClear();
            // Retry with desperate statistic
            stats->options = DESPERATE_MEAN_STAT | DESPERATE_STDEV_STAT;
            if (!psImageStats(stats, image, mask, maskVal)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to measure background statistics for image");
                psFree(rng);
                psFree(stats);
                return false;
            } else {
                // Desperate retry
                mean = psStatsGetValue(stats, DESPERATE_MEAN_STAT);
		if ((DESPERATE_STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(DESPERATE_STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
		  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
		}
		else { 
		  stdev = psStatsGetValue(stats, DESPERATE_STDEV_STAT);
		}
            }
        } else {
            // Retry with all available pixels
            mean = psStatsGetValue(stats, MEAN_STAT);
	    if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	      stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
	    }
	    else {
	      stdev = psStatsGetValue(stats, STDEV_STAT);
	    }
        }
    } else {
        // First attempt
        mean = psStatsGetValue(stats, MEAN_STAT);
	if ((STDEV_STAT == PS_STAT_SAMPLE_QUARTILE)||(STDEV_STAT == PS_STAT_ROBUST_QUARTILE)) {
	  stdev = 0.7415 * (stats->sampleUQ - stats->sampleLQ); // Calculate a "sigma" based on the interquartile distance.
	}
	else {
	  stdev = psStatsGetValue(stats, STDEV_STAT);
	}
    }
    psFree(rng);
    psFree(stats);
    if (!isfinite(mean) || !isfinite(stdev)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Mean (%f) or stdev (%f) is non-finite.", mean, stdev);
        return false;
    }

    psImage *copy;

    switch (image->type.type) {
    case PS_TYPE_F32:
      copy = psImageCopy(NULL,image,PS_TYPE_F32);
      break;
    case PS_TYPE_F64:
      copy = psImageCopy(NULL,image,PS_TYPE_F64);
      break;
    default:
      psError(PS_ERR_UNKNOWN, true, "Target type is not a float: %d", image->type.type);
      return NULL;
      break;
    }
    // Do scaling
    float a = 1.0857362; // 2.5 * log(e);
    *bsoften = sqrt(a) * stdev;
    *boffset = mean;
    // float m0 = 0; // Can I just arbitrarily set this?
    
    switch (image->type.type) {
    case PS_TYPE_F32: 
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
/* 	  if (x == 2331 && y == 2843) { */
/* 	    fprintf(stderr,"psFS32: %d %d %g %g %g\n",x,y,offset,image->data.F32[y][x],log10(image->data.F32[y][x] - offset)); */
/* 	  } */
/* 	  if (x == 266 && y == 4584) { */
/* 	    fprintf(stderr,"psFS32: %d %d %g %g %g %g %g\n",x,y,*boffset,*bsoften,image->data.F32[y][x],log10(image->data.F32[y][x] - *boffset), */
/* 		    a * asinh((image->data.F32[y][x] - *boffset) / (2 * *bsoften))); */
/* 	  } */

	  if (isfinite(image->data.F32[y][x])) {
	    copy->data.F32[y][x] = a * asinh( (image->data.F32[y][x] - *boffset) / (2 * *bsoften));// - 2.5 * log10(b) + m0;
	  }
	  else {
	    copy->data.F32[y][x] = image->data.F32[y][x];
	  }
	}
      }
      break;
    case PS_TYPE_F64:
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    //	    fprintf(stderr,"psFS64: %d %d %g %g %g\n",x,y,offset,image->data.F64[y][x],log10(image->data.F64[y][x] - offset));
	    if (isfinite(image->data.F64[y][x])) {
	      copy->data.F64[y][x] = a * asinh( (image->data.F64[y][x] - *boffset)/ (2 * *bsoften));// - 2.5 * log10(b) + m0;
	    }
	    else {
	      copy->data.F64[y][x] = image->data.F32[y][x];
	    }
	  }
	}
      break;
    default:
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target type is not a float: %d",image->type.type);
      return NULL;
      break;
    }
      
    // Do regular scaling on the asinh image
    if (!scaleRange(bscale, bzero, copy, options)) {
      psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
      return false;
    }
    psFree(copy);
    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool psFitsScaleDetermine(double *bscale, double *bzero, double *boffset, double *bsoften,
			  long *blank, const psImage *image,
                          const psImage *mask, psImageMaskType maskVal, const psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(bscale, false);
    PS_ASSERT_PTR_NON_NULL(bzero, false);
    PS_ASSERT_PTR_NON_NULL(boffset, false);
    PS_ASSERT_PTR_NON_NULL(blank, false);
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    if (mask) {
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, image, false);
    }
    PS_ASSERT_FITS_NON_NULL(fits, false);

    *bscale = NAN;
    *bzero = NAN;
    *boffset = 0;
    *blank = 0;

    psFitsOptions *options = fits->options; // FITS options
    if (!PS_IS_PSELEMTYPE_REAL(image->type.type) || !options) {
        return true;
    }

    switch (options->bitpix) {
      case 0:
        // No scaling applied
        return true;
      case 8:
      case 16:
      case 32:
      case 64:
        // Nothing to do; just allowing these values to pass through
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target bitpix (%d) is not one of 0,8,16,32,64",
                options->bitpix);
        return false;
    }

    *blank = (1L << (options->bitpix - 1)) - 1;

    switch (options->scaling) {
      case PS_FITS_SCALE_NONE:
        // No scaling applied
        break;
      case PS_FITS_SCALE_RANGE:
        if (!scaleRange(bscale, bzero, image, options)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from range");
            return false;
        }
        break;
      case PS_FITS_SCALE_STDEV_POSITIVE:
      case PS_FITS_SCALE_STDEV_NEGATIVE:
      case PS_FITS_SCALE_STDEV_BOTH:
	if (!scaleStdev(bscale, bzero, image, mask, maskVal, options)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
            return false;
	}
        break;
      case PS_FITS_SCALE_LOG_RANGE:
	if (!logscaleRange(bscale,bzero,boffset,image,options)) {
	  psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from range");
	  return false;
	}
	break;
      case PS_FITS_SCALE_LOG_STDEV_POSITIVE:
      case PS_FITS_SCALE_LOG_STDEV_NEGATIVE:
      case PS_FITS_SCALE_LOG_STDEV_BOTH:
	if (!logscaleStdev(bscale, bzero,boffset, image, mask, maskVal, options)) {
	  psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
	  return false;
        }
        break;
      case PS_FITS_SCALE_ASINH_RANGE:
	if (!asinhRange(bscale,bzero,boffset,bsoften,image,mask,maskVal,options)) {
	  psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from range");
	  return false;
	}
	break;
      case PS_FITS_SCALE_ASINH_STDEV_POSITIVE:
      case PS_FITS_SCALE_ASINH_STDEV_NEGATIVE:
      case PS_FITS_SCALE_ASINH_STDEV_BOTH:
	if (!asinhStdev(bscale, bzero,boffset,bsoften, image, mask, maskVal, options)) {
	  psError(PS_ERR_UNKNOWN, false, "Unable to set BSCALE and BZERO from stdev");
	  return false;
        }
        break;
	
	
      case PS_FITS_SCALE_MANUAL:
        *bscale = options->bscale;
        *bzero = options->bzero;
        break;
      case PS_FITS_SCALE_LOG_MANUAL:
        *bscale = options->bscale;
        *bzero = options->bzero;
	*boffset = options->boffset;
        break;
      case PS_FITS_SCALE_ASINH_MANUAL:
        *bscale = options->bscale;
        *bzero = options->bzero;
	*boffset = options->boffset;
	*bsoften = options->bsoften;
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised FITS scaling method: %x",
                options->scaling);
        return false;
    }

    if (options->bitpix == 8) {
        // FITS standard wants unsigned for BITPIX=8, two's-complement for BITPIX=16,32,64.
        *bzero -= *bscale * 128;
        *blank = 255;
    }


    psTrace("psLib.fits", 3, "BSCALE = %.10lf, BZERO = %.10lf, BOFFSET = %.10lf, BSOFTEN = %.10lf, BLANK = %ld\n",
	    *bscale, *bzero, *boffset, *bsoften, *blank);
    return true;
}


psImage *psFitsScaleForDisk(const psImage *image, const psFits *fits, double bscale, double bzero, double boffset, double bsoften, psRandom *rng)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    // fprintf (stderr, "psFitsScaleForDisk : bscale  : %g\n", bscale);
    // fprintf (stderr, "psFitsScaleForDisk : bzero   : %g\n", bzero);
    // fprintf (stderr, "psFitsScaleForDisk : boffset : %g\n", boffset);
    // fprintf (stderr, "psFitsScaleForDisk : bsoften : %g\n", bsoften);

    psFitsOptions *options = fits->options; // FITS options
    if (!PS_IS_PSELEMTYPE_REAL(image->type.type) || !options || options->bitpix == 0) {
        // No scaling desired
        return psMemIncrRefCounter((psImage*)image); // Casting away "const"
    }

    int bitpix = options->bitpix;       // Bits per pixel
    psElemType outType;                 // Type for output image

    // fprintf (stderr, "psFitsScaleForDisk : bitpix : %d\n", bitpix);

    // Choosing to use signed types because those don't require BSCALE,BZERO to represent them in the FITS
    // file
    switch (bitpix) {
      case 8:
        // Note: Use unsigned integer for BITPIX=8
        outType = PS_TYPE_U8;
        break;
      case 16:
        outType = PS_TYPE_S16;
        break;
      case 32:
        outType = PS_TYPE_S32;
        break;
      case 64:
        outType = PS_TYPE_S64;
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target bitpix (%d) is not one of 8,16,32,64", bitpix);
        return NULL;
    }

    if (bscale == 1.0 && bzero == 0.0 && !options->fuzz) {
        return psImageCopy(NULL, image, outType);
    }

    int numCols = image->numCols, numRows = image->numRows; // Size of image
    psImage *out = psImageAlloc(numCols, numRows, outType); // Output image

    // fprintf (stderr, "corruption check psFitsScaleForDisk 1\n");
    // psMemCheckCorruption (stderr, true);

    if (options->fuzz && !psMemIncrRefCounter (rng)) {
      // Don't blab about which seed we're going to get --- it's not necessary for this purpose
      rng = psRandomAlloc(PS_RANDOM_TAUS);
    }

#define SCALE_WRITE_OUT_CASE(IN, INTYPE, OUT, OUTTYPE)			\
    case PS_TYPE_##OUTTYPE: {						\
        double scale = 1.0 / bscale;					\
        double zero = bzero;						\
        /* Note: BITPIX=8 treated differently, since it uses unsigned values; the rest use signed */ \
        double min = bitpix == 8 ? 0 : -pow(2.0, options->bitpix - 1);	\
        double max = bitpix == 8 ? 255 : (pow(2.0, options->bitpix - 1) - 1.0); \
        for (int y = 0; y < numRows; y++) {				\
            for (int x = 0; x < numCols; x++) {				\
		ps##INTYPE value;					\
		if ((options->scaling == PS_FITS_SCALE_LOG_RANGE)||	\
		    (options->scaling == PS_FITS_SCALE_LOG_MANUAL)||	\
		    (options->scaling == PS_FITS_SCALE_LOG_STDEV_POSITIVE)|| \
		    (options->scaling == PS_FITS_SCALE_LOG_STDEV_NEGATIVE)|| \
		    (options->scaling == PS_FITS_SCALE_LOG_STDEV_BOTH)) { \
		    if (isfinite((IN)->data.INTYPE[y][x])) {		\
			value = log10( (IN)->data.INTYPE[y][x] - boffset ); \
		    }							\
		    else {						\
			value = (IN)->data.INTYPE[y][x];		\
		    }							\
		}							\
		else if ((options->scaling == PS_FITS_SCALE_ASINH_RANGE)|| \
			 (options->scaling == PS_FITS_SCALE_ASINH_MANUAL)|| \
			 (options->scaling == PS_FITS_SCALE_ASINH_STDEV_POSITIVE)|| \
			 (options->scaling == PS_FITS_SCALE_ASINH_STDEV_NEGATIVE)|| \
			 (options->scaling == PS_FITS_SCALE_ASINH_STDEV_BOTH)) { \
		    if (isfinite((IN)->data.INTYPE[y][x])) {		\
			value = 1.0857362 * (asinh( ((IN)->data.INTYPE[y][x] - boffset) / (2.0 * bsoften))); \
		    }							\
		    else {						\
			value = (IN)->data.INTYPE[y][x];		\
		    }							\
		}							\
		else {							\
		    value = (IN)->data.INTYPE[y][x];			\
		}							\
		if (!isfinite(value)) {					\
		    /* This choice of "max" for non-finite pixels is mainly cosmetic --- it has to be */ \
		    /* something, and "min" would produce holes in the cores of bright stars. */ \
		    (OUT)->data.OUTTYPE[y][x] = max;			\
		} else {						\
		    value = (value - zero) * scale;			\
		    if (options->fuzz && (value - (int)value != 0.0)) {	\
			/* Add random factor [0.0,1.0): adds a variance of 1/12, */ \
			/* but preserves the expectation value given the floor() */ \
			value += psRandomUniform(rng) ;			\
		    }							\
		    /* Check for underflow and overflow; set either to max */ \
		    (OUT)->data.OUTTYPE[y][x] = (value < min || value > max ? max : floor(value)); \
		    psAssert(x >= 0, "oops");				\
		    psAssert(y >= 0, "oops");				\
		    psAssert(x < numCols, "oops");			\
		    psAssert(y < numRows, "oops");			\
		}							\
            }								\
        }								\
        break;								\
    }
    
#define SCALE_WRITE_IN_CASE(IN, INTYPE, OUT)			\
    case PS_TYPE_##INTYPE: {					\
	switch (outType) {					\
	    SCALE_WRITE_OUT_CASE(IN, INTYPE, OUT, U8);;		\
	    SCALE_WRITE_OUT_CASE(IN, INTYPE, OUT, S16);;	\
	    SCALE_WRITE_OUT_CASE(IN, INTYPE, OUT, S32);;	\
	    SCALE_WRITE_OUT_CASE(IN, INTYPE, OUT, S64);;	\
	  default:						\
	    psAbort("Should be unreachable.");			\
	}							\
	break;							\
    }
    
    switch (image->type.type) {
	SCALE_WRITE_IN_CASE(image, F32, out);
	SCALE_WRITE_IN_CASE(image, F64, out); 
      default:
        psAbort("Should be unreachable.");
    }

    psFree(rng);
    return out;
}



// This function to apply BSCALE and BZERO to an image read immediately from disk should not be necessary at
// the present time, since cfitsio should apply the scaling itself in the process of reading.  However, we may
// later desire it (e.g., if we ever make our own FITS implementation).
psImage *psFitsScaleFromDisk(const psImage *image, double boffset, double bsoften)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);

    psElemType inType = image->type.type; // Type for input image
    psElemType outType;                 // Type for output image
    switch (inType) {
      case PS_TYPE_S8:
      case PS_TYPE_S16:
      case PS_TYPE_S32:
      case PS_TYPE_U8:
      case PS_TYPE_U16:
        outType = PS_TYPE_F32;
        break;
      case PS_TYPE_S64:
      case PS_TYPE_U32:
      case PS_TYPE_U64:
        outType = PS_TYPE_F64;
        break;
        // Including floating-point types just in case someone wants to apply a BSCALE and BZERO to them.
      case PS_TYPE_F32:
        outType = PS_TYPE_F32;
        break;
      case PS_TYPE_F64:
        outType = PS_TYPE_F64;
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unsupported image type: %x", inType);
        return NULL;
    }

    int numCols = image->numCols, numRows = image->numRows;
    psImage *out = psImageAlloc(numCols, numRows, outType); // Output scaled image


#define SCALE_READ_OUT_CASE(INTYPE, OUTTYPE) \
  case PS_TYPE_##OUTTYPE: { \
      for (int y = 0; y < numRows; y++) { \
          for (int x = 0; x < numCols; x++) { \
	    if (bsoften) {						\
	      out->data.OUTTYPE[y][x] = 2 * bsoften * sinh(image->data.INTYPE[y][x] / (1.0857362)) + boffset; \
            }								\
	    else if (boffset) {					\
	      out->data.OUTTYPE[y][x] = pow(10,image->data.INTYPE[y][x]) + boffset;; \
             }									\
          } \
      } \
      break; \
  }

#define SCALE_READ_IN_CASE(INTYPE) \
  case PS_TYPE_##INTYPE: { \
      switch (outType) { \
          SCALE_READ_OUT_CASE(INTYPE, F32); \
          SCALE_READ_OUT_CASE(INTYPE, F64); \
        default: \
          psAbort("Should never get here: type %x should be F32 or F64", outType); \
      } \
      break; \
  }

    switch (inType) {
        SCALE_READ_IN_CASE(S8);
        SCALE_READ_IN_CASE(S16);
        SCALE_READ_IN_CASE(S32);
        SCALE_READ_IN_CASE(S64);
        SCALE_READ_IN_CASE(U8);
        SCALE_READ_IN_CASE(U16);
        SCALE_READ_IN_CASE(U32);
        SCALE_READ_IN_CASE(U64);
        SCALE_READ_IN_CASE(F32);
        SCALE_READ_IN_CASE(F64);
      default:
          psAbort("Should never get here: type %x should be integer", inType);
    }

    return out;
}

psFitsScaling psFitsScalingFromString(const char *string)
{
    PS_ASSERT_STRING_NON_EMPTY(string, PS_FITS_SCALE_NONE);

    if (strcasecmp(string, "RANGE") == 0)          return PS_FITS_SCALE_RANGE;
    if (strcasecmp(string, "STDEV_POSITIVE") == 0) return PS_FITS_SCALE_STDEV_POSITIVE;
    if (strcasecmp(string, "STDEV_NEGATIVE") == 0) return PS_FITS_SCALE_STDEV_NEGATIVE;
    if (strcasecmp(string, "STDEV_BOTH") == 0)     return PS_FITS_SCALE_STDEV_BOTH;
    if (strcasecmp(string, "LOG_RANGE") == 0)      return PS_FITS_SCALE_LOG_RANGE;
    if (strcasecmp(string, "LOG_MANUAL") == 0)      return PS_FITS_SCALE_LOG_MANUAL;
    if (strcasecmp(string, "LOG_STDEV_POSITIVE") == 0) return PS_FITS_SCALE_LOG_STDEV_POSITIVE;
    if (strcasecmp(string, "LOG_STDEV_NEGATIVE") == 0) return PS_FITS_SCALE_LOG_STDEV_NEGATIVE;
    if (strcasecmp(string, "LOG_STDEV_BOTH") == 0) return PS_FITS_SCALE_LOG_STDEV_BOTH;
    if (strcasecmp(string, "ASINH_RANGE") == 0)      return PS_FITS_SCALE_ASINH_RANGE;
    if (strcasecmp(string, "ASINH_MANUAL") == 0)      return PS_FITS_SCALE_ASINH_MANUAL;
    if (strcasecmp(string, "ASINH_STDEV_POSITIVE") == 0) return PS_FITS_SCALE_ASINH_STDEV_POSITIVE;
    if (strcasecmp(string, "ASINH_STDEV_NEGATIVE") == 0) return PS_FITS_SCALE_ASINH_STDEV_NEGATIVE;
    if (strcasecmp(string, "ASINH_STDEV_BOTH") == 0) return PS_FITS_SCALE_ASINH_STDEV_BOTH;
    if (strcasecmp(string, "MANUAL") == 0)         return PS_FITS_SCALE_MANUAL;

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to interpret FITS scaling: %s", string);
    return PS_FITS_SCALE_NONE;
}


