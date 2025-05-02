#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <string.h>
#include <strings.h>

#include "psPolynomialMD.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPARead.h"
#include "pmFPAWrite.h"
#include "pmReadoutStack.h"
#include "pmDetrendThreads.h"
#include "pmErrorCodes.h"
#include "pmDark.h"

#define PM_DARK_FITS_EXTNAME "PS_DARK"  // FITS extension name for ordinates table
#define PM_DARK_FITS_NAME    "NAME"     // Column name for concept name in ordinates table
#define PM_DARK_FITS_RULE    "RULE"     // Column name for concept rule in ordinates table
#define PM_DARK_FITS_ORDER   "ORDER"    // Column name for polynomial order in ordinates table
#define PM_DARK_FITS_SCALE   "SCALE"    // Column name for scaling option in ordinates table
#define PM_DARK_FITS_MIN     "MIN"      // Column name for minimum value in ordinates table
#define PM_DARK_FITS_MAX     "MAX"      // Column name for maximum value in ordinates table

static bool ordinateParseConcept(double *value, const pmReadout *readout, const char *name) {

    *value = NAN;

    pmCell *cell = readout->parent; // Parent cell
    psAssert(cell, "readout is missing cell \n");

    psMetadataItem *item = psMetadataLookup(cell->concepts, name);
    if (!item) {
        pmChip *chip = cell->parent; // Parent chip
        psAssert(chip, "cell is missing chip \n");

        item = psMetadataLookup(chip->concepts, name);
        if (!item) {
            pmFPA *fpa = chip->parent; // Parent FPA
            psAssert(fpa, "chip is missing fpa \n");

            item = psMetadataLookup(fpa->concepts, name);
            if (!item) {
                psError(PM_ERR_CONFIG, true, "Unable to find concept %s in readout", name);
                return false;
            }
        }
    }

    *value = psMetadataItemParseF64(item); // Value of interest
    if (!isfinite(*value)) {
        psWarning("Non-finite value (%f) of concept %s in readout", *value, name);
    }
    return true;
}

static bool ordinateParseRule(double *value, const pmReadout *readout, const char *name, const char *rule) {

    psAssert(name, "ordinate name is not defined");
    psAssert(rule, "ordinate rule is not defined");

    *value = NAN;

    psArray *words = psStringSplitArray(rule, " ", false);

    // we should have a rule of the form (concept) OP (concept) OP (concept) ...
    // for now, the only allowed OP is * (eventually, we can steal code from opihi for a better
    // RPN parser).

    if (words->n % 2 == 0) {
        psError(PM_ERR_CONFIG, true, "syntax error in DARK.ORDINATE %s rule %s\n", name, rule);
        psFree(words);
        return false;
    }

    for (int i = 1; i < words->n; i+=2) {
        if (strcmp((char *)words->data[i], "*")) {
            psError(PM_ERR_CONFIG, true, "syntax error in DARK.ORDINATE %s rule %s\n", name, rule);
            psFree(words);
            return false;
        }
    }

    if (!ordinateParseConcept(value, readout, words->data[0])) {
        psError(PM_ERR_CONFIG, false, "syntax error in DARK.ORDINATE %s rule %s\n", name, rule);
        psFree(words);
        return false;
    }

    double value2 = 0.0;
    for (int i = 2; i < words->n; i+=2) {
        if (!ordinateParseConcept(&value2, readout, words->data[i])) {
            psError(PM_ERR_CONFIG, false, "syntax error in DARK.ORDINATE %s rule %s\n", name, rule);
            psFree(words);
            return false;
        }
        *value *= value2;
    }
    psFree(words);
    return true;
}

// Look up the value of an ordinate in a readout
static bool ordinateLookup(double *value, // Value of ordinate, to return
                           bool *inRange, // is value within min : max range?
                           const char *name, // Name of ordinate (concept or abstract name)
                           const char *rule, // Rule for generating the value (if NULL use name as concept)
                           bool scale,  // Scale the value?
                           float min, float max, // Minimum and maximum values for scaling
                           const pmReadout *readout // Readout of interest
                           )
{
    assert(value);
    assert(name);
    assert(readout);

    *inRange = true;

    if (rule) {
        if (!ordinateParseRule(value, readout, name, rule)) {
            psError(PM_ERR_CONFIG, false, "trouble parsing rule %s for DARK.ORDINATE %s", rule, name);
            return false;
        }
    } else {
        if (!ordinateParseConcept(value, readout, name)) {
            psError(PM_ERR_CONFIG, false, "trouble parsing rule %s for DARK.ORDINATE %s", "NULL", name);
            return false;
        }
    }

    if (scale) {
        if (*value < min || *value > max) {
            psWarning("Value of concept %s (%f) outside range (%f:%f)", name, *value, min, max);
            *inRange = false;
        }
        *value = 2.0 * (*value - min) / (max - min) - 1.0;
    }

    return true;
}

static void darkOrdinateFree(pmDarkOrdinate *ord)
{
    psFree(ord->name);
    psFree(ord->rule);
    return;
}

pmDarkOrdinate *pmDarkOrdinateAlloc(const char *name, int order)
{
    pmDarkOrdinate *ord = psAlloc(sizeof(pmDarkOrdinate)); // Ordinate data, to return
    psMemSetDeallocator(ord, (psFreeFunc)darkOrdinateFree);

    ord->name = psStringCopy(name);
    ord->rule = NULL;
    ord->order = order;
    ord->scale = false;
    ord->min = NAN;
    ord->max = NAN;

    return ord;
}

// this creates and saves: values, roMask, norm, orders, counts, sigma, and saves the on output->analysis
bool pmDarkCombinePrepare(pmCell *output, const psArray *inputs, psArray *ordinates, const char *normConcept)
{
    psArray *values = psArrayAlloc(inputs->n);
    psVector *roMask = psVectorAlloc(inputs->n, PS_TYPE_VECTOR_MASK); // Mask for bad readouts
    psVector *norm = normConcept ? psVectorAlloc(inputs->n, PS_TYPE_F32) : NULL; // Normalizations for each
    psVector *orders = psVectorAlloc(ordinates->n, PS_TYPE_U8); // Orders for each concept

    psVectorInit(roMask, 0);

    bool inRange = false;
    int numBadInputs = 0;               // Number of bad inputs

    // build the 'norm' vector and the 'values' vectors, count the number of bad inputs
    for (int i = 0; i < inputs->n; i++) {
        values->data[i] = psVectorAlloc(ordinates->n, PS_TYPE_F32);
        if (!norm) continue;

        pmReadout *readout = inputs->data[i]; // Readout of interest
        double normValue;            // Normalisation value
        if (!ordinateLookup(&normValue, &inRange, normConcept, NULL, false, NAN, NAN, readout)) {
            psError(PM_ERR_CONFIG, false, "problem finding concept %s for DARK.NORM", normConcept);
            return false;
        }
        if (!isfinite(normValue)) {
            psWarning("Unable to find acceptable value of %s for readout %d", normConcept, i);
            roMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
            norm->data.F32[i] = NAN;
            numBadInputs++;
            continue;
        }
        if (normValue == 0.0) {
            psWarning("Normalisation value (%s) for readout %d is zero", normConcept, i);
            roMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
            norm->data.F32[i] = NAN;
            numBadInputs++;
            continue;
        }
        norm->data.F32[i] = 1.0 / normValue;
    }

    // build the 'orders' vector and set the array of 'values'
    for (int i = 0; i < ordinates->n; i++) {
        pmDarkOrdinate *ord = ordinates->data[i]; // Ordinate information
        if (ord->order <= 0) {
            psError(PS_ERR_UNKNOWN, true, "Bad order for DARK.ORDINATE %s (%d) --- ignored", ord->name, ord->order);
            psFree(values);
            psFree(roMask);
            psFree(orders);
            psFree(norm);
            return false;
        }
        orders->data.U8[i] = ord->order;

        for (int j = 0; j < inputs->n; j++) {
            psVector *val = values->data[j]; // Value vector for readout
            if (roMask->data.PS_TYPE_VECTOR_MASK_DATA[j]) {
                val->data.F32[i] = NAN;
                continue;
            }

            pmReadout *readout = inputs->data[j]; // Readout of interest
            double value = NAN;          // Value of ordinate
            if (!ordinateLookup(&value, &inRange, ord->name, ord->rule, ord->scale, ord->min, ord->max, readout)) {
                psError(PM_ERR_CONFIG, false, "problem finding rule for DARK.ORDINATE %s", ord->name);
                return false;
            }
            if (!isfinite(value)) {
                psWarning("Unable to find acceptable value of DARK.ORDINATE %s for readout %d", ord->name, i);
                roMask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0xff;
                val->data.F32[i] = NAN;
                numBadInputs++;
                continue;
            }
            if (!inRange) {
                psWarning("Value of DARK.ORDINATE %s for readout %d is out of range", ord->name, i);
                roMask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0xff;
                val->data.F32[i] = NAN;
                numBadInputs++;
                continue;
            }
            val->data.F32[i] = value;
        }
    }

    if (psTraceGetLevel("psModules.detrend") > 9) {
        for (int i = 0; i < inputs->n; i++) {
            psVector *val = values->data[i];
            (void) val; // avoid unused variable message when tracing is compiled out
            for (int j = 0; j < ordinates->n; j++) {
                psTrace("psModules.detrend", 9, "Image %d, ordinate %d: %f\n", i, j, val->data.F32[j]);
            }
        }
    }

    int numTerms = 1;                   // Number of terms in polynomial
    for (int i = 0; i < orders->n; i++) {
        numTerms += orders->data.U8[i];
    }

    if (numTerms > inputs->n - numBadInputs) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Insufficient inputs (%ld) to fit polynomial terms (%d).",
                inputs->n - numBadInputs, numTerms);
        psFree(values);
        psFree(roMask);
        psFree(norm);
        return false;
    }

    // determine the output image size based on the input images
    int row0, col0, numCols, numRows;
    if (!pmReadoutStackSetOutputSize(&col0, &row0, &numCols, &numRows, inputs)) {
        psError(PS_ERR_UNKNOWN, false, "problem setting output readout size.");
        return false;
    }

    // the output is potentially a cube (depending on the dimensionality of the fit)
    if (output->readouts->n != numTerms) {
        output->readouts = psArrayRealloc(output->readouts, numTerms);
    }

    // generate the required output images based on the specified sizes
    for (int i = 0; i < numTerms; i++) {
        pmReadout *readout = output->readouts->data[i]; // Readout to update
        if (!readout) {
            readout = output->readouts->data[i] = pmReadoutAlloc(output);
        }

        pmReadoutStackDefineOutput(readout, col0, row0, numCols, numRows, false, false, 0);
        psTrace("psModules.imcombine", 7, "Output minimum: %d,%d\n", col0, row0);
    }

    // these calls allocate and save the requested images on the output analysis metadata
    psImage *counts = pmReadoutSetAnalysisImage(output->readouts->data[0], PM_READOUT_STACK_ANALYSIS_COUNT,
                                                numCols, numRows, PS_TYPE_U16, 0);
    if (!counts) {
        return false;
    }
    psImage *sigma = pmReadoutSetAnalysisImage(output->readouts->data[0], PM_READOUT_STACK_ANALYSIS_SIGMA,
                                               numCols, numRows, PS_TYPE_F32, NAN);
    if (!sigma) {
        return false;
    }

    psMetadataAddPtr(output->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_ORDINATES,
                     PS_DATA_ARRAY | PS_META_REPLACE, "Dark ordinates", ordinates);
    psMetadataAddStr(output->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_NORM, PS_META_REPLACE,
                     "Dark normalisation", normConcept);

    psMetadataAddPtr(output->analysis, PS_LIST_TAIL, "DARK.VALUES",  PS_DATA_ARRAY  | PS_META_REPLACE,
                     "Dark values", values);
    psMetadataAddPtr(output->analysis, PS_LIST_TAIL, "DARK.RO.MASK", PS_DATA_VECTOR | PS_META_REPLACE,
                     "Dark Readout Mask", roMask);
    psMetadataAddPtr(output->analysis, PS_LIST_TAIL, "DARK.NORM",    PS_DATA_VECTOR | PS_META_REPLACE,
                     "Dark norm", norm);
    psMetadataAddPtr(output->analysis, PS_LIST_TAIL, "DARK.ORDERS",  PS_DATA_VECTOR | PS_META_REPLACE,
                     "Dark orders", orders);

    for (int i = 0; i < numTerms; i++) {
        pmReadout *readout = output->readouts->data[i]; // Readout to update
        readout->data_exists = true;
    }
    output->data_exists = true;
    output->parent->data_exists = true;

    psFree(norm);
    psFree(roMask);
    psFree(orders);
    psFree(values);

    return true;
}

// do the combine work for this portion of the output (range is set by input data)
bool pmDarkCombine(pmCell *output, const psArray *inputs, int iter, float rej, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(output, false);
    PS_ASSERT_PTR_NON_NULL(output->readouts, false);
    PS_ASSERT_INT_NONNEGATIVE(output->readouts->n, false);
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_INT_NONNEGATIVE(iter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);

    bool mdok = false;

    // retrieve the required parameter vectors
    psArray *in_values  = psMetadataLookupPtr(&mdok, output->analysis, "DARK.VALUES");
    psAssert(in_values, "values not supplied");
    psVector *roMask = psMetadataLookupPtr(&mdok, output->analysis, "DARK.RO.MASK");
    psAssert(roMask, "roMask not supplied");
    psVector *max_orders = psMetadataLookupPtr(&mdok, output->analysis, "DARK.ORDERS");
    psAssert(max_orders, "orders not supplied");

    psArray *values_set = psArrayAlloc(max_orders->n);
    psArray *poly_set = psArrayAlloc(max_orders->n);
    psVector *logL = psVectorAlloc(max_orders->n,PS_TYPE_F64);

    for (int i = 0; i < max_orders->n; i++) {
      psVector *orders = psVectorAlloc(i+1,PS_TYPE_U8);
      for (int j = 0; j < orders->n; j++) {
	orders->data.U8[j] = max_orders->data.U8[j];
      }
      poly_set->data[i] =  psPolynomialMDAlloc(orders); // Polynomial for fitting
      
      psArray *values = psArrayAlloc(in_values->n);
      
      for (int j = 0; j < values->n; j++) {
	psVector *these_values = psVectorAlloc(i+1,PS_TYPE_F32);
	psVector *input_values = in_values->data[j];

	for (int k = 0; k < orders->n; k++) {
	  these_values->data.F32[k] = input_values->data.F32[k];
	}
	values->data[j] = these_values;
      }
      values_set->data[i] = values;
      psFree(orders);
    }
      
    // retrieve the norm vector, if supplied
    psVector *norm       = psMetadataLookupPtr(&mdok, output->analysis, "DARK.NORM");

    // retrieve the 'counts' and 'sigma' images
    psImage *counts = pmReadoutGetAnalysisImage(output->readouts->data[0], PM_READOUT_STACK_ANALYSIS_COUNT);
    if (!counts) {
        return false;
    }
    psImage *sigma = pmReadoutGetAnalysisImage(output->readouts->data[0], PM_READOUT_STACK_ANALYSIS_SIGMA);
    if (!sigma) {
        return false;
    }

    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize,
                                inputs)) {
        psError(PS_ERR_UNKNOWN, false, "No valid input readouts.");
        return false;
    }

    pmDarkVisualInit(values_set->data[max_orders->n - 1]);

    pmReadout *outReadout = output->readouts->data[0];

    // Iterate over pixels, fitting polynomial
    psVector *pixels = psVectorAlloc(inputs->n, PS_TYPE_F32); // Stack of pixels
    psVector *mask   = psVectorAlloc(inputs->n, PS_TYPE_VECTOR_MASK); // Mask for stack
    for (int i = minInputRows; i < maxInputRows; i++) {
        int yOut = i - outReadout->row0; // y position on output readout

#ifdef SHOW_BUSY
        if (psTraceGetLevel("psModules.detrend") > 9) {
            printf("Processing row %d\r", i);
            fflush(stdout);
        }
#endif

        for (int j = minInputCols; j < maxInputCols; j++) {
            int xOut = j - outReadout->col0; // x position on output readout

            psVectorInit(mask, 0);
            for (int r = 0; r < inputs->n; r++) {
                if (roMask->data.PS_TYPE_VECTOR_MASK_DATA[r]) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[r] = 0xff;
                    continue;
                }
                pmReadout *readout = inputs->data[r]; // Input readout
		if ((!readout)||(!readout->image)) {
		  mask->data.PS_TYPE_VECTOR_MASK_DATA[r] = 0xff;
		  continue;
		}		  
                int yIn = i - readout->row0; // y position on input readout
                int xIn = j - readout->col0; // x position on input readout

                pixels->data.F32[r] = readout->image->data.F32[yIn][xIn];
                if (norm) {
                    pixels->data.F32[r] *= norm->data.F32[r];
                }
                if (readout->mask) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[r] = (readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & maskVal);
                }

            }

	    int k_best = 0;
	    for (int k = 0; k < max_orders->n; k++) {
	      psPolynomialMD *poly = poly_set->data[k];
	      psArray *values = values_set->data[k];
	      
	      if (!psPolynomialMDClipFit(poly, pixels, NULL, mask, 0xff, values, iter, rej)) {
                psErrorClear();         // Nothing we can do about it
                psVectorInit(poly->coeff, NAN);
	      }

	      pmDarkVisualPixelFit(pixels, mask);
	      pmDarkVisualPixelModel(poly, values);

	      // Insert math here to choose optimum model.
	      logL->data.F64[k] = 0.0;
	      psPolynomialMD *polySig = poly_set->data[0];
	      for (int m = 0; m < poly->deviations->n; m++) {
		logL->data.F64[k] += pow(poly->deviations->data.F32[m] / polySig->stdevFit,2);
/* 		if ((xOut == 20) && (yOut == 256)) { */
/* 		  psTrace("psModules.detrend",3,"pmDarkCombine DEV: %d %d: input %d models: Norders: %d logL: %g value: %g\n", */
/* 			  xOut,yOut,m,k,logL->data.F64[k],poly->deviations->data.F32[m]); */
/* 		} */
	      }
	      if (k > 0) {
		if ( ( logL->data.F64[k - 1] - logL->data.F64[k] ) > 1) { // Hard coded criterion for a ~5% limit with one degree of freedom
		  k_best = k;
		}
	      }
	      if ((xOut <= 600) && (yOut <= 600)) {
		psTrace("psModules.detrend",3,"pmDarkCombine: %d %d: models: Norders: %d logL: %g BestOrders: %d\n",
			xOut,yOut,k,logL->data.F64[k],k_best);
	      }
	    }
	    if (k_best > 1) {
	      k_best = 1;
	    }
/* 	    k_best = 1; */
	    // Select the polynomial that seems best.
	    psPolynomialMD *poly = poly_set->data[k_best];
	      
	    //            for (int k = 0; k < poly->coeff->n; k++) {
	    for (int k = 0; k < max_orders->n + 1; k++) { // There is one more coefficient than is stored here.
                pmReadout *ro = output->readouts->data[k]; // Readout of interest
		if (k < poly->coeff->n) {
		  ro->image->data.F32[yOut][xOut] = poly->coeff->data.F64[k];
		}
		else {
		  ro->image->data.F32[yOut][xOut] = 0.0;
		}
            }
            counts->data.U16[yOut][xOut] = poly->numFit;
            sigma->data.F32[yOut][xOut] = poly->stdevFit;
        }
    }

    psFree(pixels);
    psFree(mask);

    for (int i = 0; i < max_orders->n; i++) {
      psFree(poly_set->data[i]);
      psArray *values = values_set->data[i];
      for (int j = 0; j < values->n; j++) {
	psFree(values->data[j]);
      }
      psFree(values_set->data[i]);
    }
    psFree(values_set);
    psFree(poly_set);
    psFree(logL);

    return true;
}

bool pmDarkApplyScan_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmReadout *readout     = job->args->data[0]; // Readout to correct
    pmCell *dark           = job->args->data[1]; // Dark to apply
    const psVector *orders = job->args->data[2]; // Polynomial orders for each ordinate
    const psVector *values = job->args->data[3]; // Values for each ordinate

    psImageMaskType bad = PS_SCALAR_VALUE(job->args->data[4], PS_TYPE_IMAGE_MASK_DATA); // Mask value to give bad pixels
    bool doNorm    = PS_SCALAR_VALUE(job->args->data[5], U8); // Normalise values?
    float norm     = PS_SCALAR_VALUE(job->args->data[6], F32); // Value by which to normalise
    int rowStart   = PS_SCALAR_VALUE(job->args->data[7], S32); // Starting row for scan
    int rowStop    = PS_SCALAR_VALUE(job->args->data[8], S32); // Stopping row for scan

    return pmDarkApplyScan(readout, dark, orders, values, bad, doNorm, norm, rowStart, rowStop);
}

bool pmDarkApplyScan(pmReadout *readout, const pmCell *dark, const psVector *orders, const psVector *values,
                     psImageMaskType bad, bool doNorm, float norm, int rowStart, int rowStop)
{
    int numCols = readout->image->numCols;
    int numTerms = dark->readouts->n;   // Number of polynomial terms

    psPolynomialMD *poly = psPolynomialMDAlloc(orders); // Polynomial to apply

    for (int y = rowStart; y < rowStop; y++) {
        for (int x = 0; x < numCols; x++) {
            for (int i = 0; i < numTerms; i++) {
                pmReadout *ro = dark->readouts->data[i]; // Dark readout
                poly->coeff->data.F64[i] = ro->image->data.F32[y][x];
            }
            float value = psPolynomialMDEval(poly, values); // Value of dark current
            if (doNorm) {
                value *= norm;
            }
            readout->image->data.F32[y][x] -= value;
            if (readout->mask && !isfinite(value)) {
                readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = bad;
            }
        }
    }
    psFree(poly);

    return true;
}

bool pmDarkApply(pmReadout *readout, pmCell *dark, psImageMaskType bad)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(dark, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);
    PS_ASSERT_IMAGE_TYPE(readout->image, PS_TYPE_F32, false);
    int numCols = readout->image->numCols, numRows = readout->image->numRows; // Size of image
    if (readout->mask) {
        PS_ASSERT_IMAGE_NON_NULL(readout->mask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(readout->mask, readout->image, false);
        PS_ASSERT_IMAGE_TYPE(readout->mask, PS_TYPE_IMAGE_MASK, false);
    }
    int numTerms = dark->readouts->n;   // Number of polynomial terms
    for (int i = 0; i < numTerms; i++) {
        pmReadout *ro = dark->readouts->data[i]; // Dark readout
        PS_ASSERT_PTR_NON_NULL(ro, false);
        PS_ASSERT_IMAGE_NON_NULL(ro->image, false);
        PS_ASSERT_IMAGE_SIZE(ro->image, numCols, numRows, false);
        PS_ASSERT_IMAGE_TYPE(ro->image, PS_TYPE_F32, false);
    }
    psArray *ordinates = psMetadataLookupPtr(NULL, dark->analysis, PM_DARK_ANALYSIS_ORDINATES); // Ordinates
    if (!ordinates) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find dark ordinates.");
        return false;
    }
    bool mdok;                          // Status of MD lookup
    psString normConcept = psMetadataLookupStr(&mdok, dark->analysis, PM_DARK_ANALYSIS_NORM); // Normalisation
    bool inRange = false;

    int numOrdinates = ordinates->n;    // Number of ordinates
    psVector *values = psVectorAlloc(numOrdinates, PS_TYPE_F32); // Values of ordinates
    for (int i = 0; i < numOrdinates; i++) {
        pmDarkOrdinate *ord = ordinates->data[i]; // Ordinate of interest
        double value = NAN;              // Value for ordinate
        if (!ordinateLookup(&value, &inRange, ord->name, ord->rule, ord->scale, ord->min, ord->max, readout)) {
            psError(PS_ERR_UNKNOWN, true, "Unable to find value for DARK.ORDINATE %s", ord->name);
            psFree(values);
            return false;
        }
        if (!isfinite(value)) {
            psError(PS_ERR_UNKNOWN, true, "Value for DARK.ORDINATE %s is NAN", ord->name);
            psFree(values);
            return false;
        }
        values->data.F32[i] = value;
    }
    double norm = NAN;                   // Normalisation value
    bool doNorm = false;                // Do normalisation?
    if (normConcept && strlen(normConcept) > 0) {
        if (!ordinateLookup(&norm, &inRange, normConcept, NULL, false, NAN, NAN, readout)) {
            psError(PS_ERR_UNKNOWN, true, "Unable to find value for %s", normConcept);
            psFree(values);
            return false;
        }
        doNorm = true;
    }

    psVector *orders = psVectorAlloc(numOrdinates, PS_TYPE_U8); // Order for each polynomial
    for (int i = 0; i < numOrdinates; i++) {
        pmDarkOrdinate *ord = ordinates->data[i]; // Ordinate information
        if (ord->order <= 0) {
            psError(PS_ERR_UNKNOWN, true, "Bad order for DARK.ORDINATE %s (%d) --- ignored", ord->name, ord->order);
            psFree(values);
            psFree(orders);
            return false;
        }
        orders->data.U8[i] = ord->order;
    }

    // thread here by scan

    bool threaded = true;
    int scanRows = pmDetrendGetScanRows();
    if (scanRows == 0) {
        threaded = false;
        scanRows = readout->image->numRows;
    }

    for (int rowStart = 0; rowStart < readout->image->numRows; rowStart += scanRows) {
        int rowStop = PS_MIN(rowStart + scanRows, readout->image->numRows);

        if (threaded) {
            psThreadJob *job = psThreadJobAlloc("PSMODULES_DETREND_DARK");
            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, dark);
            psArrayAdd(job->args, 1, orders);
            psArrayAdd(job->args, 1, values);
            PS_ARRAY_ADD_SCALAR(job->args, bad, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, doNorm, PS_TYPE_U8);
            PS_ARRAY_ADD_SCALAR(job->args, norm, PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, rowStop, PS_TYPE_S32);

            if (!psThreadJobAddPending (job)) {
                psFree(orders);
                psFree(values);
                return false;
            }
        } else if (!pmDarkApplyScan(readout, dark, orders, values, bad, doNorm, norm, rowStart, rowStop)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to apply dark.");
            psFree(orders);
            psFree(values);
            return false;
        }
    }

    if (threaded) {
        // wait here for the threaded jobs to finish
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to apply dark.");
            psFree(orders);
            psFree(values);
            return false;
        }
    }

    psFree(orders);
    psFree(values);

    return true;
}

bool pmFPAWriteDark(pmFPA *fpa, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!pmFPAWrite(fpa, fits, config, blank, recurse)) {
        psError(PS_ERR_IO, false, "Unable to write FPA dark images");
        return false;
    }

    pmHDU *hdu = fpa->hdu;
    if (blank || !hdu) {
        // No more to do
        return true;
    }

    psArray *ordinates = NULL;      // Dark ordinates, to write
    const char *normConcept = NULL;     // Normalisation concept
    psArray *chips = fpa->chips;    // Component chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i]; // Chip of interest
        psArray *cells = chip->cells; // Component cells
        for (int j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[j]; // Cell of interest
            bool mdok;              // Status of MD lookup
            psArray *newOrd = psMetadataLookupPtr(&mdok, cell->analysis, PM_DARK_ANALYSIS_ORDINATES); // Ords
            if (!mdok) {
                continue;
            }
            psString newNorm = psMetadataLookupPtr(&mdok, cell->analysis, PM_DARK_ANALYSIS_NORM); // Norm
            if (ordinates) {
                if (newOrd != ordinates) {
                    psError(PS_ERR_UNKNOWN, true, "Dark ordinates differ across cells.");
                    return false;
                }
                if ((normConcept && (!newNorm || strcmp(normConcept, newNorm) != 0)) ||
                    (!normConcept && newNorm)) {
                    psError(PS_ERR_UNKNOWN, true, "Dark normalisations differ across cells.");
                    return false;
                }
            } else {
                ordinates = newOrd;
                normConcept = newNorm;
            }
        }
    }

    if (!ordinates) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find dark ordinates.");
        return false;
    }

    return pmDarkWrite(fits, hdu->header, ordinates, normConcept);
}

bool pmChipWriteDark(pmChip *chip, psFits *fits, pmConfig *config, bool blank, bool recurse)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!pmChipWrite(chip, fits, config, blank, recurse)) {
        psError(PS_ERR_IO, false, "Unable to write chip dark images");
        return false;
    }

    pmHDU *hdu = pmHDUFromChip(chip);
    if (blank || !hdu) {
        // No more to do
        return true;
    }

    psArray *ordinates = NULL;          // Dark ordinates, to write
    const char *normConcept = NULL;     // Normalisation concept
    psArray *cells = chip->cells;       // Component cells
    for (int j = 0; j < cells->n; j++) {
        pmCell *cell = cells->data[j]; // Cell of interest
        bool mdok;                      // Status of MD lookup
        psArray *newOrd = psMetadataLookupPtr(&mdok, cell->analysis, PM_DARK_ANALYSIS_ORDINATES); // Ordinates
        if (!mdok) {
            continue;
        }
        psString newNorm = psMetadataLookupPtr(&mdok, cell->analysis, PM_DARK_ANALYSIS_NORM); // Normalisation
        if (ordinates) {
            if (newOrd != ordinates) {
                psError(PS_ERR_UNKNOWN, true, "Dark ordinates differ across cells.");
                return false;
            }
            if ((normConcept && (!newNorm || strcmp(normConcept, newNorm) != 0)) ||
                (!normConcept && newNorm)) {
                psError(PS_ERR_UNKNOWN, true, "Dark normalisations differ across cells.");
                return false;
            }
        } else {
            ordinates = newOrd;
            normConcept = newNorm;
        }
    }

    if (!ordinates) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find dark ordinates.");
        return false;
    }

    return pmDarkWrite(fits, hdu->header, ordinates, normConcept);
}

bool pmCellWriteDark(pmCell *cell, psFits *fits, pmConfig *config, bool blank)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    // Allow the usual pmFPAWrite functions to handle the heavy lifting for the images
    if (!pmCellWrite(cell, fits, config, blank)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write dark cell.");
        return false;
    }

    pmHDU *hdu = pmHDUFromCell(cell);
    if (blank || !hdu) {
        // No more to do
        return true;
    }

    psArray *ordinates = psMetadataLookupPtr(NULL, cell->analysis, PM_DARK_ANALYSIS_ORDINATES);
    if (!ordinates) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find dark ordinates.");
        return false;
    }
    bool mdok;                          // Status of MD lookup
    psString normConcept = psMetadataLookupPtr(&mdok, cell->analysis, PM_DARK_ANALYSIS_NORM); // Normalisation

    return pmDarkWrite(fits, hdu->header, ordinates, normConcept);
}

bool pmDarkWrite(psFits *fits, psMetadata *header, const psArray *ordinates, const char *normConcept)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_ARRAY_NON_NULL(ordinates, false);

    // Only write table once per FITS file
    bool write = false;             // Write table?
    if (psFitsGetSize(fits) <= 1) {
        write = true;
    } else {
        psMetadata *headers = psFitsReadHeaderSet(NULL, fits); // FITS headers
        if (!psMetadataLookup(headers, PM_DARK_FITS_EXTNAME)) {
            write = true;
        }
        psFree(headers);
    }
    if (!write) {
        return true;
    }

    if (!psMemIncrRefCounter((psMetadata*)header)) {
        header = psMetadataAlloc();
    }
    psMetadataAddStr(header, PS_LIST_TAIL, PM_DARK_HEADER_NORM, PS_META_REPLACE,
                     "Dark normalisation concept", normConcept);

    if (ordinates->n > 0) {
        // Format ordinates into FITS table
        int numOrdinates = ordinates->n;// Number of ordinates
        psArray *table = psArrayAlloc(numOrdinates); // FITS table, constructed from ordinates
        for (int i = 0; i < ordinates->n; i++) {
            pmDarkOrdinate *ord = ordinates->data[i]; // Ordinate of interest
            psMetadata *row = psMetadataAlloc(); // FITS table row
            psMetadataAddStr(row, PS_LIST_TAIL, PM_DARK_FITS_NAME, 0, "DARK.ORDINATE name", ord->name);

            // XXX write a dummy value if ord->rule == NULL? (eg, NONE)
            if (ord->rule) {
                psMetadataAddStr(row, PS_LIST_TAIL, PM_DARK_FITS_RULE, 0, "DARK.ORDINATE rule", ord->rule);
            } else {
                psMetadataAddStr(row, PS_LIST_TAIL, PM_DARK_FITS_RULE, 0, "DARK.ORDINATE rule", "NONE");
            }

            psMetadataAddS32(row, PS_LIST_TAIL, PM_DARK_FITS_ORDER, 0, "Polynomial order", ord->order);
            psMetadataAddBool(row, PS_LIST_TAIL, PM_DARK_FITS_SCALE, 0, "Scale values?", ord->scale);
            psMetadataAddF32(row, PS_LIST_TAIL, PM_DARK_FITS_MIN, 0, "Minimum value", ord->min);
            psMetadataAddF32(row, PS_LIST_TAIL, PM_DARK_FITS_MAX, 0, "Maximum value", ord->max);
            table->data[i] = row;
        }

        if (!psFitsWriteTable(fits, header, table, PM_DARK_FITS_EXTNAME)) {
            psError(PS_ERR_IO, false, "Unable to write dark ordinates.");
            psFree(table);
            psFree(header);
            return false;
        }
        psFree(table);
    } else {
        // No ordinates to write to a table, so write to a blank header.
        if (!psFitsWriteBlank(fits, header, PM_DARK_FITS_EXTNAME)) {
            psError(PS_ERR_IO, false, "Unable to write dark header.");
            psFree(header);
            return false;
        }
    }
    psFree(header);

    return true;
}


bool pmFPAReadDark(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!pmFPARead(fpa, fits, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read dark FPA.");
        return false;
    }

    psString normConcept = NULL;        // Normalisation concept
    psArray *ordinates = pmDarkRead(&normConcept, fits); // Dark ordinates
    if (!ordinates) {
        psError(PS_ERR_IO, false, "Unable to read dark ordinates.");
        return false;
    }

    psArray *chips = fpa->chips;        // Component chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        psArray *cells = chip->cells;   // Component cells
        for (int j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[j]; // Cell of interest
            psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_ORDINATES,
                             PS_DATA_ARRAY | PS_META_REPLACE, "Dark ordinates", ordinates);
            psMetadataAddStr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_NORM, PS_META_REPLACE,
                             "Dark normalisation", normConcept);
        }
    }
    psFree(ordinates);
    psFree(normConcept);

    return true;
}

bool pmChipReadDark(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!pmChipRead(chip, fits, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read dark chip.");
        return false;
    }

    psString normConcept = NULL;        // Normalisation concept
    psArray *ordinates = pmDarkRead(&normConcept, fits); // Dark ordinates
    if (!ordinates) {
        psError(PS_ERR_IO, false, "Unable to read dark ordinates.");
        return false;
    }

    psArray *cells = chip->cells;       // Component cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_ORDINATES,
                         PS_DATA_ARRAY | PS_META_REPLACE, "Dark ordinates", ordinates);
        psMetadataAddStr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_NORM, PS_META_REPLACE,
                         "Dark normalisation", normConcept);
    }
    psFree(ordinates);
    psFree(normConcept);

    return true;
}


bool pmCellReadDark(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    // Allow the usual pmFPARead functions to handle the heavy lifting for the images
    if (!pmCellRead(cell, fits, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read dark cell.");
        return false;
    }

    psString normConcept = NULL;        // Normalisation concept
    psArray *ordinates = pmDarkRead(&normConcept, fits); // Dark ordinates
    if (!ordinates) {
        psError(PS_ERR_IO, false, "Unable to read dark ordinates.");
        return false;
    }
    psMetadataAddPtr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_ORDINATES,
                     PS_DATA_ARRAY | PS_META_REPLACE, "Dark ordinates", ordinates);
    psMetadataAddStr(cell->analysis, PS_LIST_TAIL, PM_DARK_ANALYSIS_NORM, PS_META_REPLACE,
                     "Dark normalisation", normConcept);
    psFree(ordinates);
    psFree(normConcept);

    return true;
}

psArray *pmDarkRead(psString *normConcept, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(normConcept, NULL);
    PS_ASSERT_PTR_NULL(*normConcept, NULL);
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    if (!psFitsMoveExtName(fits, PM_DARK_FITS_EXTNAME)) {
        psError(PS_ERR_IO, false, "Unable to find extension with dark ordinates table (%s).",
                PM_DARK_FITS_EXTNAME);
        return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // Header
    bool mdok;                          // Status of MD lookup
    *normConcept = psMemIncrRefCounter(psMetadataLookupStr(&mdok, header, PM_DARK_HEADER_NORM));

    psArray *ordinates = NULL;          // Dark ordinates to return

    psFitsType type = psFitsGetExtType(fits); // Type of FITS extension
    switch (type) {
      case PS_FITS_TYPE_IMAGE: {
          // Check that it's of zero size; otherwise we might have some conflict
          int numCols = psMetadataLookupS32(&mdok, header, "NAXIS1");
          int numRows = psMetadataLookupS32(&mdok, header, "NAXIS2");
          if (numCols != 0 || numRows != 0) {
              psError(PS_ERR_UNKNOWN, true, "Extension %s is not a DARK table.", PM_DARK_FITS_EXTNAME);
              psFree(header);
              return NULL;
          }
          // No ordinates fit --- only a constant term
          ordinates = psArrayAlloc(0);
          break;
      }
      case PS_FITS_TYPE_BINARY_TABLE:
      case PS_FITS_TYPE_ASCII_TABLE: {
          psArray *table = psFitsReadTable(fits); // FITS Table with ordinates
          int numOrdinates = table->n;        // Number of ordinates
          ordinates = psArrayAlloc(numOrdinates);

          for (int i = 0; i < numOrdinates; i++) {
              psMetadata *row = table->data[i]; // Row of interest
              const char *name = psMetadataLookupStr(NULL, row, PM_DARK_FITS_NAME); // Concept name
              int order = psMetadataLookupS32(NULL, row, PM_DARK_FITS_ORDER); // Polynomial order
              if (!name || order <= 0) {
                  psError(PS_ERR_UNKNOWN, false, "Bad value reading dark ordinates table.");
                  psFree(table);
                  psFree(ordinates);
                  return false;
              }
              pmDarkOrdinate *ord = pmDarkOrdinateAlloc(name, order); // Ordinate data
              ord->scale = psMetadataLookupBool(NULL, row, PM_DARK_FITS_SCALE);
              ord->min = psMetadataLookupF32(NULL, row, PM_DARK_FITS_MIN);
              ord->max = psMetadataLookupF32(NULL, row, PM_DARK_FITS_MAX);

              // load the ordinate rule; it is not an error if this field is missing or NULL
              // a NULL rule means 'use the name as the single concept'
              const char *rule = psMetadataLookupStr(&mdok, row, PM_DARK_FITS_RULE);
              if (!rule || !strcasecmp(rule, "NONE")) {
                  ord->rule = NULL;
              } else {
                  ord->rule = psStringCopy(rule);
              }
              ordinates->data[i] = ord;
          }
          psFree(table);
          break;
      }
      default:
        psError(PS_ERR_UNKNOWN, true, "Unrecognised FITS extension type.");
        return NULL;
    }
    psFree(header);

    return ordinates;
}

#if (HAVE_KAPA)
#include <kapa.h>
#include "pmKapaPlots.h"
#include "pmVisual.h"

static int nKapa = 0;
static int *kapa = NULL;
static bool plotFlag = true;
static psArray *xVectors = NULL;

// this init function only gets the ordinates for the first readout...
bool pmDarkVisualInit(psArray *values) {

    if (!pmVisualIsVisual()) return true;

    // skip if we have already opened the windows (or if none are requested...)
    if (nKapa) return true;

    // values has Ninput vectors with Norder elements; we need Norder vectors with Ninput elements...
    int nOrders = 0;
    for (int i = 0; i < values->n; i++) {
        psVector *vect = values->data[i];
        if (!nOrders) {
            nOrders = vect->n;
        } else {
            psAssert (nOrders == vect->n, "mismatch in order vector lengths");
        }
    }
    xVectors = psArrayAlloc(nOrders);
    for (int i = 0; i < nOrders; i++) {
        xVectors->data[i] = psVectorAlloc(values->n, PS_TYPE_F32);
    }

    for (int i = 0; i < values->n; i++) {
        psVector *vect = values->data[i];
        for (int j = 0; j < vect->n; j++) {
            psVector *xVec = xVectors->data[j];
            xVec->data.F32[i] = vect->data.F32[j];
        }
    }

    nKapa = nOrders;

    kapa = psAlloc(nKapa*sizeof(int));

    for (int i = 0; i < nKapa; i++) {
        kapa[i] = -1;
        pmVisualInitWindow(&kapa[i], "ppmerge");
    }
    return true;
}

bool pmDarkVisualPixelFit(psVector *pixels, psVector *mask) {

    Graphdata graphdata;

    if (!pmVisualIsVisual()) return true;

    KapaInitGraph(&graphdata);

    psAssert(nKapa == xVectors->n, "inconsistent number of orders %d vs %ld\n", nKapa, xVectors->n);

    psVector *xSub = psVectorAlloc(pixels->n, PS_TYPE_F32);
    psVector *ySub = psVectorAlloc(pixels->n, PS_TYPE_F32);

    for (int i = 0; i < xVectors->n; i++) {
        psVector *x = xVectors->data[i];

        // generate vectors of the unmasked values
        int nSub = 0;
        for (int j = 0; j < pixels->n; j++) {
            if (mask && mask->data.PS_TYPE_VECTOR_MASK_DATA[j]) continue;
            xSub->data.F32[nSub] = x->data.F32[j];
            ySub->data.F32[nSub] = pixels->data.F32[j];
            nSub ++;
        }
        xSub->n = ySub->n = nSub;

        // plot the unmasked values
        pmVisualScaleGraphdata (&graphdata, xSub, ySub, false);
        KapaSetGraphData(kapa[i], &graphdata);
        KapaSetLimits(kapa[i], &graphdata);
        KapaClearPlots (kapa[i]);

        KapaSetFont (kapa[i], "courier", 14);
        KapaBox (kapa[i], &graphdata);
        KapaSendLabel (kapa[i], "ordinate", KAPA_LABEL_XM);
        KapaSendLabel (kapa[i], "pixel values", KAPA_LABEL_YM);

        graphdata.color = KapaColorByName("black");
        graphdata.style = KAPA_PLOT_POINTS;
        graphdata.ptype = KAPA_POINT_CROSS;
        KapaPrepPlot  (kapa[i], xSub->n, &graphdata);
        KapaPlotVector(kapa[i], xSub->n, xSub->data.F32, "x");
        KapaPlotVector(kapa[i], xSub->n, ySub->data.F32, "y");
    }
    pmVisualAskUser (&plotFlag);
    return true;
}

bool pmDarkVisualPixelModel(psPolynomialMD *poly, psArray *values) {

    Graphdata graphdata;

    if (!pmVisualIsVisual()) return true;

    KapaInitGraph(&graphdata);

    psAssert(nKapa == xVectors->n, "inconsistent number of orders %d vs %ld\n", nKapa, xVectors->n);

    psVector *yFit = psVectorAlloc(values->n, PS_TYPE_F32);

    for (int i = 0; i < values->n; i++) {
        psVector *coord = values->data[i];
        yFit->data.F32[i] = psPolynomialMDEval (poly, coord);
    }

    for (int i = 0; i < xVectors->n; i++) {
        psVector *xFit = xVectors->data[i];

        KapaGetGraphData(kapa[i], &graphdata);
        graphdata.color = KapaColorByName("red");
        graphdata.style = KAPA_PLOT_POINTS;
        graphdata.ptype = KAPA_POINT_CIRCLE_OPEN;
        KapaPrepPlot  (kapa[i], xFit->n, &graphdata);
        KapaPlotVector(kapa[i], xFit->n, xFit->data.F32, "x");
        KapaPlotVector(kapa[i], xFit->n, yFit->data.F32, "y");
    }
    pmVisualAskUser (&plotFlag);
    return true;
}

bool pmDarkVisualCleanup() {

    for (int i = 0; i < nKapa; i++) {
        KapaClose(kapa[i]);
    }
    psFree (kapa);
    psFree (xVectors);
    return true;
}

# else

bool pmDarkVisualInit(psArray *values) { return true; }
bool pmDarkVisualPixelFit(psVector *pixels, psVector *mask) { return true; }
bool pmDarkVisualPixelModel(psPolynomialMD *poly, psArray *values) { return true; }
bool pmDarkVisualCleanup() { return true; }

# endif
