#include "ppStack.h"

#define WCS_TOLERANCE 0.001             // Tolerance for WCS

bool ppStackUpdateHeader(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config) {

    pmReadout *outRO = options->outRO;                                      // Output readout
    pmReadout *expRO = options->expRO;

    // Propagate WCS
    bool wcsDone = false;           // Have we done the WCS?
    for (int i = 0; i < options->num && !wcsDone; i++) {
      if ((options->inputMask)&&(options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i])) {
            continue;
        }

        ppStackThread *thread = stack->threads->data[0]; // Representative stack
        pmReadout *inRO = thread->readouts->data[i]; // Template readout
        if (inRO && !wcsDone) {
            // Copy astrometry over
            wcsDone = true;
            pmHDU *inHDU = pmHDUFromCell(inRO->parent); // Template HDU
            pmHDU *outHDU = pmHDUFromCell(outRO->parent); // Output HDU
            pmChip *outChip = outRO->parent->parent; // Output chip
            pmFPA *outFPA = outChip->parent; // Output FPA
            if (!outHDU || !inHDU) {
                psWarning("Unable to find HDU at FPA level to copy astrometry.");
            } else {
                if (!pmAstromReadWCS(outFPA, outChip, inHDU->header, 1.0)) {
                    psErrorClear();
                    psWarning("Unable to read WCS astrometry from input FPA.");
                    wcsDone = false;
                } else {
                    if (!outHDU->header) {
                        outHDU->header = psMetadataAlloc();
                    }
                    if (!pmAstromWriteWCS(outHDU->header, outFPA, outChip, WCS_TOLERANCE)) {
                        psErrorClear();
                        psWarning("Unable to write WCS astrometry to output FPA.");
                        wcsDone = false;
                    }
                }
            }
        }
    }

    // Put version information into the header
    pmHDU *hdu = pmHDUFromCell(outRO->parent);
    if (!hdu) {
        psError(PPSTACK_ERR_PROG, false, "Unable to find HDU for output.");
        return false;
    }
    if (!hdu->header) {
        hdu->header = psMetadataAlloc();
    }
    ppStackVersionHeader(hdu->header);
    
    // other interesting header info
    psMetadataAddS32(hdu->header, PS_LIST_TAIL, "NINPUTS", PS_META_REPLACE, "Number of input images", options->num);

    psString stacktype = psMetadataLookupStr(NULL, config->arguments, "STACK_TYPE"); // NIGHTLY, DEEP, BEST_IQ
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "STK_TYPE", PS_META_REPLACE, "type of stack", stacktype);

    psString stackID = psMetadataLookupStr(NULL, config->arguments, "-stack_id"); // stack ID (eg, 123222)
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "STK_ID", PS_META_REPLACE, "type of stack", stackID);

    psString skycellID = psMetadataLookupStr(NULL, config->arguments, "-skycell_id"); // skycell ID (eg, skycell.101.00)
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "SKYCELL", PS_META_REPLACE, "type of stack", skycellID);

    psString tessID = psMetadataLookupStr(NULL, config->arguments, "-tess_id"); // tessellation ID (eg, RINGS.V0)
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "TESS_ID", PS_META_REPLACE, "type of stack", tessID);

    psMetadataAddF32(hdu->header, PS_LIST_TAIL, "AIRM_SLP", PS_META_REPLACE, "airmass slope", options->airmassSlope);
    

    // write the following fields to the output headers:
    // INP_NNNN : input file names
    // SCL_NNNN : scale factor for each input file
    // ZPT_NNNN : zero point of input file 
    // EXT_NNNN : exptime of input file
    // AIR_NNNN : airmass of input file

    // use separate loops so they are blocked together in the headers (more readable)

    // save the input filenames
    for (int i = 0; i < options->num; i++) {
	char field[64];
	snprintf (field, 64, "INP_%04d", i);

	psString basename = psStringFileBasename(options->origImages->data[i]);
	psMetadataAddStr(hdu->header, PS_LIST_TAIL, field, PS_META_REPLACE, "input image name", basename);
	psFree(basename);
    }	

    // save the input normalizations
    for (int i = 0; i < options->num; i++) {
	char field[64];

	float value = options->norm ? pow(10.0, -0.4*options->norm->data.F32[i]) : NAN;
	if ((options->inputMask)&&(options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i])) {
	  value = 0.0; // If this is NAN, then it's recorded in the header as a character string instead of a float.
	}
	snprintf (field, 64, "SCL_%04d", i);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, field, PS_META_REPLACE, "input image scale factor", value);
    }	
    // save the input exptimes
    for (int i = 0; i < options->num; i++) {
	char field[64];

	float value = options->zpInput ? options->zpInput->data.F32[i] : NAN;
	snprintf (field, 64, "ZPT_%04d", i);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, field, PS_META_REPLACE, "input image zero point", value);
    }	
    // save the input normalizations
    for (int i = 0; i < options->num; i++) {
	char field[64];

	float value = options->expTimeInput ? options->expTimeInput->data.F32[i] : NAN;
	snprintf (field, 64, "EXP_%04d", i);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, field, PS_META_REPLACE, "input image exptime", value);
    }	
    // save the input normalizations
    for (int i = 0; i < options->num; i++) {
	char field[64];

	float value = options->airmassInput ? options->airmassInput->data.F32[i] : NAN;
	snprintf (field, 64, "AIR_%04d", i);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, field, PS_META_REPLACE, "input image airmass", value);
    }

    // Copy information into expRO, because it should be there too.
    if ((expRO)&&(expRO->parent)) {
      pmHDU *expROhdu = pmHDUFromCell(expRO->parent);
      if (!expROhdu->header) {
	expROhdu->header = psMetadataAlloc();
      }

      expRO->parent->parent->parent->hdu->header = psMetadataCopy(expRO->parent->parent->parent->hdu->header,
								  outRO->parent->parent->parent->hdu->header);
      
      expRO->parent->concepts = psMetadataCopy(expRO->parent->concepts,
					       outRO->parent->concepts);
      expRO->parent->parent->concepts = psMetadataCopy(expRO->parent->parent->concepts,
						       outRO->parent->concepts);
      expRO->parent->parent->parent->concepts = psMetadataCopy(expRO->parent->parent->parent->concepts,
							       outRO->parent->parent->parent->concepts);
    }
    
    return true;
}
