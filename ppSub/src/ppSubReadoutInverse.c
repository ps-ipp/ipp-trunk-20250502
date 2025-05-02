#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

bool ppSubReadoutInverse(pmConfig *config)
{
    psAssert(config, "Require configuration");
    // Check configuration for convolve option
    psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, PPSUB_RECIPE);
    bool noConvolve = psMetadataLookupBool(NULL, recipe, "NOCONVOLVE"); // Do not use convolved images.
    
    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *outRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");
    pmReadout *invRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INVERSE");

    invRO->image = (psImage*)psBinaryOp(invRO->image, outRO->image, "*", psScalarAlloc(-1.0, PS_TYPE_F32));
    invRO->mask = psMemIncrRefCounter(outRO->mask);
    invRO->variance = psMemIncrRefCounter(outRO->variance);
    invRO->covariance = psMemIncrRefCounter(outRO->covariance);
    invRO->analysis = psMetadataCopy(invRO->analysis, outRO->analysis);

    // MEH -- need to also clear out detections or inv.cmf is corrupted on update
    if (psMetadataLookup(invRO->analysis, "PSPHOT.DETECTIONS")) {
      psMetadataRemoveKey(invRO->analysis, "PSPHOT.DETECTIONS");
    }

    invRO->data_exists = invRO->parent->data_exists = invRO->parent->parent->data_exists = true;

    // Get concepts from reference
    pmFPAfile *refFile;
    if (noConvolve) {
	refFile = psMetadataLookupPtr(NULL, config->files, "PPSUB.REF"); // File with concepts
    }
    else {
	refFile = psMetadataLookupPtr(NULL, config->files, "PPSUB.REF.CONV"); // File with concepts
    }
    pmFPA *invFPA = invRO->parent->parent->parent; // Inverse FPA
    pmConceptsCopyFPA(invFPA, refFile->fpa, true, true);

    // Get astrometry from (forward) subtraction
    pmChip *outChip = outRO->parent->parent;       // Output chip
    pmFPA *outFPA = outChip->parent;               // Output FPA
    pmChip *invChip = invRO->parent->parent; // Inverse chip
    pmHDU *invHDU = invFPA->hdu;          // Inverse HDU
    if (!pmAstromWriteWCS(invHDU->header, outFPA, outChip, WCS_TOLERANCE)) {
	psError(psErrorCodeLast(), false, "Unable to write WCS astrometry to PPSUB.INVERSE.");
	psFree(view);
	return false;
    }
    // Read from newly written astrometry so that it exists in the "inverse" FPA (for sources)
    if (!pmAstromReadWCS(invFPA, invChip, invHDU->header, 1.0)) {
	psError(psErrorCodeLast(), false, "Unable to read WCS astrometry.");
	psFree(view);
	return false;
    }
    
    psFree(view);
    return true;
}
