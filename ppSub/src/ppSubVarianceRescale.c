/** @file ppSubVarianceRescale.c
 *
 *  @brief measure the background signal/noise distribution and rescale the variance if needed
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

bool ppSubVarianceRescale(pmConfig *config, ppSubData *data)
{
    psAssert(config, "Require configuration");

    bool mdok; // Status of metadata lookups

    psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSub
    psAssert(recipe, "Need PPSUB recipe");

    if (!psMetadataLookupBool(&mdok, recipe, "RENORM")) return true;

    int num = psMetadataLookupS32(&mdok, recipe, "RENORM.NUM");
    if (!mdok) {
        psError(PPSUB_ERR_ARGUMENTS, true, "RENORM.NUM is not set in the recipe");
        return false;
    }
    float minValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MIN");
    if (!mdok) {
        psError(PPSUB_ERR_ARGUMENTS, true, "RENORM.MIN is not set in the recipe");
        return false;
    }
    float maxValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MAX");
    if (!mdok) {
        psError(PPSUB_ERR_ARGUMENTS, true, "RENORM.MAX is not set in the recipe");
        return false;
    }

    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask

    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *readout = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT"); // Output image

    if (!readout->variance) {
        // Nothing to renormalise
        psWarning("Renormalisation of the variance requested, but no variance provided.");
	psFree(view);
        return true;
    }

    if (!pmReadoutVarianceRenormalise(readout, maskBad, num, minValid, maxValid)) {
        psErrorStackPrint(stderr, "Unable to renormalise variances");
        psWarning("Unable to renormalise variances --- suspect bad data quality.");
        // Allow the convolved and subtracted images to be written
        ppSubDataQuality(data, PPSUB_ERR_VARIANCE,
                         PPSUB_FILES_INPUT | PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV |
                         PPSUB_FILES_PSF | PPSUB_FILES_PHOT);
    }

    psFree(view);
    return true;
}
