#include "ppImage.h"

bool ppImageDetrendNewNonLinear(pmReadout *input, pmFPAview *detview, pmConfig  *config) {

    bool status;

    pmFPAfile *linearity_file = psMetadataLookupPtr(&status, config->files, "PPIMAGE.NEWNONLIN");
    psFits *linearity_fits = linearity_file->fits;

    char *extname = psMetadataLookupStr(&status, input->parent->concepts, "CELL.NAME");
    if (!extname) {
	psError(PS_ERR_IO, false, "missing CELL.NAME in concepts");
	return(false);
    }

    // if pmFPAfile has been loaded (by ppImageDefineFile in ppImageParseCamera), then
    // the file corresponding to the current chip is found and opened
    // NOTE: if the extname is missing, we skip the correction
    if (!psFitsMoveExtName(linearity_fits, extname)) {
        psLogMsg ("ppImageDetrendNewNonLinear", 4, "Unable to move to non-linearity (v2023) table %s, skipping", extname);
	return(true);
    }
  
    psArray *table = psFitsReadTable(linearity_fits);
    if (!table) {
	psError(PS_ERR_IO, false, "Unable to read non-linearity table.\n");
	return(false);
    }

    if (!pmNewNonLinearityApply(input,table)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to apply non-linearity corrections.\n");
	psFree (table);
	return(false);
    }	    
    psFree (table);

    return true;
}

// the new non-linearity correction is a set of splines, one per cell, with the
// xKnots equal to the log10(DN) in the pixel, and the spline value at a point
// equal to a fractional multiplier (i.e., 1 + dF)
