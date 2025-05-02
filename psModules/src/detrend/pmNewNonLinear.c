#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmNewNonLinear.h"

// model is stored as a FITS table, read into an array
// each row is one knot, with (X, Y, dY2)
bool pmNewNonLinearityApply(pmReadout *inputReadout, psArray *table)
{
    PS_ASSERT_PTR_NON_NULL(table, false);
    PS_ASSERT_PTR_NON_NULL(inputReadout, false);
    PS_ASSERT_PTR_NON_NULL(inputReadout->image, false);
    PS_ASSERT_IMAGE_TYPE(inputReadout->image, PS_TYPE_F32, false);

    psTimerStart ("nonlinear");

    psImage *image = inputReadout->image;

    // create an empty 1D spline with table->n knots
    psSpline1D *model = psSpline1DCreate(table->n);
    
    // parse the table entries
    for (int i = 0; i < table->n; i++) {
	psMetadata *row = table->data[i];
	
	bool status;
	model->xKnots[i]   = psMetadataLookupF32(&status, row, "X_KNOT");
	model->yKnots[i]   = psMetadataLookupF32(&status, row, "Y_KNOT");
	model->d2yKnots[i] = psMetadataLookupF32(&status, row, "DY2_DX");
    }

    // set equal spacing info?
    // psSpline1DisEqualSpacing (model);

    for (int i = 0; i < image->numRows; i++) { // Loop over rows : note: problem with discontinuity here
	for (int j = 0; j < image->numCols; j++) { // Loop over columns
	    // Calculate correction factor contribution for this pixel.
	    psF32 flux = image->data.F32[i][j];
	    psF32 lFlux = (flux > 1.0) ? log10(flux) : 0.0; // do not introduce NANs for negative flux
	    
	    psF32 factor = psSpline1DEval (model, lFlux);
	    
	    // Apply correction to image data
	    image->data.F32[i][j] = flux * factor;

	}
    }
    psLogMsg ("psModules", PS_LOG_MINUTIA, "apply correction: %f sec\n", psTimerMark ("nonlinear"));

    psFree (model);
  
    return(true);
}

