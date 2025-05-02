/** @file ppSubReadoutForcedphot.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
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

// run forced photometry on the image in 'targetName' at the positions of the sources in 'sourceName'
// this function is only called if (data->forcedPhot1 || data->forcedPhot2) (ppSubLoop.c:186,194)
bool ppSubReadoutForcedPhot(const char *outputName, const char *targetName, const char *sourceName, ppSubData *data)
{
    bool foundDetections = false;
    bool mdok = false;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    // select the view of interest
    pmFPAview *view = ppSubViewReadout(); // View to readout

    pmFPAfile *sourceFile = psMetadataLookupPtr(&mdok, config->files, sourceName); // positive image sources
    pmFPAfile *targetFile = psMetadataLookupPtr(&mdok, config->files, targetName); // diff image pixels
    pmFPAfile *outputFile = psMetadataLookupPtr(&mdok, config->files, outputName); // result file

    psAssert (sourceFile, "failed to construct sourceName %s", sourceName);
    psAssert (targetFile, "failed to construct targetName %s", targetName);
    psAssert (outputFile, "failed to construct outputName %s", outputName);

    // copy the image data to PSPHOT.INPUT so that psphotReadoutForcedKnownSources does
    // not affect the prior results on targetName (ie, diff detections)
    pmFPAfile *photFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.INPUT"); // Photometry file
    if (!pmFPACopy(photFile->fpa, targetFile->fpa)) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to copy FPA for photometry");
        psFree(view);
        return false;
    }

    // select the reference position sources from sourceName
    pmReadout *sourceRO = pmFPAviewThisReadout(view, sourceFile->fpa);
    psAssert(sourceRO, "programming error: source readout not defined");

    pmDetections *sourceDet = psMetadataLookupPtr(NULL, sourceRO->analysis, "PSPHOT.DETECTIONS"); // Sources
    if (!sourceDet) {
	// XXX remove the pixels from photFile?
	// XXX other cleanup operations?
        psFree(view);
	return true;
    }
    psArray *sources = sourceDet->allSources;
    psAssert (sources, "missing sources?");

    // grab the PSF information from the pmFPAfile PSPHOT.PSF.LOAD
    pmFPAfile *psfFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.PSF.LOAD"); // PSF file
    ppSubCopyPSF (photFile, psfFile, view);

    // psphotSaveImage (photFile->fpa->hdu->header, photRO->image, "findsrc.im.fits");
    // psphotSaveImage (photFile->fpa->hdu->header, photRO->variance, "findsrc.wt.fits");
    // psphotSaveImage (photFile->fpa->hdu->header, photRO->mask, "findsrc.mk.fits");

    // erase the overlays from a previous psphot-related step
    if (pmVisualIsVisual()) {
        //      psphotVisualEraseOverlays (1, "all");
    }

    if (!psphotReadoutForcedKnownSources(config, view, "PSPHOT.INPUT", sources)) {
        // This is likely a data quality issue
        // XXX Split into multiple cases using error codes?
        psErrorStackPrint(stderr, "Unable to perform photometry on image");
        psWarning("Unable to perform photometry on image --- suspect bad data quality.");
        ppSubDataQuality(data, psErrorCodeLast(), PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
    }
    // save the outputs on PPSUB.POS1.SOURCES
    if (!psphotCopyResults (&foundDetections, outputFile, photFile, view)) {
	psError(PPSUB_ERR_PROG, false, "Unable to copy psphot outputs");
        psFree(view);
	return false;
    }

    float newTime = psTimerClear("PPSUB_PHOT"); // Time for photometry
    float oldTime = psMetadataLookupF32(&mdok, data->stats, "TIME_PHOT"); // Previous time for photometry
    float elapsed = isfinite(oldTime) ? oldTime + newTime : newTime;
    psMetadataAddF32(data->stats, PS_LIST_TAIL, "TIME_PHOT", PS_META_REPLACE, "Time to do photometry", elapsed);

    psFree(view);
    return true;
}
