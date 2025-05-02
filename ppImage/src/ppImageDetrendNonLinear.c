#include "ppImage.h"

bool ppImageDetrendNonLinearPolynomial(pmReadout *input, psMetadataItem *dataItem) {

    // These are the polynomial coefficients
    psVector *coeff = dataItem->data.V; // The coefficient vector
    if (coeff->type.type != PS_TYPE_F64) {
        psVector *temp = psVectorCopy(NULL, coeff, PS_TYPE_F64); // F64 version
        psFree (coeff);
        coeff = temp;
    }
    psPolynomial1D *correction = psPolynomial1DAlloc(coeff->n - 1, PS_POLYNOMIAL_ORD);
    psFree(correction->coeff);
    correction->coeff = psMemIncrRefCounter(coeff->data.F64);
    pmNonLinearityPolynomial(input, correction);
    psFree(coeff);
    psFree(correction);
    return true;
}

bool ppImageDetrendNonLinearLookup(pmReadout *input, psMetadataItem *dataItem) {

    // This is a filename: lookup table
    char *name = dataItem->data.V;       // Filename
    psLookupTable *table = psLookupTableAlloc(name, "%f %f", 0);
    if (psLookupTableRead(table) <= 0) {
        psErrorStackPrint(stderr, "Unable to read non-linearity correction file "
                          "%s --- ignored\n", name);
        return false;
    }
#ifdef PRODUCTION
    pmNonLinearityLookup(input, table);
#else
    psVector *influx = table->values->data[0];
    psVector *outflux = table->values->data[1];
    pmNonLinearityLookup(input, influx, outflux);
#endif
    psFree(table);
    return true;
}


bool ppImageDetrendNonLinear(pmReadout *input, pmFPAview *detview, pmConfig  *config) {
    bool status;

    pmFPAfile *linearity_file = psMetadataLookupPtr(&status,config->files,"PPIMAGE.LINEARITY");
    psFits *linearity_fits = linearity_file->fits;

    char *extname = psMetadataLookupStr(&status,input->parent->concepts,"CELL.NAME");
    if (!extname) {
	psError(PS_ERR_IO, false, "missing CELL.NAME in concepts");
	return(false);
    }

    if (!psFitsMoveExtName(linearity_fits,extname)) {
	psError(PS_ERR_IO, false, "Unable to move to non-linearity table %s", extname);
	return(false);
    }
  
    psArray *table = psFitsReadTable(linearity_fits);
    if (!table) {
	psError(PS_ERR_IO, false, "Unable to read non-linearity table.\n");
	return(false);
    }

    // It might be better to pack lookup table here...
    // Why? I only use that lookup table once for the single cell it matches. 
  
    if (!pmNonLinearityApply(input,table)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to apply non-linearity corrections.\n");
	psFree (table);
	return(false);
    }	    
    psFree (table);

    return true;
}

bool ppImageDetrendNonLinear_Original(pmReadout *input, ppImageOptions *options) {

    psMetadataItem *concept;
    pmCell *cell = input->parent;

    switch (options->nonLinearType) {
      case PS_DATA_VECTOR:
        ppImageDetrendNonLinearPolynomial (input, options->nonLinearData);
        return true;

      case PS_DATA_STRING:
        ppImageDetrendNonLinearLookup (input, options->nonLinearData);
        return true;

      case PS_DATA_METADATA:
        // Go looking for the value in the hierarchy
        concept = psMetadataLookup(cell->concepts, options->nonLinearSource);
        if (! concept) {
            pmChip *chip = cell->parent;// Parent chip
            concept = psMetadataLookup(chip->concepts, options->nonLinearSource);
            if (! concept) {
                pmFPA *fpa = chip->parent; // Parent FPA
                concept = psMetadataLookup(fpa->concepts, options->nonLinearSource);
                if (! concept) {
                    psLogMsg("phase2", PS_LOG_WARN, "Unable to find value of concept %s "
                             "for non-linearity correction --- ignored.\n", (char *)options->nonLinearSource);
                    return false;
                }
            }
        }

        if (concept->type != PS_DATA_STRING) {
            psLogMsg("phase2", PS_LOG_WARN, "Type for concept %p isn't STRING, as"
                     " expected for non-linearity correction --- ignored.\n",
                     concept);
            return false;
        }

        // Get the value of the concept
        psString conceptValue = concept->data.V;
        psMetadata *folder = (psMetadata *)options->nonLinearData->data.V;
        psMetadataItem *optionItem = psMetadataLookup(folder, conceptValue);
        if (!optionItem) {
            psLogMsg("phase2", PS_LOG_WARN, "Unable to find %s in NONLIN.DATA"
                     " --- ignored.\n", conceptValue);
            return false;
        }

        switch (optionItem->type) {
          case PS_DATA_VECTOR:
            ppImageDetrendNonLinearPolynomial (input, optionItem);
            return true;
          case PS_DATA_STRING:
            ppImageDetrendNonLinearLookup (input, optionItem);
            return true;
          default:
            psLogMsg("phase2", PS_LOG_WARN, "Non-linearity correction "
                     "desired but unable to interpret NONLIN.DATA for %s"
                     " --- ignored\n", conceptValue);
            return false;
        }
      default:
        psAbort("Invalid options->nonLinearType");
    }
    return true;
}

