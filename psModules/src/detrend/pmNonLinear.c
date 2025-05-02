#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmNonLinear.h"

psF32 pmNonLinearityMeasureNoisy(psF32 flux, psVector *correction_fluxes, psVector *correction_factors);
pmReadout *pmNonLinearityPolynomial(pmReadout *inputReadout, const psPolynomial1D *input1DPoly)
{
    PS_ASSERT_PTR_NON_NULL(inputReadout, NULL);
    PS_ASSERT_PTR_NON_NULL(inputReadout->image, NULL);
    PS_ASSERT_IMAGE_TYPE(inputReadout->image, PS_TYPE_F32, NULL);
    PS_ASSERT_PTR_NON_NULL(input1DPoly, NULL);

    psImage *image = inputReadout->image; // Image to correct
    for (int i = 0; i < image->numRows; i++) {
        for (int j = 0; j < image->numCols; j++) {
            image->data.F32[i][j] = psPolynomial1DEval(input1DPoly, image->data.F32[i][j]);
        }
    }
    return inputReadout;
}

// set the bin closest to the corresponding value.  
#define PS_BIN_FOR_VALUE(RESULT, VECTOR, VALUE) {			\
       	psVectorBinaryDisectResult result;				\
       	psScalar tmpScalar;						\
       	tmpScalar.type.type = PS_TYPE_F32;				\
	tmpScalar.data.F32 = (VALUE);					\
	RESULT = psVectorBinaryDisect (&result, VECTOR, &tmpScalar);	\
	switch (result) {						\
	  case PS_BINARY_DISECT_PASS:					\
            break;							\
	  case PS_BINARY_DISECT_OUTSIDE_RANGE:				\
            numPixels ++;						\
	    break;							\
	  case PS_BINARY_DISECT_INVALID_INPUT:				\
	  case PS_BINARY_DISECT_INVALID_TYPE:				\
	    psAbort ("programming error");				\
	    break;							\
        } }


# define PS_BIN_INTERPOLATE(RESULT, VECTOR, BOUNDS, BIN, VALUE) {	\
	float dX, dY, Xo, Yo, Xt;					\
	if (BIN == BOUNDS->n - 1) {					\
	    dX = 0.5*(BOUNDS->data.F32[BIN+1] - BOUNDS->data.F32[BIN-1]); \
	    dY = VECTOR->data.F32[BIN] - VECTOR->data.F32[BIN-1];	\
	    Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]);	\
	    Yo = VECTOR->data.F32[BIN];					\
	} else {							\
	    dX = 0.5*(BOUNDS->data.F32[BIN+2] - BOUNDS->data.F32[BIN]);	\
	    dY = VECTOR->data.F32[BIN+1] - VECTOR->data.F32[BIN];	\
	    Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]);	\
	    Yo = VECTOR->data.F32[BIN];					\
	}								\
	if (dY != 0) {							\
	    Xt = (VALUE - Yo)*dX/dY + Xo;				\
	} else {							\
	    Xt = Xo;							\
	}								\
	Xt = PS_MIN (BOUNDS->data.F32[BIN+1], PS_MAX(BOUNDS->data.F32[BIN], Xt)); \
	psTrace("pmNonLinear", 6, "(Xo, Yo, dX, dY, Xt, Yt) is (%.2f %.2f %.2f %.2f %.2f %.2f)\n", \
		Xo, Yo, dX, dY, Xt, VALUE);				\
	RESULT = Xt; }

pmReadout *pmNonLinearityLookup(pmReadout *inputReadout, const psVector *inFlux, const psVector *outFlux)
{
    PS_ASSERT_PTR_NON_NULL(inputReadout, NULL);
    PS_ASSERT_PTR_NON_NULL(inputReadout->image, NULL);
    PS_ASSERT_IMAGE_TYPE(inputReadout->image, PS_TYPE_F32, NULL);
    PS_ASSERT_PTR_NON_NULL(inFlux, NULL);
    if (inFlux->n < 2) {
        psError(PS_ERR_UNKNOWN, true,
                "pmNonLinearityLookup(): input vector less than 2 elements.  Returning inputReadout image.");
        return(inputReadout);
    }
    PS_ASSERT_PTR_NON_NULL(outFlux,NULL);
    // XXX unused psS32 tableSize = inFlux->n;
    if (inFlux->n != outFlux->n) {
        // XXX unused tableSize = PS_MIN(inFlux->n, outFlux->n);
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: pmNonLinear.c: pmNonLinearityLookup(): "
                 "input vectors have different sizes (%ld, %ld)\n",
                 inFlux->n, outFlux->n);
    }
    PS_ASSERT_VECTOR_TYPE(inFlux, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(outFlux, PS_TYPE_F32, NULL);

    psImage *image = inputReadout->image; // Input image
    int numPixels = 0;                  // Number of pixels outside the range
    int binNum;

    for (int i = 0; i < image->numRows; i++) {
        for (int j = 0; j < image->numCols; j++) {
	    float value = image->data.F32[i][j];
            PS_BIN_FOR_VALUE(binNum, inFlux, value);

	    // Perform linear interpolation.
	    // XXX this will result in non-sensical results if inFlux contains equal-value
	    // bins.  either enforce d(inFlux)/d(binNum) > 0 or see psStats.c PS_BIN_INTERPOLATE
	    float slope = 
		(outFlux->data.F32[binNum + 1] - outFlux->data.F32[binNum]) /
		(inFlux->data.F32[binNum + 1] - inFlux->data.F32[binNum]);
	    image->data.F32[i][j] = slope*(value - inFlux->data.F32[binNum]) + outFlux->data.F32[binNum];
        }
    }
    if (numPixels > 0) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: pmNonLinear.c: pmNonLinearityLookup(): %d pixels outside table.", numPixels);
    }
    return inputReadout;
}

bool pmNonLinearityApply(pmReadout *inputReadout, psArray *Ltab)
{
    PS_ASSERT_PTR_NON_NULL(inputReadout, false);
    PS_ASSERT_PTR_NON_NULL(inputReadout->image, false);
    PS_ASSERT_IMAGE_TYPE(inputReadout->image, PS_TYPE_F32, false);
    PS_ASSERT_PTR_NON_NULL(Ltab, false);

    psTimerStart ("nonlinear");

    psS32 numSamples = 39;
    psS32 numBorder  = 10;

    //  psS32 tableSize = Ltab->n;

    psImage *image = inputReadout->image;

    // Load default data.
    psVector *default_row_correction_fluxes = psVectorAlloc(numSamples,PS_TYPE_F32);
    psVector *default_col_correction_fluxes = psVectorAlloc(numSamples,PS_TYPE_F32);

    psVector *default_row_correction_factors = psVectorAlloc(numSamples,PS_TYPE_F32);
    psVector *default_col_correction_factors = psVectorAlloc(numSamples,PS_TYPE_F32);

    // pre-allocate the correction vectors
    psVector *row_correction_fluxes  = NULL;
    psVector *col_correction_fluxes  = NULL;
    psVector *row_correction_factors = NULL;
    psVector *col_correction_factors = NULL;

    int n = 0;
    int m = 0;
    for (int k = 0; k < Ltab->n; k++) { // Begin load default tables
	psMetadata *row = Ltab->data[k];	
	if (psMetadataLookupS32(NULL,row,"POSITION") != -1) {
	    continue;
	}
	if (psMetadataLookupS32(NULL,row,"DIRECTION") == 0) {
	    psVectorSet(default_row_correction_fluxes,n,psMetadataLookupF32(NULL,row,"FLUX"));
	    psVectorSet(default_row_correction_factors,n,psMetadataLookupF32(NULL,row,"FACTOR"));
	    n++;
	}
	else {
	    psVectorSet(default_col_correction_fluxes,m,psMetadataLookupF32(NULL,row,"FLUX"));
	    psVectorSet(default_col_correction_factors,m,psMetadataLookupF32(NULL,row,"FACTOR"));
	    m++;
	}
    } // End load default tables
  
    psLogMsg ("psModules", PS_LOG_MINUTIA, "load default data from table: %f sec\n", psTimerMark ("nonlinear"));

    // pre-allocate arrays with the correction vectors for the borders
    psArray *x_lo_flux = psArrayAlloc(numBorder);
    psArray *x_hi_flux = psArrayAlloc(numBorder);
    psArray *y_lo_flux = psArrayAlloc(numBorder);
    psArray *y_hi_flux = psArrayAlloc(numBorder);

    psArray *x_lo_fact = psArrayAlloc(numBorder);
    psArray *x_hi_fact = psArrayAlloc(numBorder);
    psArray *y_lo_fact = psArrayAlloc(numBorder);
    psArray *y_hi_fact = psArrayAlloc(numBorder);
    for (int i = 0; i < numBorder; i++) {
	// pre-allocate the correction vectors
	x_lo_flux->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	x_hi_flux->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	y_lo_flux->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	y_hi_flux->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);

	x_lo_fact->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	x_hi_fact->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	y_lo_fact->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
	y_hi_fact->data[i] = psVectorAllocEmpty(numSamples,PS_TYPE_F32);
    }	

    // parse out the full table:
    for (int k = 0; k < Ltab->n; k++) {
	psMetadata *row = Ltab->data[k];	
	int dir = psMetadataLookupS32(NULL,row,"DIRECTION");
	int pos = psMetadataLookupS32(NULL,row,"POSITION");

	psVector *fluxVector = NULL;
	psVector *factVector = NULL;

	int seq = -1;
	if ((dir == 0) && (pos < image->numCols/2)) {
	  seq = pos ;
	    if (seq < 0) continue;
	    fluxVector = x_lo_flux->data[seq];
	    factVector = x_lo_fact->data[seq];
	} 
	if ((dir == 0) && (pos > image->numCols/2)) {
	    seq = pos + numBorder - image->numCols;
	    if (seq >= image->numCols) continue;
	    fluxVector = x_hi_flux->data[seq];
	    factVector = x_hi_fact->data[seq];
	}
	if ((dir == 1) && (pos < image->numRows/2)) {
	    seq = pos;
	    if (seq < 0) continue;
	    fluxVector = y_lo_flux->data[seq];
	    factVector = y_lo_fact->data[seq];
	} 
	if ((dir == 1) && (pos > image->numRows/2)) {
	    seq = pos + numBorder - image->numRows;
	    if (seq >= image->numRows) continue;
	    fluxVector = y_hi_flux->data[seq];
	    factVector = y_hi_fact->data[seq];
	}

	float flux = psMetadataLookupF32(NULL,row,"FLUX");
	float factor = psMetadataLookupF32(NULL,row,"FACTOR");

	psVectorAppend(fluxVector, flux);
	psVectorAppend(factVector, factor);
    }
    psLogMsg ("psModules", PS_LOG_MINUTIA, "load border data from table: %f sec\n", psTimerMark ("nonlinear"));

    for (int i = 0; i < image->numRows; i++) { // Loop over rows : note: problem with discontinuity here
	row_correction_fluxes = NULL;
	if (i < numBorder) {
	    row_correction_fluxes  = y_lo_flux->data[i];
	    row_correction_factors = y_lo_fact->data[i];
	}
	if (i > image->numRows - numBorder) {
	    row_correction_fluxes  = y_hi_flux->data[i + numBorder - image->numRows];
	    row_correction_factors = y_hi_fact->data[i + numBorder - image->numRows];
	}
	if (row_correction_fluxes == NULL) {
	  row_correction_factors = default_row_correction_factors;
	    row_correction_fluxes = default_row_correction_fluxes;
	}

	for (int j = 0; j < image->numCols; j++) { // Loop over columns
	    col_correction_fluxes = NULL;
	    if (j < numBorder) {
		col_correction_fluxes  = x_lo_flux->data[j];
		col_correction_factors = x_lo_fact->data[j];
	    }
	    if (j > image->numCols - numBorder) {
		col_correction_fluxes  = x_hi_flux->data[j + numBorder - image->numCols];
		col_correction_factors = x_hi_fact->data[j + numBorder - image->numCols];
	    }
	    if (col_correction_fluxes == NULL) {
	      col_correction_factors = default_col_correction_factors;
	      col_correction_fluxes = default_col_correction_fluxes;
	    }

	    // Calculate correction factor contribution for this pixel.
	    psF32 factor_row = pmNonLinearityMeasure(image->data.F32[i][j], row_correction_fluxes,row_correction_factors);
	    psF32 factor_col = pmNonLinearityMeasure(image->data.F32[i][j], col_correction_fluxes,col_correction_factors);
#if (0)
	    if (((i == 200)&&(j == 200))||((i == 9)&&(j == 5))) { // Print out if we're looking at a test case.
	      psF32 factor_row = pmNonLinearityMeasureNoisy(image->data.F32[i][j], row_correction_fluxes,row_correction_factors);
	      psF32 factor_col = pmNonLinearityMeasureNoisy(image->data.F32[i][j], col_correction_fluxes,col_correction_factors);
	      
		psTrace("psModules.nonlin",6,"Linearity: %d %d %s %f %f %f %d %d\n",i,j,
			psMetadataLookupStr(NULL,inputReadout->parent->concepts,"CELL.NAME"),
			image->data.F32[i][j],factor_row,factor_col,numBorder,numSamples);
		psTrace("psModules.nonlin",6,"Linearity: R: %d %d %d C: %d %d %d\n",
			i,(i < numBorder),(image->numRows - i),
			j,(j < numBorder),(image->numCols - j));

		psTrace("psModules.nonlin",6,"Linearity: V: ");
		for (int k = 0; k < numSamples; k++) {
		    psTrace("psModules.nonlin",6,"(%f %f) (%f %f) DDDD> (%f %f) (%f %f)",
			    col_correction_fluxes->data.F32[k],col_correction_factors->data.F32[k],
			    row_correction_fluxes->data.F32[k],row_correction_factors->data.F32[k],
			    default_col_correction_fluxes->data.F32[k],default_col_correction_factors->data.F32[k],
			    default_row_correction_fluxes->data.F32[k],default_row_correction_factors->data.F32[k]);
		}
		psTrace("psModules.nonlin",6,"\n");
	    } // End Test case
#endif
	    // Apply correction to image data
#if (0)
	    if (((i == 200)&&(j == 200))||((i == 9)&&(j == 5))) { // Print out if we're looking at a test case.
	      psTrace("psModules.nonlin",4,"Applied Linearity Correction: %s %d %d : %f %f -> %f\n",
		      psMetadataLookupStr(NULL,inputReadout->parent->concepts,"CELL.NAME"),
		      i,j,image->data.F32[i][j],(factor_row + factor_col) / 2.0,
		      image->data.F32[i][j] + (factor_row + factor_col) / 2.0);
	    }
# endif
	    image->data.F32[i][j] = image->data.F32[i][j] - ( factor_row + factor_col ) / 2.0;

	} // End loop over columns
    } // End loop over rows
    psLogMsg ("psModules", PS_LOG_MINUTIA, "apply correction: %f sec\n", psTimerMark ("nonlinear"));

    psFree(x_lo_flux);
    psFree(x_hi_flux);
    psFree(y_lo_flux);
    psFree(y_hi_flux);

    psFree(x_lo_fact);
    psFree(x_hi_fact);
    psFree(y_lo_fact);
    psFree(y_hi_fact);

    psFree(default_row_correction_fluxes);
    psFree(default_row_correction_factors);
    psFree(default_col_correction_fluxes);
    psFree(default_col_correction_factors);
  
    return(true);
}

psF32 pmNonLinearityMeasure(psF32 flux, psVector *correction_fluxes, psVector *correction_factors) {
    //  psS32 numPixels = 0;
    psF32 result = 0;
    psU32 bin = 0;

    bin = correction_fluxes->n - 1;
    psTrace("psModules.nonlin",6,"NLMN: %f %d %f %f\n",flux,bin,correction_fluxes->data.F32[0],correction_fluxes->data.F32[bin]);
    if (bin < 0) { /* warn? */ }
    if (flux < correction_fluxes->data.F32[0]) {
	return(0.0);
    }
  
    for (int i = 0; i < correction_fluxes->n - 1; i++) {
	if ((flux >= correction_fluxes->data.F32[i])&&
	    (flux <  correction_fluxes->data.F32[i+1])) {
	    result = correction_factors->data.F32[i] +
		(flux - correction_fluxes->data.F32[i]) *
		((correction_factors->data.F32[i+1] - correction_factors->data.F32[i]) /
		 (correction_fluxes->data.F32[i+1] - correction_fluxes->data.F32[i]));
	    continue;
	}
    }

    if (!isfinite(result)) {
	result = 0.0;
    }
    return(result);
}

psF32 pmNonLinearityMeasureNoisy(psF32 flux, psVector *correction_fluxes, psVector *correction_factors) {
    //  psS32 numPixels = 0;
    psF32 result = 0;
    psU32 bin = 0;

    bin = correction_fluxes->n - 1;
    psTrace("psModules.nonlin",6,"NLMN: %f %d %f %f\n",flux,bin,correction_fluxes->data.F32[0],correction_fluxes->data.F32[bin]);
    if (bin < 0) { /* warn? */ }
    if (flux < correction_fluxes->data.F32[0]) {
	return(0.0);
    }

    for (int i = 0; i < correction_fluxes->n - 1; i++) {
      psTrace("psModules.nonlin",6,"NLMN: %f %d %f %f %f %f\n",flux,i,correction_fluxes->data.F32[i],correction_fluxes->data.F32[i],
	      correction_factors->data.F32[i],correction_factors->data.F32[i+1]);
	if ((flux >= correction_fluxes->data.F32[i])&&
	    (flux <  correction_fluxes->data.F32[i+1])) {
	    result = correction_factors->data.F32[i] +
		(flux - correction_fluxes->data.F32[i]) *
		((correction_factors->data.F32[i+1] - correction_factors->data.F32[i]) /
		 (correction_fluxes->data.F32[i+1] - correction_fluxes->data.F32[i]));
	    continue;
	}
    }

    if (!isfinite(result)) {
	result = 0.0;
    }
    return(result);
}
