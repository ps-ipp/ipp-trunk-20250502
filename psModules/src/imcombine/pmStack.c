/** @file  pmStack.c
 *
 *  This file will perform image combination of several images of the
 *  same field, produce a list of questionable pixels, then tag some
 *  of those pixels as cosmic rays.
 *
 *  @author Paul Price, IfA
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.48.2.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-19 17:59:50 $
 *  Copyright 2004-2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h> // for memset
#include <pslib.h>

#include <gsl/gsl_cdf.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmReadoutStack.h"
#include "pmConceptsAverage.h"

#include "pmStack.h"

#define PIXEL_LIST_BUFFER 100           // Number of entries to add to pixel list at a time
#define PIXEL_MAP_BUFFER 2              // Number of entries to add to pixel map at a time
//#define ADD_VARIANCE                    // Allow additional variance (besides variance factor)?
#define NUM_DIRECT_STDEV 5              // For less than this number of values, measure stdev directly


# if (0)
#define TESTING                         Enable test output
/* #define TEST_X 5745                       // x coordinate to examine */
/* #define TEST_Y 5331                       // y coordinate to examine */
// #define TEST_X 972
// #define TEST_Y 3213
//#define TEST_X 3289
//#define TEST_Y 4810
//#define TEST_RADIUS 0.5                 // Radius to examine
//MEH -- streak-like junk md04s065i
#define TEST_X 1129 
#define TEST_Y 4256
#define TEST_RADIUS 2.0                 // Radius to examine
# endif

# ifdef TESTING
# define CHECKPIX(XPIX,YPIX,MSG,...) { if (PS_SQR(XPIX - TEST_X) + PS_SQR(YPIX - TEST_Y) <= PS_SQR(TEST_RADIUS)) { fprintf(stderr,MSG,__VA_ARGS__); } }
# else
# define CHECKPIX(XPIX,YPIX,MSG,...) { }
# endif    

// Data structure for use as a buffer when combining pixels
// Use of this structure means we don't have to do an allocation in the combination function for each pixel
typedef struct {
    psVector *pixels;                   // Pixel values
    psVector *variances;                // Pixel variances
    psVector *weights;                  // Pixel weightings
    psVector *exps;                     // Pixel exposures
    psVector *sources;                  // Pixel sources (which image did they come from?)
    psVector *limits;                   // Rejection limits
    psVector *suspects;                 // Pixel is suspect?
    psVector *sort;                     // Buffer for sorting (to get a robust estimator of the standard dev)
} combineBuffer;

static void combineBufferFree(combineBuffer *buffer)
{
    psFree(buffer->pixels);
    psFree(buffer->variances);
    psFree(buffer->weights);
    psFree(buffer->exps);
    psFree(buffer->sources);
    psFree(buffer->limits);
    psFree(buffer->suspects);
    psFree(buffer->sort);
    return;
}

static combineBuffer *combineBufferAlloc(long numImages // Number of images that will be combined
    )
{
    combineBuffer *buffer = psAlloc(sizeof(combineBuffer));
    psMemSetDeallocator(buffer, (psFreeFunc)combineBufferFree);

    buffer->pixels = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->variances = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->weights = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->exps = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->sources = psVectorAlloc(numImages, PS_TYPE_U16);
    buffer->limits = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->suspects = psVectorAlloc(numImages, PS_TYPE_U8);
    buffer->sort = psVectorAlloc(numImages, PS_TYPE_F32);
    return buffer;
}


// Deallocator for the stack data
static void stackDataFree(pmStackData *data)
{
    psFree(data->readout);
    psFree(data->reject);
    psFree(data->inspect);
    return;
}

// KMM functions to do bimodality rejection of pixels
double gaussian(float x, float m, float s) {
    return(pow(s * sqrt(2 * M_PI),-1) * exp(-0.5 * pow( (x - m) / s, 2)));
}

static void KMMcalculate(const psVector *values,
			 float *Punimodal,int *iter,
			 float *mU, float *sU,
			 float *pi1, float *m1, float *s1,
			 float *pi2, float *m2, float *s2,
                         int xyrdebug) {
    assert(values);
    assert(values->type.type == PS_TYPE_F32);
  
    double logL_bimodal = 0, logL_unimodal;
    psVector *P1 = psVectorAlloc(values->n,PS_TYPE_F32);
    psVector *P2 = psVectorAlloc(values->n,PS_TYPE_F32);
    int i;
    int discrepant_index = -1;
    double discrepant_value = 0;
/*   int debug = 0; */
  
    // Calculate unimodal properties
    *mU = 0;
    *sU = 0;
    logL_unimodal = 0;
    for (i = 0; i < values->n; i++) { // Calculate mean
	*mU += values->data.F32[i];
    }
    *mU /= values->n;
    for (i = 0; i < values->n; i++) { // Calculate sigma
	*sU += pow(values->data.F32[i] - *mU,2);

	// Attempt to guess better starting values
	if (pow(values->data.F32[i] - *mU,2) > discrepant_value) {
	    discrepant_value = pow(values->data.F32[i] - *mU,2);
	    discrepant_index = i;
	}
    
    }
    *sU = sqrt(*sU / values->n);
    for (i = 0; i < values->n; i++) { // Calculate log likelihood
	logL_unimodal += log(gaussian(values->data.F32[i],*mU,*sU));
    }

    if (xyrdebug == 1) { 
	fprintf(stderr,"KMM uni: %d %f %d (%f %f)\n", 
		xyrdebug,logL_unimodal,discrepant_index, 
		*mU,*sU); 
    } 

    // Do EM loop
    float dL = 0;
    float oldL = -999;
    *iter = 0;
    logL_bimodal = logL_unimodal;

    if (discrepant_index == -1) {
	*m1 = *mU - 3 * *sU;
	*m2 = *mU + 3 * *sU;
	*s1 = *sU / 2;
	*s2 = *sU / 2;
    }
    else {
	// This is an attempt to speed up convergence. Find the largest contributor to sigma, and set one mean
	// to that value.  Set the other mean to the mean of all other points with this one removed.  Next,
	// set the sigmas to be equal to each other.  Take the value of sigma to be such that a point equidistant
	// to the initial values of the two modes is equally not believed by either mode (2.5 sigma away).
    
	discrepant_value = values->data.F32[discrepant_index];
    
	if (discrepant_value >  *mU) {
	    *m1 = ((*mU * values->n) - discrepant_value) / (values->n - 1);
	    *m2 = discrepant_value;
	}
	else {
	    *m1 = discrepant_value;
	    *m2 = ((*mU * values->n) - discrepant_value) / (values->n - 1);
	}
	*s1 = fabs((*m1 - *m2) / 5);
	*s2 = *s1;
    }
    
    *pi1 = 0.5;
    *pi2 = 0.5;

    //MEH -- need to be double to help avoid 0 in norm
    double g1,g2,norm;
    float w1,w2;

    // These should be options.
    float KMM_TOLERANCE = 1e-6;
    int KMM_MAX_ITERATIONS = 30;
    float KMM_SMALL_NUMBER = 1e-5;
    while (((dL > KMM_TOLERANCE)||(*iter < 3))&&(*iter < KMM_MAX_ITERATIONS)) {
	*iter += 1;
	dL = fabs(logL_bimodal - oldL);
	oldL = logL_bimodal;
/*     if (debug == 1) { */
/*       fprintf(stderr,"KMM: %d %f %f %f %f (%f %f %f) (%f %f %f)\n", */
/* 	      *iter,logL_unimodal,logL_bimodal,oldL,dL, */
/* 	      *m1,*s1,*pi1, */
/* 	      *m2,*s2,*pi2); */
/*     } */

	if (xyrdebug == 1) { 
	    fprintf(stderr,"KMM EM iter: %d %f %f %f %f (%f %f %e) (%f %f %e)\n", 
		    *iter,logL_unimodal,logL_bimodal,oldL,dL, 
		    *m1,*s1,*pi1, 
		    *m2,*s2,*pi2); 
	} 

	// Expectation/P-stage
	for (i = 0; i < values->n; i++) { // Calculate probabilities for each mode
	    g1 = gaussian(values->data.F32[i],*m1,*s1);
	    g2 = gaussian(values->data.F32[i],*m2,*s2);
	    norm = (*pi1 * g1 + *pi2 * g2);
	    //MEH -- must protect denom from norm=0
	    if (norm > 0) {
		P1->data.F32[i] = (*pi1 * g1) / norm;
		P2->data.F32[i] = (*pi2 * g2) / norm;
	    } else {
		P1->data.F32[i] = 0.0;
		P2->data.F32[i] = 0.0;
	    }	 
 
	    if (xyrdebug == 1) {
		fprintf(stderr,"KMM EM-P loop: %d %d %le %le %le\n",
			*iter,i,norm,g1,g2);
	    }

	}
	// Maximization/M-stage
	logL_bimodal = 0;
	w1 = 0;
	w2 = 0;
	for (i = 0; i < values->n; i++) { // Calculate log likelihood
	    if (!((*pi1 == 0)||(*pi2 == 0))) {
		logL_bimodal += log(*pi1 * gaussian(values->data.F32[i],*m1,*s1) +
				    *pi2 * gaussian(values->data.F32[i],*m2,*s2));
	    }
	}
	*m1 = 0;
	*m2 = 0;
	*s1 = 0;
	*s2 = 0;
	for (i = 0; i < values->n; i++) { // Calculate new means and weights
	    *m1 += values->data.F32[i] * P1->data.F32[i];
	    *m2 += values->data.F32[i] * P2->data.F32[i];

	    w1 += P1->data.F32[i];
	    w2 += P2->data.F32[i];

	    if (xyrdebug == 1) {
		fprintf(stderr,"KMM EM-M loop: %d %d (%f %f %f %e) (%f %f %f %e)\n",
			*iter,i,*m1,values->data.F32[i],w1,P1->data.F32[i],*m2,values->data.F32[i],w2,P2->data.F32[i]);
	    }

	}
	*m1 /= w1;
	*m2 /= w2;
	for (i = 0; i < values->n; i++) { // Calculate new sigmas
	    *s1 += pow(values->data.F32[i] - *m1,2) * P1->data.F32[i];
	    *s2 += pow(values->data.F32[i] - *m2,2) * P2->data.F32[i];
	}
	*s1 = sqrt(*s1 / w1);
	*s2 = sqrt(*s2 / w2);

	*pi1 = w1 / values->n;
	*pi2 = w2 / values->n;

	if (!isfinite(*pi1)) { // finite checks
	    *pi1 = 0.0;
	}
	if (!isfinite(*pi2)) { // finite checks
	    *pi2 = 0.0;
	}
	if (*s1 == 0) { // sigma may not be zero -- MEH -- nor <0 and need additive offset if m~0
	    *s1 = fabsf(KMM_SMALL_NUMBER * *m1) + KMM_SMALL_NUMBER;
	}
	if (*s2 == 0) { // sigma may not be zero 
	    *s2 = fabsf(KMM_SMALL_NUMBER * *m2) + KMM_SMALL_NUMBER;
	}

	if (xyrdebug == 1) { 
	    fprintf(stderr,"KMM EM end: %d %f %f %f %f (%f %e %e %f) (%f %e %e %f)\n", 
		    *iter,logL_unimodal,logL_bimodal,oldL,dL, 
		    *m1,*s1,*pi1,w1, 
		    *m2,*s2,*pi2,w2); 
	} 

    } // End EM phase

    // Calculate Punimodal
    double lambda = -2.0 * (logL_unimodal - logL_bimodal);
    int    df     = 2 + 2 * 1; // I can't find my reference on this. 
    if (lambda > 0) {
	*Punimodal = gsl_cdf_chisq_Q(lambda,df);
    }
    else { // If lambda <= 0, then logL_unimodal > logL_bimodal, so Punimodal must be by definition 1.0
	*Punimodal = 1.0;
    }

    if (xyrdebug == 1) { 
	fprintf(stderr,"KMM calc Puni: %d %f %d %f\n", 
		xyrdebug,lambda,df,*Punimodal); 
    } 

    psFree(P1);
    psFree(P2);
}

static void KMMFindPopular(const psVector *values, float *Punimodal, float *mean, float *sigma, float *pi, int xyrdebug) {
    float KMM_MINIMUM_PVALUE = 0.05; // Should be an option.
    float mU,sU;
    float pi1,m1,s1,pi2,m2,s2;
    int iter;

    assert(values);
    assert(values->type.type == PS_TYPE_F32);
  
    KMMcalculate(values,Punimodal,&iter,
		 &mU,&sU,
		 &pi1,&m1,&s1,
		 &pi2,&m2,&s2,xyrdebug);
/*   fprintf(stdout,"%g %g : %g %g %g : %g %g %g : %g %d\t", */
/* 	  mU,sU, */
/* 	  m1,s1,pi1, */
/* 	  m2,s2,pi2, */
/* 	  *Punimodal,iter); */
/*   if (iter > 3) { */
/*     for (int i = 0; i < values->n; i++) { */
/*       fprintf(stdout," %f ",values->data.F32[i]); */
/*     } */
/*   } */
/*   fprintf(stdout,"\n"); */
    if (*Punimodal > KMM_MINIMUM_PVALUE) {
	// Is unimodal
	*mean = mU;
	*sigma = sU;
	*pi = 1.0;
    }
    else {
	// Is bimodal. Select most popular mode.
	if (pi1 >= pi2) {
	    *mean = m1;
	    *sigma = s1;
	    *pi = pi1;
	}
	else {
	    *mean = m2;
	    *sigma = s2;
	    *pi = pi2;
	}
    }  
}

			       


// Determine a mean value and variance for the combination
// Not using psVectorStats because it assumes that the "weights" are errors, and weights by 1/error^2
static bool combinationMeanVariance(float *mean, // Mean value, to return
                                    float *var, // Variance value, to return
                                    float *exp, // Exposure time, to return
                                    float *expWeight,          // Weighted exposure time, to return
                                    const psVector *values, // Values to combine
                                    const psVector *variances, // Pixel variances to combine
                                    const psVector *exps,      // Exposure times to combine
                                    const psVector *weights // Weights to apply
    )
{
    assert(mean);
    assert(values && weights);
    assert(values->n == weights->n);
    assert((var && variances) || !var);
    assert(!variances || variances->n == values->n);
    assert(values->type.type == PS_TYPE_F32);
    assert(!values || values->type.type == PS_TYPE_F32);
    assert(weights->type.type == PS_TYPE_F32);

    // We're not using the input pixel variances to generate a weighted average for the pixel flux (because
    // that introduces systematic biases), so the variance of the output pixel value should simply be:
    //     simga^2 = sum(weight_i^2 * sigma_i^2) / (sum(weight_i))^2
    // This reduces, when the weights are all identically unity, to:
    //     variance_combination = sum(variance_i) / N^2
    // and if the variances are all equal:
    //     variance_combination = variance_individual / N
    // which makes sense --- the standard deviation of the combination is reduced by a factor of sqrt(N).
    // NOTE: in 2012.07.13, the variance calculation was changed without justification to the variance
    // appropriate to a weighted mean, while the pixel mean was kept as the average unweighted by pixel variance

    float sumValueWeight = 0.0;         // Sum of the value multiplied by the weight
    float sumVarianceWeight = 0.0;     // Sum of the pixel variances multiplied by the global weights
    float sumWeight = 0.0;              // Sum of the image weights
    float sumExp = 0.0;                 // Sum of the exposure time
    float sumExpWeight = 0.0;           // Sum of the exposure time multiplied by the global weights
    int numGood = 0;                    // Number of good exposures
    for (int i = 0; i < values->n; i++) {
        sumValueWeight += values->data.F32[i] * weights->data.F32[i];
        sumWeight += weights->data.F32[i];
        if (variances) {
	    //            sumVarianceWeight += variances->data.F32[i] * PS_SQR(weights->data.F32[i]);
	    sumVarianceWeight += 1 / variances->data.F32[i];
        }
        if (exps) {
            sumExp += exps->data.F32[i];
            sumExpWeight += exps->data.F32[i] * weights->data.F32[i];
            numGood++;
        }
    }

    if (sumWeight <= 0) {
        return false;
    }

    *mean = sumValueWeight / sumWeight;
    if (var) {
	//*var = sumVarianceWeight / PS_SQR(sumWeight);
	*var = 1 / sumVarianceWeight;
    }
    if (exp) {
        *exp = sumExp;
    }
    if (expWeight) {
        *expWeight = sumExpWeight;
    }
    return true;
}

// Return the median and standard deviation for the pixels
// Not using psVectorStats because it has additional allocations which slow things down
static bool combinationMedianStdev(float *median, // Median value, to return
                                   float *stdev, // Standard deviation value, to return
                                   const psVector *values, // Values to combine
                                   psVector *sortBuffer // Buffer for sorting
    )
{
    assert(values);
    assert(values->type.type == PS_TYPE_F32);
    assert(sortBuffer && sortBuffer->nalloc >= values->n && sortBuffer->type.type == PS_TYPE_F32);

    int num = values->n;                // Number of values
    sortBuffer = psVectorSortIndex(sortBuffer, values);
    if (!sortBuffer) {
        *median = NAN;
        *stdev = NAN;
        return false;
    }

    if (num == 3) {
        // Attempt to measure standard deviation with only three values (and one of those possibly corrupted)
        *median = values->data.F32[sortBuffer->data.S32[1]];
        if (stdev) {
            float diff1 = values->data.F32[sortBuffer->data.S32[0]] - *median;
            float diff2 = values->data.F32[sortBuffer->data.S32[2]] - *median;
            // This factor of sqrt(2) might not be exact, but it's about right
            *stdev = M_SQRT2 * PS_MIN(fabsf(diff1), fabsf(diff2));
        }
    } else {
        *median = num % 2 ? values->data.F32[sortBuffer->data.S32[num / 2]] :
            (values->data.F32[sortBuffer->data.S32[num / 2 - 1]] +
             values->data.F32[sortBuffer->data.S32[num / 2]]) / 2.0;
        if (stdev) {
            if (num <= NUM_DIRECT_STDEV) {
                // If there are not many values, the direct standard deviation is better
                double sum = 0.0;
                for (int i = 0; i < num; i++) {
                    sum += PS_SQR(values->data.F32[sortBuffer->data.S32[i]] - *median);
                }
                *stdev = sqrt(sum / (double)(num - 1));
            } else {
                // Standard deviation from the interquartile range
                *stdev = 0.74 * (values->data.F32[sortBuffer->data.S32[(int)(0.75 * num)]] -
                                 values->data.F32[sortBuffer->data.S32[(int)(0.25 * num)]]);
            }
        }
    }

    return true;
}

// Return the weighted Olympic mean for the pixels
static float combinationWeightedOlympic(const psVector *values, // Values to combine
                                        const psVector *weights, // Weights to combine
                                        float frac, // Fraction to discard
                                        psVector *sortBuffer // Buffer for sorting
    )
{
    int numGood = values->n;            // Number of good values

    int numBad = frac * numGood + 0.5;  // Number of bad values
    int low = numBad / 2, high = low + numGood - numBad; // Indices (modulo masked pixels)

    sortBuffer = psVectorSortIndex(sortBuffer, values);

    double sumValues = 0.0, sumWeight = 0.0; // Sums for weighted mean
    for (int i = 0, j = 0; i < values->n; i++) {
        int index = sortBuffer->data.S32[i]; // Index of interest
        j++;
        if (j > high) {
            break;
        }
        if (j <= low) {
            continue;
        }
        sumValues += values->data.F32[index] * weights->data.F32[index];
        sumWeight += weights->data.F32[index];
    }

    return sumValues / sumWeight;
}

// Mark a pixel for inspection
// Value in pixel doesn't seem to agree with the stack, so need to look closer
static inline void combineMarkInspect(const psArray *inputs, // Stack data
                                      int x, int y, // Pixel
                                      int source // Source image index
    )
{
    CHECKPIX(x, y, "Marking image %d, pixel %d,%d for inspection\n", source, x, y);
    pmStackData *data = inputs->data[source]; // Stack data of interest
    if (!data) {
        psWarning("Can't find input data for source %d", source);
        return;
    }
    data->inspect = psPixelsAdd(data->inspect, data->inspect->nalloc, x, y);
    return;
}

// Mark a pixel for rejection
// Cannot possibly inspect this pixel and confirm that it's good.
// e.g., Only a single input
static inline void combineMarkReject(const psArray *inputs, // Stack data
                                     int x, int y, // Pixel
                                     int source // Source image index
    )
{
    CHECKPIX(x, y, "Marking pixel image %d, pixel %d,%d for rejection\n", source, x, y);
    pmStackData *data = inputs->data[source]; // Stack data of interest
    if (!data) {
        psWarning("Can't find input data for source %d", source);
        return;
    }
    data->reject = psPixelsAdd(data->reject, data->reject->nalloc, x, y);
    return;
}
#if (0)
// Currently unused function to reject inputs for a given pixel(x,y) based on the selecting the
// most popular mode after running the KMM test, and rejecting all inputs that belong to the
// least popular mode.
static void KMMRejectUnpopular(const psArray *inputs, int x, int y) {
    float KMM_MINIMUM_PVALUE = 0.05;
    float mU,sU;
    float Punimodal,pi1,m1,s1,pi2,m2,s2;
    int iter;
    int j,k;

    psVector *values = psVectorAlloc(inputs->n, PS_TYPE_F32);
    k = 0;
    for (j = 0; j < inputs->n; j++) {
	pmStackData *data = inputs->data[j]; // Stack data of interest
	if (!data) {
	    k++;
	    continue;
	}
	psImage *image = data->readout->image; // Image of interest
	int xIn = x - data->readout->col0, yIn = y - data->readout->row0; // Coordinates on input readout
	values->data.F32[j - k] = image->data.F32[yIn][xIn];
    }
  
    KMMcalculate(values,&Punimodal,&iter,
		 &mU,&sU,
		 &pi1,&m1,&s1,
		 &pi2,&m2,&s2);

    CHECKPIX(x, y, 
	     "KMM Unpopular Test: %d,%d: Puni: %g in %d",x,y,Punimodal,iter);  
    if (Punimodal < KMM_MINIMUM_PVALUE) {
	int i;
	float g1,g2,norm;
	float P1,P2;

	for (i = 0; i < values->n; i++) { // Calculate probabilities for each mode
	    g1 = gaussian(values->data.F32[i],m1,s1);
	    g2 = gaussian(values->data.F32[i],m2,s2);
	    norm = (pi1 * g1 + pi2 * g2);
	    P1 = (pi1 * g1) / norm;
	    P2 = (pi2 * g2) / norm;

	    CHECKPIX(x, y, "KMM Unpopular Rejection: %d,%d: %f(%d): %d %f %f:(%f %f %f ) %f:(%f %f %f) rejection? %d %d\n",
		     x, y,
		     Punimodal,iter,
		     i, values->data.F32[i],
		     P1,m1,s1,pi1,
		     P2,m2,s2,pi2,
		     (pi1 > pi2)&&(P1 < P2),
		     (pi1 < pi2)&&(P1 > P2));
	    if ((pi1 > pi2)&&(P1 < P2)) { // mode 1 is more popular, but this element belongs to mode 2
		combineMarkReject(inputs,x,y,i);
	    }
	    if ((pi1 < pi2)&&(P1 > P2)) { // mode 2 is more popular, but this element belongs to mode 1
		combineMarkReject(inputs,x,y,i);
	    }
	}
    }
    psFree(values);
    // else do nothing.
}

// Currently unused function to reject inputs for a given pixel(x,y) based on the selecting the
// faintest mode as determined by the KMM test, and rejecting all inputs that belong to the brighest.
static void KMMRejectBright(const psArray *inputs, int x, int y) {
    float KMM_MINIMUM_PVALUE = 0.05;
    float mU,sU;
    float Punimodal,pi1,m1,s1,pi2,m2,s2;
    int iter;
    int j;

    psVector *values = psVectorAlloc(inputs->n, PS_TYPE_F32);
    for (j = 0; j < inputs->n; j++) {
	pmStackData *data = inputs->data[j]; // Stack data of interest
	psImage *image = data->readout->image; // Image of interest
	int xIn = x - data->readout->col0, yIn = y - data->readout->row0; // Coordinates on input readout
	values->data.F32[j] = image->data.F32[yIn][xIn];
    }
  
    KMMcalculate(values,&Punimodal,&iter,
		 &mU,&sU,
		 &pi1,&m1,&s1,
		 &pi2,&m2,&s2);
    if (Punimodal < KMM_MINIMUM_PVALUE) {
	int i;
	float g1,g2,norm;
	float P1,P2;

	for (i = 0; i < values->n; i++) { // Calculate probabilities for each mode
	    g1 = gaussian(values->data.F32[i],m1,s1);
	    g2 = gaussian(values->data.F32[i],m2,s2);
	    norm = (pi1 * g1 + pi2 * g2);
	    P1 = (pi1 * g1) / norm;
	    P2 = (pi2 * g2) / norm;

	    if ((m1 > m2)&&(P1 > P2)) { // m1 is larger, and this element belongs to mode 1
		combineMarkReject(inputs,x,y,i);
	    }
	    if ((m1 < m2)&&(P1 < P2)) { // m2 is larger, and this element belongs to mode 2
		combineMarkReject(inputs,x,y,i);
	    }
	}
    }
    psFree(values);
    // else do nothing.
}
#endif // End if(0) to prevent KMMReject{Unpopular|Bright} from being defined.

// Extract vectors for simple combination/rejection operations
static void combineExtract(int *num,                        // Number of good pixels
                           bool *suspect,                   // Any suspect pixels?
			   psImageMaskType *badMask,	    // OR of all bad (masked) pixels
			   psImageMaskType *goodMask,	    // OR of all good (unmasked) pixels
                           combineBuffer *buffer, // Buffer with vectors
                           psImage *image, // Combined image, for output
                           psImage *mask, // Combined mask, for output
                           psImage *variance, // Combined variance map, for output
                           const psArray *inputs, // Stack data
                           const psVector *weights, // Global (single value) weights for data, or NULL
                           const psVector *exps,    // Exposures for data, or NULL
                           const psVector *addVariance, // Additional variance for data
                           const psVector *reject, // Indices of pixels to reject, or NULL
                           int x, int y, // Coordinates of interest; frame of output image
                           psImageMaskType badMaskBits, // Value to mask as 'bad'
                           psImageMaskType suspectMaskBits // Value to mask as 'suspect'
    )
{
    // Rudimentary error checking
    assert(buffer);
    assert(image);
    assert(mask);
    assert(inputs);

    psVector *pixelData = buffer->pixels; // Values for the pixel of interest
    psVector *pixelVariances = variance ? buffer->variances : NULL; // Variances for the pixel of interest
    psVector *pixelWeights = buffer->weights; // Image weights for the pixel of interest
    psVector *pixelExps = buffer->exps;       // Exposure times
    psVector *pixelSources = buffer->sources; // Sources for the pixel of interest
    psVector *pixelLimits = buffer->limits; // Limits for the pixel of interest
    psVector *pixelSuspects = buffer->suspects; // Is the pixel suspect?

    if (suspect) {
        *suspect = false;
    }

    // mask values to store possible mask combinations
    *badMask = 0;
    *goodMask = 0xffff;

    int nGoodBits[16]; // accumulate the good pixel bits here for fuzzy logic
    psAssert (sizeof(psImageMaskType) == 2, "psImageMaskType is not the expected size");
    memset (nGoodBits, 0, 16*sizeof(int));


    // Extract the pixel and mask data    
    int numGood = 0;                    // Number of good pixels
    for (int i = 0, j = 0; i < inputs->n; i++) {
        // Check if this pixel has been rejected.  Assumes that the rejection pixel list is sorted --- it
        // should be because of how pixelMapGenerate works
        if (reject && reject->data.U16[j] == i) {
	    // pixels can be rejected because:
	    // 1) only 1 input pixel (and 'safe' is true)
	    // 2) only 2 input pixels were available and they were mutually inconsistent (or variance info was missing)
	    // 3) NOTE : ifdef'ed out code for 3 inputs case 
	    // 4) outlier from sample of N pixels
	    // 5) some of these may have been suspect.
	    // XXX raise a specific mask bit for these (currently results in BLANK)
            j++;

	    pmStackData *data = inputs->data[i]; // Stack data of interest
	    if (data) {
		int xIn = x - data->readout->col0, yIn = y - data->readout->row0; // Coordinates on input readout
		psImage *mask = data->readout->mask; // Mask of interest
		*badMask |= mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn]; // save the bad bits used
		CHECKPIX(x, y, "reject: adding bit to mask: %d : %x (badMask = %x)\n", i, mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn], *badMask);
	    } else {
		CHECKPIX(x, y, "reject: no item in data (badMask = %x)\n", *badMask);
	    }
            continue;
        }

        pmStackData *data = inputs->data[i]; // Stack data of interest
        if (!data) {
	    CHECKPIX(x, y, "skip: no item in data (badMask = %x)\n", *badMask);
            continue;
        }
	
        int xIn = x - data->readout->col0, yIn = y - data->readout->row0; // Coordinates on input readout
        psImage *mask = data->readout->mask; // Mask of interest
        if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & badMaskBits) {
	    *badMask |= (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & badMaskBits); // save the bad bits used
	    CHECKPIX(x, y, "skip: adding bit to mask: %d : %x (badMask = %x)\n", i, mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn], *badMask);
            continue;
        }

        pixelSuspects->data.U8[numGood] = mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & suspectMaskBits ? true : false;

	// *goodMask &= mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn]; // save the mask bits still used
	// check for set bits and increment counter as appropriate
	// count the number of times a given mask bit is set in the input pixels.
	// NOTE: since we have explicitly skipped the pixels with any bad bits, these are only
	// the suspect bits (nGoodBits is a bit of a misnomer: it is more like 'nSuspectBitsForGoodInputs'
	{ 
	    psImageMaskType value = 0x0001;
	    for (int nbit = 0; nbit < 16; nbit ++) {
		if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & value) {
		    nGoodBits[nbit] ++;
		}
		value <<= 1;
	    }
	}

        psImage *image = data->readout->image; // Image of interest
        psImage *variance = data->readout->variance; // Variance map of interest
        pixelData->data.F32[numGood] = image->data.F32[yIn][xIn];
        if (variance) {
            pixelVariances->data.F32[numGood] = variance->data.F32[yIn][xIn] * addVariance->data.F32[i];
        }
        pixelWeights->data.F32[numGood] = data->weight;
        pixelExps->data.F32[numGood] = data->exp;
        pixelSources->data.U16[numGood] = i;
        numGood++;

	CHECKPIX(x, y, "keep: %d : %x (badMask = %x)\n", i, mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn], *badMask);
    }
    
    pixelData->n = numGood;
    if (variance) {
        pixelVariances->n = numGood;
    }
    pixelWeights->n = numGood;
    pixelSources->n = numGood;
    pixelLimits->n = numGood;
    pixelSuspects->n = numGood;
    *num = numGood;

    // set the mask bits if nGoodBits[i] > f*numGood
    {
# define SUSPECT_FRACTION 0.65
	*goodMask = 0x0000;
	psImageMaskType value = 0x0001;
	for (int nbit = 0; nbit < 16; nbit ++) {
	    if (nGoodBits[nbit] > SUSPECT_FRACTION*numGood) {
		*goodMask |= value;
	    }
	    value <<= 1;
	}
    }

#ifdef TESTING
    if (PS_SQR(x - TEST_X) + PS_SQR(y - TEST_Y) <= PS_SQR(TEST_RADIUS)) {
        for (int i = 0; i < numGood; i++) {
	    fprintf(stderr,"Input %d, pixel %d,%d (%" PRIu16 "): %f %f (%g) %g %f %d %x %x -> %x %x\n",
                    i, x, y, pixelSources->data.U16[i],
		    pixelData->data.F32[i], pixelVariances->data.F32[i],
                    addVariance->data.F32[i],
		    pixelWeights->data.F32[i], pixelExps->data.F32[i],
                    pixelSuspects->data.U8[i],
		    badMaskBits, suspectMaskBits,
		    *badMask, *goodMask);
        }
    }
#endif

    return;
}

// Combine pixels
static void combinePixels(psImage *image, // Combined image, for output
                          psImage *mask, // Combined mask, for output
                          psImage *variance, // Combined variance map, for output
                          psImage *exp,   // Exposure map (time), for output
                          psImage *expnum,       // Exposure map (number) for output
                          psImage *expweight,    // Exposure map (weighted time) for output
                          int num,      // Number of good pixels
                          combineBuffer *buffer, // Buffer with vectors
                          int x, int y, // Coordinates of interest; frame of output image
                          psImageMaskType blankMask, // Value for empty pixels
                          psImageMaskType badMask, // Value for bad pixels
                          psImageMaskType goodMask, // Value for good pixels
                          bool safe,           // Safe combination?
			  int nminpix,         // Minimum number of input per pixel
                          float invTotalWeight    // Inverse of total weight for all inputs
    )
{
    psVector *pixelData = buffer->pixels; // Values for the pixel of interest
    psVector *pixelVariances = variance ? buffer->variances : NULL; // Variances for the pixel of interest
    psVector *pixelWeights = buffer->weights; // Image weights for the pixel of interest
    psVector *pixelExps = buffer->exps;       // Exposure times

    // Default option is that the pixel is bad
    float imageValue = NAN, varianceValue = NAN; // Value for combined image and variance map
    float expValue = 0.0, expWeightValue = NAN; // Exposure value (straight, and weighted)

    // default output mask value.  badMask is the OR of all unused input pixels.  
    // if there are no input pixels, this value will be 0, in which case we want to set the output pixel to BLANK.
    // if there are only good input pixels, and they do not result in a valid pixel, we still want to set this to BLANK.
    psImageMaskType maskValue = badMask ? badMask : blankMask;    // Value for combined mask 

    CHECKPIX(x, y, "bad vs good : %x %x %x\n", maskValue, badMask, blankMask);

    //MEH -- hackish adding of lower limit for input per pixel 
    int numN = num;
    if (num < nminpix) {
        CHECKPIX(x, y, "Nmin (%d) inputs (%d) to combine, pixel %d,%d is manually set bad\n", nminpix, numN, x, y);
        numN = 0;
    }
    switch (numN) {
      case 0: {
          // Nothing to combine: it's bad
	  CHECKPIX(x, y, "No inputs to combine, pixel %d,%d is bad.\n", x, y);
          break;
      }
      case 1: {
          // Accept the single pixel unless we have to be safe
          if (!safe) {
              imageValue = pixelData->data.F32[0];
              if (variance) {
                  varianceValue = pixelVariances->data.F32[0];
              }
              if (exp) {
                  expValue = pixelExps->data.F32[0];
                  expWeightValue = pixelExps->data.F32[0];
              }
              maskValue = goodMask;
	      CHECKPIX(x, y, "Single input to combine, safety off, pixel %d,%d --> %f\n", x, y, imageValue);
          } else {
	      CHECKPIX(x, y, "Single input to combine, safety on, pixel %d,%d is bad.\n", x, y);
	  }
          break;
      }
      case 2: {
          // Automatically accept the mean of the pixels only if we're not playing safe
          if (!safe) {
              if (combinationMeanVariance(&imageValue, &varianceValue, &expValue, &expWeightValue, pixelData, pixelVariances, pixelExps, pixelWeights)) {
		  maskValue = goodMask;
		  CHECKPIX(x, y, "Two inputs to combine using unsafe, pixel %d,%d --> %f %f\n", x, y, imageValue, varianceValue);
              }
          } else {
	      CHECKPIX(x, y, "Two inputs to combine, safety on, pixel %d,%d is bad\n", x, y);
          }
          break;
      }
      default: {
          // Can combine without too much worrying
          if (!combinationMeanVariance(&imageValue, &varianceValue, &expValue, &expWeightValue,
                                       pixelData, pixelVariances, pixelExps, pixelWeights)) {
              break;
          }
	  maskValue = goodMask;
	  CHECKPIX(x, y, "Combined inputs, pixel %d,%d --> %f %f\n", x, y, imageValue, varianceValue);
          break;
      }
    }

    image->data.F32[y][x] = imageValue;
    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = maskValue;
    if (variance) {
        variance->data.F32[y][x] = varianceValue;
    }
    if (exp) {
        exp->data.F32[y][x] = expValue;
    }
    if (expnum) {
        expnum->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = num;
    }
    if (expweight) {
        expweight->data.F32[y][x] = expWeightValue * invTotalWeight;
    }

    return;
}


// Test pixels to be combined
// Returns false to repeat without suspect pixels
static bool combineTest(int num,      // Number of good pixels
                        bool suspect, // Does the stack contain suspect pixels?
                        psArray *inputs,       // Original inputs (for flagging)
                        combineBuffer *buffer, // Buffer with vectors
                        int x, int y, // Coordinates of interest; frame of output image
                        float iter, // Number of rejection iterations per input
                        float rej, // Number of standard deviations at which to reject
                        float sys,    // Relative systematic error
                        float olympic,// Fraction of values to discard (Olympic weighted mean)
                        bool useVariance, // Use variance for rejection when combining?
                        bool safe    // Combine safely?
    )
{
    if (iter <= 0) {
        return true;
    }

    int numIter = PS_MAX(iter * num, 1); // Number of iterations

    CHECKPIX(x, y, "Testing pixel %d,%d: %d %f %f %f %d %d\n", x, y, numIter, rej, sys, olympic, useVariance, safe);

    psVector *pixelData = buffer->pixels; // Values for the pixel of interest
    psVector *pixelWeights = buffer->weights; // Is the pixel suspect?
    psVector *pixelVariances = buffer->variances; // Variances for the pixel of interest
    psVector *pixelSources = buffer->sources; // Sources for the pixel of interest
    psVector *pixelSuspects = buffer->suspects; // Is the pixel suspect?
    psVector *pixelLimits = buffer->limits; // Is the pixel suspect?
    //MEH -- adding a debug option for TESTING xyr position but could be better..
    int xyrdebug = 0;

    // KMM values;
    float Punimodal = 1.0, KMMmean = NAN, KMMsigma = NAN, KMMpi = NAN;
    int KMM_MINIMUM_INPUTS = 6;
    bool useKMM = false;
    if (num >= KMM_MINIMUM_INPUTS) {
	useKMM = true;
    }
    
    // Set up rejection limits
    float rej2 = PS_SQR(rej); // Rejection level squared
    if (num > 2 && useVariance) {
        // Convert rejection limits --- saves doing it later multiple times
        // Using squared rejection limit because it's cheaper than sqrts
        double sumWeights = 0.0;

	// Determine the systematic error from the most popular population in the sample
	// This should probably be an option
	if (useKMM) {
#ifdef TESTING
            if (PS_SQR(x - TEST_X) + PS_SQR(y - TEST_Y) <= PS_SQR(TEST_RADIUS)) {
                xyrdebug = 1;
            }
# endif
	    KMMFindPopular(pixelData,&Punimodal,&KMMmean,&KMMsigma,&KMMpi,xyrdebug);
	    CHECKPIX(x,y,"KMM Popularity Contest: (%d,%d) Puni: %g Mean: %f Sigma %f Pi: %f\n",
		     x,y,Punimodal,KMMmean,KMMsigma,KMMpi);
	}
        for (int i = 0; i < num; i++) {
            sumWeights += pixelWeights->data.F32[i];
        }
        for (int i = 0; i < num; i++) {
	    // Systematic error contributes to the rejection level
	    float sysVar;
	    if (useKMM) { // If we can trust KMM results, set the systematic variance
		sysVar = PS_SQR(KMMsigma);
	    }
	    else { // Otherwise, use the 10% systematic variance we've done in the past.
		sysVar = PS_SQR(sys * pixelData->data.F32[i]);
	    }

	    CHECKPIX(x, y, "Variance %d (%d), pixel %d,%d: %f %f %f\n", 
		     i, pixelSources->data.U16[i], x, y, 
		     pixelVariances->data.F32[i], sysVar, 1.0 - pixelWeights->data.F32[i] / sumWeights);
            pixelLimits->data.F32[i] = rej2 * (pixelVariances->data.F32[i] + sysVar);
        }
    }

    int maskIndex = 0;                  // Index of pixel to mask
    int totalClipped = 0;               // Total number of pixels clipped
    for (int i = 0; i < numIter && maskIndex >= 0; i++) {
        maskIndex = -1;                 // Nothing to reject

        switch (num) {
          case 0:
            break;
          case 1:
            if (i == 0 && safe) {
                combineMarkReject(inputs, x, y, pixelSources->data.U16[0]);
            }
            break;
          case 2: {
              if (useVariance) {
                  // Use variance to check that the two are consistent
                  float diff = 0.5 * (pixelData->data.F32[0] - pixelData->data.F32[1]); // Mean flux difference
                  float var1 = pixelVariances->data.F32[0]; // Variance of first
                  float var2 = pixelVariances->data.F32[1]; // Variance of second
                  // Systematic error contributes to the rejection level
                  var1 += PS_SQR(sys * pixelData->data.F32[0]);
                  var2 += PS_SQR(sys * pixelData->data.F32[1]);

                  float sigma2 = var1 + var2; // Combined variance
                  if (PS_SQR(diff) > rej2 * sigma2) {
                      // Not consistent: don't believe either!
                      if (i == 0 && suspect) {
                          combineMarkReject(inputs, x, y, pixelSources->data.U16[0]);
                          combineMarkReject(inputs, x, y, pixelSources->data.U16[1]);
                      } else {
                          combineMarkInspect(inputs, x, y, pixelSources->data.U16[0]);
                          combineMarkInspect(inputs, x, y, pixelSources->data.U16[1]);
                      }
		      CHECKPIX(x, y, "Flagged both inputs for pixel %d,%d (%f > %f x %f\n)", x, y, diff, rej, sqrtf(sigma2));
                  }
              } else if (i == 0 && safe) {
                  // Can't test them, and we want to be safe, so reject
                  combineMarkReject(inputs, x, y, pixelSources->data.U16[0]);
                  combineMarkReject(inputs, x, y, pixelSources->data.U16[1]);
              }
              break;
          }
#if 0
          case 3: {
              // Want to be a bit careful on the rejection than for a larger number of inputs
              if (!useVariance) {
                  return combineTestGeneral(num, suspect, inputs, buffer, x, y, numIter, rej, sys,
                                            olympic, useVariance, safe, allowSuspect);
              }

              // Differences between pixel values
              float diff01 = pixelData->data.F32[0] - pixelData->data.F32[1];
              float diff12 = pixelData->data.F32[1] - pixelData->data.F32[2];
              float diff20 = pixelData->data.F32[2] - pixelData->data.F32[0];
              // Variance for each pixel
              float var0 = pixelVariances->data.F32[0] + PS_SQR(sys * pixelData->data.F32[0]);
              float var1 = pixelVariances->data.F32[1] + PS_SQR(sys * pixelData->data.F32[1]);
              float var2 = pixelVariances->data.F32[2] + PS_SQR(sys * pixelData->data.F32[2]);
              // Errors in pixel differences
              float err01 = var0 + var1;
              float err12 = var1 + var2;
              float err20 = var2 + var0;

#ifdef TESTING
              if (PS_SQR(x - TEST_X) + PS_SQR(y - TEST_Y) <= PS_SQR(TEST_RADIUS)) {
                  fprintf(stderr, "Diff 0-1: %f %f\n", diff01, err01);
                  fprintf(stderr, "Diff 1-2: %f %f\n", diff12, err12);
                  fprintf(stderr, "Diff 2-0: %f %f\n", diff20, err20);
              }
#endif

              int badPairs = 0;         // Number of bad pairs
              bool bad01 = false, bad12 = false, bad20 = false; // Pair is bad?
              if (PS_SQR(diff01) > rej2 * err01) {
                  bad01 = true;
                  badPairs++;
              }
              if (PS_SQR(diff12) > rej2 * err12) {
                  bad12 = true;
                  badPairs++;
              }
              if (PS_SQR(diff20) > rej2 * err20) {
                  bad20 = true;
                  badPairs++;
              }

              if (badPairs > 0 && allowSuspect && suspect) {
                  return false;
              }

              switch (badPairs) {
                case 0:
                  // Nothing to worry about!
                  break;
                case 1:
                  // Can't tell which image is bad, so be sure to get it
                  if (bad01) {
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[0]);
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[1]);
                      break;
                  }
                  if (bad12) {
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[1]);
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[2]);
                      break;
                  }
                  if (bad20) {
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[2]);
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[0]);
                      break;
                  }
                  psAbort("Should never get here");
                case 2:
                  if (bad01 && bad12) {
                      // 2 and 0 are good
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[1]);
                      break;
                  }
                  if (bad12 && bad20) {
                      // 0 and 1 are good
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[2]);
                      break;
                  }
                  if (bad20 && bad01) {
                      // 1 and 2 are good
                      combineMarkInspect(inputs, x, y, pixelSources->data.U16[0]);
                      break;
                  }
                  psAbort("Should never get here");
                case 3:
                  // Everything's bad
                  combineMarkInspect(inputs, x, y, pixelSources->data.U16[0]);
                  combineMarkInspect(inputs, x, y, pixelSources->data.U16[1]);
                  combineMarkInspect(inputs, x, y, pixelSources->data.U16[2]);
                  break;
              }
              break;
          }
#endif
          default: {
              if (useVariance) {
		  float median;
		  if ((useKMM)&&(Punimodal < 0.05)) {
		      median = KMMmean;
		  }
		  else {
		      median = combinationWeightedOlympic(pixelData, pixelWeights,
							  olympic, buffer->sort); // Median for stack
		  }
		
		  CHECKPIX(x, y, "Flag with variance pixel %d,%d: median = %f\n", x, y, median);
                  float worst = -INFINITY; // Largest deviation
                  for (int j = 0; j < num; j++) {
		      float diff = pixelData->data.F32[j] - median; // Difference from expected
		      CHECKPIX(x, y, "Testing input %d for pixel %d,%d: %f\n", j, x, y, diff);

                      // Comparing squares --- cheaper than lots of sqrts
                      // pixelVariances includes the rejection limit, from above
                      float diff2 = PS_SQR(diff); // Square difference
		      CHECKPIX(x,y, "Input %d, pixel %d,%d (%" PRIu16 "): %f %f (%f) %f %f :: %f %f %f %f\n",
			       i, x, y, pixelSources->data.U16[j], pixelData->data.F32[j], pixelVariances->data.F32[j],
			       1.0, pixelWeights->data.F32[j], 1.0,
			       pixelLimits->data.F32[j], diff2, diff2 / pixelLimits->data.F32[j],worst);

                      if (diff2 > pixelLimits->data.F32[j]) {
                          float dev = diff2 / pixelLimits->data.F32[j]; // Deviation
                          if (dev > worst) {
                              worst = dev;
                              maskIndex = j;
                          }
                      }
                  }
              } else {
                  float median = NAN, stdev = NAN;  // Median and stdev of the combination, for rejection
                  combinationMedianStdev(&median, &stdev, pixelData, buffer->sort);
                  float limit = rej * stdev; // Rejection limit
		  CHECKPIX(x, y, "Flag without variance pixel %d,%d; median = %f, stdev = %f, limit = %f\n", x, y, median, stdev, limit);
                  float worst = -INFINITY; // Largest deviation
                  for (int j = 0; j < num; j++) {
                      float diff = fabsf(pixelData->data.F32[j] - median); // Difference from expected

                      if (diff > limit) {
                          float dev = diff / limit; // Deviation
                          if (dev > worst) {
                              worst = dev;
                              maskIndex = j;
                          }
                      }
                  }
              }
          }
        }

        // Do the actual rejection of the pixel
        if (maskIndex >= 0) {
            if (suspect) {
		CHECKPIX(x, y, "Throwing out all suspect pixels for %d,%d\n", x, y);
                // Throw out all suspect pixels
                int numGood = 0;        // Number of good pixels
                for (int j = 0; j < num; j++) {
                    if (pixelSuspects->data.U8[j]) {
                        combineMarkReject(inputs, x, y, pixelSources->data.U16[j]);
                        continue;
                    }
                    if (numGood == j) {
                        numGood++;
                        continue;
                    }
                    pixelData->data.F32[numGood] = pixelData->data.F32[j];
                    pixelWeights->data.F32[numGood] = pixelWeights->data.F32[j];
                    pixelSources->data.U16[numGood] = pixelSources->data.U16[j];
                    pixelLimits->data.F32[numGood] = pixelLimits->data.F32[j];
                    pixelVariances->data.F32[numGood] = pixelVariances->data.F32[j];
                    numGood++;
                }
                pixelData->n = numGood;
                pixelWeights->n = numGood;
                pixelSources->n = numGood;
                pixelLimits->n = numGood;
                pixelVariances->n = numGood;
                totalClipped += num - numGood;
                num = numGood;
                suspect = false;
            } else {
                // Throw out masked pixel
		CHECKPIX(x, y, "Throwing out input %d for pixel %d,%d\n", maskIndex, x, y);
                combineMarkInspect(inputs, x, y, pixelSources->data.U16[maskIndex]);
                int numGood = 0;        // Number of good pixels
                for (int j = 0; j < num; j++) {
                    if (j == maskIndex) {
                        continue;
                    }
                    if (numGood == j) {
                        numGood++;
                        continue;
                    }
                    pixelData->data.F32[numGood] = pixelData->data.F32[j];
                    pixelWeights->data.F32[numGood] = pixelWeights->data.F32[j];
                    pixelSources->data.U16[numGood] = pixelSources->data.U16[j];
                    pixelLimits->data.F32[numGood] = pixelLimits->data.F32[j];
                    pixelVariances->data.F32[numGood] = pixelVariances->data.F32[j];
                    numGood++;
                }
                pixelData->n = numGood;
                pixelWeights->n = numGood;
                pixelSources->n = numGood;
                pixelLimits->n = numGood;
                pixelVariances->n = numGood;
                totalClipped++;
                num--;
            }
        }
    }

    return true;
}


// Ensure the input array of pmStackData is valid, and get some details out of it
static bool validateInputData(bool *haveVariances, // Do we have variance maps in the sky images?
                              int *num,    // Number of inputs
                              int *numCols, int *numRows, // Size of (sky) images
                              const psArray *input, // Input array of pmStackData to validate
                              const pmReadout *output, // Output readout
                              const pmReadout *exp    // Exposure map
    )
{
    PS_ASSERT_ARRAY_NON_NULL(input, false);
    *num = input->n;

    pmStackData *data = NULL;           // First image off the rank, used as a template
    for (int i = 0; !data && i < input->n; i++) {
        data = input->data[i];
    }
    PS_ASSERT_PTR_NON_NULL(data, false);
    assert(psMemGetDeallocator(data) == (psFreeFunc)stackDataFree); // Ensure it's the right type
    *haveVariances = false;
    PS_ASSERT_IMAGE_NON_NULL(data->readout->image, false);
    PS_ASSERT_IMAGE_TYPE(data->readout->image, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_NON_NULL(data->readout->mask, false);
    PS_ASSERT_IMAGE_TYPE(data->readout->mask, PS_TYPE_IMAGE_MASK, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(data->readout->image, data->readout->mask, false);
    *numCols = data->readout->image->numCols;
    *numRows = data->readout->image->numRows;
    if (data->readout->variance) {
        *haveVariances = true;
        PS_ASSERT_IMAGE_NON_NULL(data->readout->variance, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(data->readout->image, data->readout->variance, false);
        PS_ASSERT_IMAGE_TYPE(data->readout->variance, PS_TYPE_F32, false);
    }
    bool haveRejects = (data->reject != NULL); // Do we have rejected pixels?

    // Make sure the rest correspond with the first
    for (int i = 1; i < *num; i++) {
        pmStackData *data = input->data[i]; // Stack data for this input
        if (!data) {
            continue;
        }
        assert(psMemGetDeallocator(data) == (psFreeFunc)stackDataFree); // Ensure it's the right type
        if (!data->readout) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "The readout is specified in some but not all inputs.");
            return false;
        }
        if ((haveRejects && !data->reject) || (data->reject && !haveRejects)) {
            psError(PS_ERR_UNEXPECTED_NULL, true,
                    "The rejected pixels are specified in some but not all inputs.");
            return false;
        }
        PS_ASSERT_IMAGE_NON_NULL(data->readout->image, false);
        PS_ASSERT_IMAGE_NON_NULL(data->readout->mask, false);
        PS_ASSERT_IMAGE_TYPE(data->readout->image, PS_TYPE_F32, false);
        PS_ASSERT_IMAGE_TYPE(data->readout->mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGE_SIZE(data->readout->image, *numCols, *numRows, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(data->readout->image, data->readout->mask, false);
        if (*haveVariances) {
            PS_ASSERT_IMAGE_NON_NULL(data->readout->variance, false);
            PS_ASSERT_IMAGES_SIZE_EQUAL(data->readout->image, data->readout->variance, false);
            PS_ASSERT_IMAGE_TYPE(data->readout->variance, PS_TYPE_F32, false);
        }
    }

    PM_ASSERT_READOUT_NON_NULL(output, false);
    if (output->image) {
        PS_ASSERT_IMAGE_NON_NULL(output->image, false);
        PS_ASSERT_IMAGE_TYPE(output->image, PS_TYPE_F32, false);
        PS_ASSERT_IMAGE_NON_NULL(output->mask, false);
        PS_ASSERT_IMAGE_TYPE(output->mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(output->image, output->mask, false);
    }

    if (exp) {
        PM_ASSERT_READOUT_NON_NULL(exp, false);
        if (exp->image) {
            PS_ASSERT_IMAGES_SIZE_EQUAL(exp->image, output->image, false);
        }
        if (exp->mask) {
            PS_ASSERT_IMAGES_SIZE_EQUAL(exp->mask, output->image, false);
        }
    }

    return true;
}


// Generate a "pixel map".
//
// A "pixel map" is an image-like structure containing a vector that contains the indices of images.  The idea
// is to provide a reverse lookup for an array of pixel lists, so that the image for which a pixel is flagged
// can be identified easily.
static psArray *pixelMapGenerate(const psArray *input, // Data to stack
                                 int minCols, int maxCols, int minRows, int maxRows // Bounds of interest
    )
{
    int numCols = maxCols - minCols + 1, numRows = maxRows - minRows + 1; // Size of map

    psArray *map = psArrayAlloc(numRows); // The pixel map
    for (int y = 0; y < numRows; y++) {
        map->data[y] = psArrayAlloc(numCols);
    }

    for (int i = 0; i < input->n; i++) {
        pmStackData *data = input->data[i];
        if (!data) {
            continue;
        }
        assert(data->reject);
        psPixels *pixels = data->reject; // The rejected pixels
        for (int j = 0; j < pixels->n; j++) {
            int x = pixels->data[j].x - minCols, y = pixels->data[j].y - minRows; // Coordinates of interest
            if (x < 0 || x >= numCols || y < 0 || y >= numRows) {
                continue;
            }
            psArray *columns = map->data[y]; // The columns for that row
            psVector *images = columns->data[x]; // The images for that column
            if (!images) {
                images = columns->data[x] = psVectorAllocEmpty(PIXEL_MAP_BUFFER, PS_TYPE_U16);
            }
            int size = images->n;       // Element number at which to add
            columns->data[x] = psVectorExtend(images, PIXEL_MAP_BUFFER, 1);
            images->data.U16[size] = i;
        }
    }

    return map;
}

// Query a "pixel map", by returning the list of image indices for a particular pixel.
static psVector *pixelMapQuery(const psArray *map, // Pixel map
                               int x0, int y0, // Offset into map
                               int x, int y // Coordinates of interest
    )
{
    // Adjust for offset
    x -= x0;
    y -= y0;

    assert(y >= 0 && y < map->n);
    psArray *colMap = map->data[y];     // Columns for that row
    assert(x >= 0 && x < colMap->n);
    return colMap->data[x];
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Constructor
pmStackData *pmStackDataAlloc(pmReadout *readout, float weight, float exp, float addVariance)
{
    pmStackData *data = psAlloc(sizeof(pmStackData)); // Stack data, to return
    psMemSetDeallocator(data, (psFreeFunc)stackDataFree);

    data->readout = psMemIncrRefCounter(readout);
    data->reject = NULL;
    data->inspect = NULL;
    data->weight = weight;
    data->exp = exp;
    data->addVariance = addVariance;

    return data;
}


bool pmStackSimpleMedianCombine(
    pmReadout *combined,
    psArray *input) {
    int num = input->n;
    //  int numCols, numRows;
    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image

    psArray *stack = psArrayAlloc(num); // Stack of readouts  
    for (int i = 0; i < num; i++) {
	//    pmStackData *data = input->data[i]; // Stack data for this input
	pmReadout *ro = input->data[i]; // data->readout;  // Readout of interest
	if (!ro) {
	    continue;
	}
	stack->data[i] = psMemIncrRefCounter(ro);
    }    

    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize,
				stack)) {
	psError(psErrorCodeLast(), false, "Input stack is not valid.");
	psFree(stack);
	return false;
    }

    psVector *pixelData = psVectorAlloc(input->n,PS_TYPE_F32);
    psVector *pixelMask = psVectorAlloc(input->n,PS_TYPE_VECTOR_MASK);
    psStats  *stats     = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);

    for (int y = minInputRows; y < maxInputRows; y++) {
	for (int x = minInputCols; x < maxInputCols; x++) {
	    for (int i = 0; i < input->n; i++) {
		pmReadout *ro  = stack->data[i];
		psImage *image = ro->image;
		pixelMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0;
		pixelData->data.F32[i] = image->data.F32[y][x];
		if (isfinite(image->data.F32[y][x])&&
		    (fabs(image->data.F32[y][x]) < 1e5)) {
		    pixelMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0;
		}
		else {
		    pixelMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
		}
#if (0)
		if ((x == 59)&&(y > 40)&&(y < 50)) {
		    fprintf(stderr,"%d %d %d %d %g\n",
			    x,y,i,pixelMask->data.PS_TYPE_VECTOR_MASK_DATA[i],pixelData->data.F32[i]);
		}
#endif
	    }
	    if (!psVectorStats(stats,pixelData,NULL,pixelMask,1)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to calculate median");
		psFree(stats);
		psFree(pixelData);
		psFree(pixelMask);
		psFree(stack);
		return(false);
	    }
	    combined->image->data.F32[y][x] = stats->robustMedian;
	    if (combined->mask) {
		combined->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = 0;
	    }
#if (0)
	    if ((x == 59)&&(y > 40)&&(y < 50)) {
		fprintf(stderr,"%d %d %d %d %g\n",
			x,y,-1,combined->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x],
			combined->image->data.F32[y][x]);
	    }
#endif
	}
    }
  
    psFree(stats);
    psFree(pixelData);
    psFree(pixelMask);
    psFree(stack);
    return (true);
}

# define SUSPECT_FRACTION 0.65

// Comparison and swap functions for sorting values directly
#define SORT_VV_COMPARE(A,B) (pixelData->data.F32[A] < pixelData->data.F32[B])
#define SORT_VV_SWAP(TYPE,A,B) {					\
	if (A != B) {							\
	    psF32 tempVal = pixelData->data.F32[A];			\
	    pixelData->data.F32[A] = pixelData->data.F32[B];		\
	    pixelData->data.F32[B] = tempVal;				\
	    psF32 tempVar = pixelVariances->data.F32[A];		\
	    pixelVariances->data.F32[A] = pixelVariances->data.F32[B];	\
	    pixelVariances->data.F32[B] = tempVar;			\
	    if (expTime) {						\
		psF32 tempExp = expTime->data.F32[A];			\
		expTime->data.F32[A] = expTime->data.F32[B];		\
		expTime->data.F32[B] = tempExp;				\
	    }								\
	}								\
    }

// this macro uses the macros above which assume pixelData, pixelVariances, expTime
#define SORT_VALUES(NVALUES) { PSSORT(NVALUES, SORT_VV_COMPARE, SORT_VV_SWAP, F32); }

#define ESCAPE	{							\
  combined->image->data.F32[y][x] = NAN;				\
  combined->variance->data.F32[y][x] = NAN;				\
  combined->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = blankMaskBits;	\
  if (expmaps) {							\
    expmaps->image->data.F32[y][x] = 0.0;				\
    expmaps->variance->data.F32[y][x] = 0.0;				\
    expmaps->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = 0;		\
  } continue; }

// combine pixels, averaging the inner XX percentile range, after rejecting 5-sigma outliers (robust 5 sigma)
bool pmStackCombineByPercentile(
    pmReadout *combined,		// output stacked readout
    pmReadout *expmaps,			// output exposure map information
    psArray *stackData,			// input exposures
    psF64 rejectFraction,               // outlier fraction of pixels to reject
    int nminpix,			// minimum number of input values required to generate an output value
    psImageMaskType badMaskBits, 	// treat these bits as 'bad'
    psImageMaskType suspectMaskBits,	// treat these bits as 'suspect'
    psImageMaskType blankMaskBits       // use this mask value for pixels missing input data (distinguish between Ninput = 0 and Ngood = 0?)
				) {
    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image

    // we need to copy the readouts to their own array for validation
    psArray *stackReadouts = psArrayAlloc(stackData->n);
    for (int i = 0; i < stackData->n; i++) {
        pmStackData *data = stackData->data[i]; // Stack data for this input
	stackReadouts->data[i] = NULL;
	if (!data) continue;
	pmReadout *ro = data->readout;  // Readout of interest
	if (!ro) continue;
	stackReadouts->data[i] = psMemIncrRefCounter(ro); // need to bump the counter since the free below will decrement
    }
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize, stackReadouts)) {
	psError(psErrorCodeLast(), false, "Input stack is not valid.");
	psFree(stackReadouts);
	return false;
    }
    psFree(stackReadouts);

    // make sure the output readout matches the inputs, and set to blank by default
    pmReadoutUpdateSize(combined, minInputCols, minInputRows, xSize, ySize, true, true, blankMaskBits);
    if (expmaps) {
	// if we are generating the expmaps, update to match the images, set blank mask areas to 0
	pmReadoutUpdateSize(expmaps, minInputCols, minInputRows, xSize, ySize, expmaps->mask != NULL, expmaps->variance != NULL, 0);
    }
   
    psVector *pixelData      = psVectorAlloc(stackData->n, PS_TYPE_F32);
    psVector *pixelVariances = psVectorAlloc(stackData->n, PS_TYPE_F32);

    // if we are asking for the exptime maps, generate a storage vector expTime
    psVector *expTime        = expmaps && expmaps->image ? psVectorAlloc(stackData->n, PS_TYPE_F32) : NULL;

    int nGoodBits[16]; // accumulate the good pixel bits here for fuzzy logic
    psAssert (sizeof(psImageMaskType) == 2, "psImageMaskType is not the expected size");

    for (int y = minInputRows; y < maxInputRows; y++) {
	for (int x = minInputCols; x < maxInputCols; x++) {

	    int nGood = 0; 
	    memset (nGoodBits, 0, 16*sizeof(int));
	    for (int i = 0; i < stackData->n; i++) {

		pmStackData *data = stackData->data[i]; // Stack data for this input
		if (!data) continue;
		pmReadout *ro = data->readout;  // Readout of interest
		if (!ro) continue;

		psAssert (ro->mask,     "must must exist, but does not");
		psAssert (ro->variance, "variance must exist, but does not");

		psImage *image    = ro->image;
		psImage *variance = ro->variance;
		psImage *mask     = ro->mask;

		int xIn = x - data->readout->col0;
		int yIn = y - data->readout->row0; // Coordinates on input readout

		// skip obviously bad input data
		if (!isfinite(image->data.F32[yIn][xIn])) continue;
		if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & badMaskBits) continue;
		if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & suspectMaskBits) continue;

		// count the number of times a given mask bit is set in the input pixels.
		// NOTE: since we have explicitly skipped the pixels with any bad bits, these are only
		// the suspect bits (nGoodBits is a bit of a misnomer: it is more like 'nSuspectBitsForGoodInputs'
		// NOTE: skip the full bit-by-bit check if we know the mask byte is empty
		psImageMaskType value = 0x0001;
		for (int nbit = 0; mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] && (nbit < 16); nbit ++) {
		    if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & value) {
			nGoodBits[nbit] ++;
		    }
		    value <<= 1;
		}

		// accumulate pixel data and variance values:
		pixelData->data.F32[nGood] = image->data.F32[yIn][xIn];
		pixelVariances->data.F32[nGood] = variance->data.F32[yIn][xIn];

		// accumulate exposure times if required
		if (expTime)  { expTime->data.F32[nGood] = data->exp; }
		nGood ++;
	    }

	    if (nGood < nminpix) ESCAPE;
	    
	    pixelData->n = nGood;

	    // sort pixelData, pixelVariance, expTime (if it exists)
	    SORT_VALUES (pixelData->n);

	    // we now have a sorted vector of values.  We can make a very coarse outlier
	    // cut based on the median and interquartile range.  This will let us reject
	    // some extreme outliers that bias the signal otherwise.

	    int Nlo = 0;     
	    int Nhi = nGood; 

	    if (nGood >= 5) {
	      int midPoint = nGood / 2;
	      float rawMedian = (nGood % 2) ? pixelData->data.F32[midPoint] : 0.5*(pixelData->data.F32[midPoint] + pixelData->data.F32[midPoint-1]);
	      
	      // XXX measure interquartile range
	      int P25 = 0.25*nGood;
	      int P75 = 0.75*nGood;
	      float rawSigma = 0.74*(pixelData->data.F32[P75] - pixelData->data.F32[P25]);
	      float minThresh = rawMedian - 5.0*rawSigma;
	      float maxThresh = rawMedian + 5.0*rawSigma;

	      // find the entries which are in the range
	      // these should be safe since minThresh and maxThresh are guaranteed to contain the median
	      while (pixelData->data.F32[Nlo    ] < minThresh) { Nlo ++; }
	      while (pixelData->data.F32[Nhi - 1] > maxThresh) { Nhi --; }
	    }

	    // if we did not clip above (either nGood < 5 or no rejection), Nlo will be 0, Nhi will be nGood
	    // In either case, Nlo is the offset of the first unclipped point.
	    int nGoodClip = Nhi - Nlo;

	    int isTest = true;
	    isTest = isTest || ((x == 4743) && (y == 2903));
	    isTest = isTest || ((x == 4953) && (y == 2919));
	    isTest = isTest || ((x == 4751) && (y == 2725));
	    isTest = false;

	    if (isTest) {
	      char testname[256];
	      snprintf (testname, 256, "testpix.%04d.%04d.txt", (int) x, (int) y);
	      FILE *f = fopen (testname, "w");
	      int fd = fileno (f);

	      fprintf (f, "# nGood: %d, Nlo: %d, Nhi: %d, nGoodClip: %d\n", nGood, Nlo, Nhi, nGoodClip);
	      p_psVectorPrint (fd, pixelData, "pixelData");
	      fclose (f);
	    }
	    // rather than define a min and max value,
	    // what we really want is a symmetric selection about the middle,
	    // or the output is biased.  If N % 2 = 1, then 
	    
	    // we are going to exclude rejectFraction*nGood measurements.  But the
	    // rejection needs to be symmetric

	    // 'AT_LEAST' means we reject at least 'rejectFraction' of values, but it could
	    // be a higher percentage for smaller numbers of inputs.
# define AT_LEAST 0

# if (AT_LEAST)
	    int Ns = MIN(MAX(1, 0.5*rejectFraction * nGoodClip), nGoodClip) + Nlo;
	    int Ne = nGoodClip - Ns + Nlo;
	    int Npt = Ne - Ns;
# else
	    int Ns = MIN(MAX(0, 0.5*rejectFraction * nGoodClip), nGoodClip) + Nlo;
	    int Ne = nGoodClip - Ns + Nlo;
	    int Npt = Ne - Ns;
# endif
	    if ((Npt < 1) || (nGood < nminpix)) ESCAPE;

	    // Set the given (suspect) mask bit if nGoodBits[i] > f*nGood in other words
	    // if more than 65% of the good inputs had one of these bits set, then we
	    // should set that bit in the output mask.  Note that this analysis counts the
	    // mask bits of pixels rejected by the clipping above.
	    psImageMaskType value = 0x0001;
	    psImageMaskType outputMask = 0x0000;
	    for (int nbit = 0; nbit < 16; nbit ++) {
		if (nGoodBits[nbit] > SUSPECT_FRACTION*nGood) {
		    outputMask |= value;
		}
		value <<= 1;
	    }

	    float sum = 0.0;
	    float varSum = 0.0;
	    for (int n = Ns; n < Ne; n++) {
		sum += pixelData->data.F32[n];
		varSum += pixelVariances->data.F32[n];
	    }
	    float mean = sum / (float) Npt;
	    float varValue = varSum / (float) (nGoodClip*nGoodClip);

	    // alternative: calculate the stdev of the pixel values
	    // float varSum = 0.0;
	    // for (int n = Ns; n < Ne; n++) {
	    // 	varSum += SQ(pixelData->data.F32[n] - mean);
	    // }
	    // variance on the mean (stdev / sqrt(N))^2

	    // the reported variance values can be extremely high / wrong.
	    // if we have enough measurements, let's just use the interquartile range
	    // of the data to estimate the per-pixel variance.  NOTE: this is not valid
	    // if the inputs have been significantly smoothed.  In that case we need
	    // to include the covariance explicitly.  But this algorithm should be used
	    // without convolution.
	    // XXX How do we choose the cutoff here?
	    if (nGoodClip >= 9) {
	      // Measure interquartile range
	      int P25 = 0.25*nGoodClip + Nlo;
	      int P75 = 0.75*nGoodClip + Nlo;
	      float rawSigma = 0.74*(pixelData->data.F32[P75] - pixelData->data.F32[P25]);
	      varValue = PS_MIN(9e7, PS_SQR(rawSigma) / (float) nGoodClip); // sigma_mean = sigma_meas / sqrt(Nmeas) -> var_mean = var_meas / Nmeas
	      // XXX the upper limit of 9e7 is set to match the output
	      // format (STK_UNIONS) which can only represent values up to that limit.
	      // perhaps it would be better to saturate the output image in psFits
	      // rather than here.  
	    }

	    // Note: since we are calculating the average of a subset of a sorted
	    // list of values, the denominator should not be the number of measurements
	    // in the calculation above (Npt): in the extreme case of a median, we would
	    // have a single value (Npt = 1), but the variance of a median is only ~1.4 x
	    // the variance of the average / sqrt(N).  We should use nGood (the total number
	    // of values in the sorted list), but the variance should be scaled by a factor
	    // which depends on the fraction of values included.  

	    // this coefficient varies between 1.4 (for pure median) and 1.05 for 68% range.

	    combined->image->data.F32[y][x] = mean;
	    combined->variance->data.F32[y][x] = varValue;
	    combined->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = outputMask;

	    // The exposure time of interest should be the total number of values, after
	    // rejection of known bad measurements, not the sorted and clipped number.
	    // Note that if we were to take the median, the relevant exposure time would
	    // still be the total of all inputs, not the single exposure for which the
	    // median was generated.

	    if (expTime) { 
		float sum = 0.0;
		for (int n = Nlo; n < Nhi; n++) {
		    sum += expTime->data.F32[n];
		}
		expmaps->image->data.F32[y][x] = sum;
	    }
	    if (expmaps) { expmaps->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = nGoodClip; }
	}
    }
    psFree(pixelData);
    psFree(pixelVariances);
    psFree(expTime);

    return (true);
}

/// Stack input images
bool pmStackCombine(
    pmReadout *combined,		// output stacked readout
    pmReadout *expmaps,			// output exposure map information
    psArray *input,			// input exposures
    psImageMaskType badMaskBits, 	// treat these bits as 'bad'
    psImageMaskType suspectMaskBits,	// treat these bits as 'suspect'
    psImageMaskType blankMaskBits,      // use this mask value for pixels missing input data (distinguish between Ninput = 0 and Ngood = 0?)
    int kernelSize,
    float iter, 
    float rej, 
    float sys, 
    float olympic,
    bool useVariance, 
    bool safe, 
    int nminpix,
    bool rejection)
{
    bool haveVariances;                 // Do we have the variance maps?
    int num;                            // Number of inputs
    int numCols, numRows;               // Size of (sky) images
    if (!validateInputData(&haveVariances, &num, &numCols, &numRows, input, combined, expmaps)) {
        return false;
    }
    PS_ASSERT_INT_NONNEGATIVE(kernelSize, false);
    PS_ASSERT_INT_POSITIVE(blankMaskBits, false);
    if (isnan(rej)) {
        PS_ASSERT_FLOAT_EQUAL(iter, 0, false);
    } else {
        PS_ASSERT_FLOAT_LARGER_THAN(iter, 0, false);
        PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);
    }
    if (useVariance && !haveVariances) {
        psWarning("Unable to use variance in rejection if no variance maps supplied --- option turned off");
        useVariance = false;
    }

    psVector *addVariance = psVectorAlloc(num, PS_TYPE_F32); // Additional variance for each image
    psVector *weights = psVectorAlloc(num, PS_TYPE_F32); // Relative weighting for each image
    psVector *exps = psVectorAlloc(num, PS_TYPE_F32);    // Exposure times for each image
    psArray *stack = psArrayAlloc(num); // Stack of readouts
    float totalExpWeight = 0.0;           // Total value of all weighted exposure times
    float totalExp = 0.0;                 // Total exposure time
    for (int i = 0; i < num; i++) {
        pmStackData *data = input->data[i]; // Stack data for this input
        if (!data) {
            weights->data.F32[i] = 0.0;
            exps->data.F32[i] = NAN;
            continue;
        }
        weights->data.F32[i] = data->weight;
        exps->data.F32[i] = data->exp;
        totalExp += exps->data.F32[i];
        totalExpWeight += exps->data.F32[i] * weights->data.F32[i];
        pmReadout *ro = data->readout;  // Readout of interest
        stack->data[i] = psMemIncrRefCounter(ro);
        addVariance->data.F32[i] = ro->covariance ? psImageCovarianceFactor(ro->covariance) : 1.0;
#ifdef ADD_VARIANCE
        if (isfinite(data->addVariance)) {
            addVariance->data.F32[i] *= data->addVariance;
        }
#endif
        if (!rejection) {
            // Ensure pixels can be put on the appropriate list
            if (!data->inspect) {
                data->inspect = psPixelsAllocEmpty(PIXEL_LIST_BUFFER);
            }
            if (!data->reject) {
                data->reject = psPixelsAllocEmpty(PIXEL_LIST_BUFFER);
            }
        }
    }
    totalExpWeight = totalExp / totalExpWeight;    // Convert to inverse

    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize,
                                stack)) {
        psError(psErrorCodeLast(), false, "Input stack is not valid.");
        psFree(stack);
        return false;
    }
    psFree(stack);
    pmReadoutUpdateSize(combined, minInputCols, minInputRows, xSize, ySize, true, true, blankMaskBits);
    psTrace("psModules.imcombine", 1, "Have for combination [%d:%d,%d:%d] (%dx%d)\n",
            minInputCols, maxInputCols, minInputRows, maxInputRows, xSize, ySize);

    // Reduce combination area by the size of the kernel
    minInputCols += kernelSize;
    maxInputCols -= kernelSize;
    minInputRows += kernelSize;
    maxInputRows -= kernelSize;
    psTrace("psModules.imcombine", 1, "Combining on [%d:%d,%d:%d]\n",
            minInputCols, maxInputCols, minInputRows, maxInputRows);


    // Buffer for combination
    combineBuffer *buffer = combineBufferAlloc(num);

    // Pull the products out, allocate if necessary
    psImage *combinedImage = combined->image; // Combined image
    if (!combinedImage) {
        combined->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        combinedImage = combined->image;
    }
    psImage *combinedMask = combined->mask; // Combined mask
    if (!combinedMask) {
        combined->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
        combinedMask = combined->mask;
    }

    psImage *combinedVariance = combined->variance; // Combined variance map
    if (haveVariances && !combinedVariance) {
        combined->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        combinedVariance = combined->variance;
    }

    psImage *exp = NULL, *expnum = NULL, *expweight = NULL; // Exposure map and exposure number
    if (expmaps) {
        if (!expmaps->image) {
            expmaps->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        }
        exp = expmaps->image;

        if (!expmaps->mask) {
            expmaps->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
        }
        expnum = expmaps->mask;

        if (!expmaps->variance) {
            expmaps->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        }
        expweight = expmaps->variance;
    }
   
    // Set up rejection list
    psArray *pixelMap = NULL;           // Map of pixels to source
    if (rejection) {
        pixelMap = pixelMapGenerate(input, minInputCols, maxInputCols, minInputRows, maxInputRows);
    }

    // Combine each pixel
    for (int y = minInputRows; y < maxInputRows; y++) {
        for (int x = minInputCols; x < maxInputCols; x++) {
	    // this is only for TESTING:
	    CHECKPIX(x, y, "Combining pixel %d,%d: %x %x %f %f %f %f %d %d %d\n", x, y, badMaskBits, blankMaskBits, iter, rej, sys, olympic, useVariance, safe, rejection);

            psVector *reject = NULL; // Images to reject for this pixel
            if (rejection) {
                reject = pixelMapQuery(pixelMap, minInputCols, minInputRows, x, y);
#ifdef TESTING
                if (PS_SQR(x - TEST_X) + PS_SQR(y - TEST_Y) <= PS_SQR(TEST_RADIUS)) {
                    fprintf(stderr, "Rejected inputs for pixel %d,%d: ", x, y);
                    if (!reject) {
                        fprintf(stderr, "<none>\n");
                    } else {
                        for (int i = 0; i < reject->n; i++) {
                            fprintf(stderr, "%d ", reject->data.U16[i]);
                        }
                        fprintf(stderr, "\n");
                    }
                }
#endif
            }
 
            int num;			  // Number of good pixels
            bool suspect;		  // Suspect pixels in stack?
	    psImageMaskType badMask = 0;  // OR of mask bits in all bad input pixels
	    psImageMaskType goodMask = 0; // OR of mask bits in all good input pixels
            combineExtract(&num, &suspect, &badMask, &goodMask, buffer, combinedImage, combinedMask, combinedVariance, input, weights, exps, addVariance, reject, x, y, badMaskBits, suspectMaskBits);
            combinePixels(combinedImage, combinedMask, combinedVariance, exp, expnum, expweight, num, buffer, x, y, blankMaskBits, badMask, goodMask, safe, nminpix, totalExpWeight);

            if (iter > 0) {
                combineTest(num, suspect, input, buffer, x, y, iter, rej, sys, olympic,
                            useVariance, safe);
            }
        }
    }

    psFree(pixelMap);
    psFree(weights);
    psFree(buffer);
    psFree(addVariance);


#ifndef PS_NO_TRACE
    if (!rejection && psTraceGetLevel("psModules.imcombine") >= 5) {
        for (int i = 0; i < num; i++) {
            pmStackData *data = input->data[i]; // Stack data for this input
            if (!data || !data->inspect) {
                continue;
            }
            psTrace("psModules.imcombine", 5, "Image %d: %ld pixels to inspect.\n", i, data->inspect->n);
        }
    }
#endif

    return true;
}

