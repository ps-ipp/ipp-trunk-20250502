/** @file ppSubMakePSF.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
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

psArray *ppSubSelectPSFSources(psArray *sources);

bool ppSubMakePSF(ppSubData *data)
{
    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    if (!data->photometry) {
        return true;
    }

    psTimerStart("PPSUB_PHOT");

    bool reverse = psMetadataLookupBool(NULL, config->arguments, "REVERSE"); // Reverse sense of subtraction?
    
    bool mdok = false;                  // Status of MD lookup
    pmReadout *minuend = NULL;          // Image that will be positive following subtraction
    pmFPAfile *minuendFile = NULL;      // File for minuend image
    pmFPAview *view = ppSubViewReadout(); // View to readout

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSUB_RECIPE); // Recipe for ppSub
    bool noConvolve = psMetadataLookupBool(&mdok, recipe, "NOCONVOLVE"); // Do not use convolved images.

    if (noConvolve) {
	// if we do not convolve, we need to copy the detections to the image for analysis
	pmReadout *psfSourcesRO = NULL;	// readout containing loaded sources for psf model
	psWarning("Not using Convolved images because NOCONVOLVE  is TRUE\n");
	if (reverse) {
	    minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF");
	    minuendFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.REF");
	    psfSourcesRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.SOURCES");
	} else {
	    minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT");
	    minuendFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.INPUT");
	    psfSourcesRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.SOURCES");
	}
	psAssert (psfSourcesRO, "missing readout with sources");
	pmDetections *psfDetections = psMetadataLookupPtr(&mdok, psfSourcesRO->analysis,  "PSPHOT.DETECTIONS");
	psMetadataAddPtr(minuend->analysis,  PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "Merged source list", psfDetections);
    } else {
	printf("Using Convolved images because NOCONVOLVE is FALSE\n");
	if (reverse) {
	    minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV");
	    minuendFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.REF.CONV");
	} else {
	    minuend = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV");
	    minuendFile = psMetadataLookupPtr(&mdok, config->files, "PPSUB.INPUT.CONV");
	}
    }

    pmFPAfile *photFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.INPUT"); // Photometry file
    if (!pmFPACopy(photFile->fpa, minuendFile->fpa)) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to copy FPA for photometry");
        psFree(view);
        return false;
    }

    pmReadout *photRO = pmFPAviewThisReadout(view, photFile->fpa); // Readout to photometer

    // we need to remove any existing PSPHOT.DETECTIONS (why not do this in psphot?)
    if (psMetadataLookup(photRO->analysis, "PSPHOT.DETECTIONS")) {
        psMetadataRemoveKey(photRO->analysis, "PSPHOT.DETECTIONS");
    }
    if (psMetadataLookup(photRO->parent->parent->analysis, "PSPHOT.PSF")) {
        psMetadataRemoveKey(photRO->parent->parent->analysis, "PSPHOT.PSF");
    }

# ifdef TESTING
    // XXX for testing, dump these images:
    psphotSaveImage (NULL, photRO->image, "findpsf.im.fits");
    psphotSaveImage (NULL, photRO->variance, "findpsf.wt.fits");
    psphotSaveImage (NULL, photRO->mask, "findpsf.mk.fits");
# endif

    // Extract the loaded sources from the associated readout, and generate PSF
    // Here, we assume the image is background-subtracted
    pmDetections *detections = psMetadataLookupPtr(&mdok, minuend->analysis, "PSPHOT.DETECTIONS");
    if (!detections || !detections->allSources) {
        psError(PPSUB_ERR_CONFIG, true, "No sources from which to determine PSF.");
        psFree(view);
        return false;
    }
    psArray *sources = detections->allSources;

    // XXX filter sources?  limit the total number and return only brighter objects?
    // use flags to toss totally bogus entries?
    psArray *goodSources = ppSubSelectPSFSources (sources);

    if (!psphotReadoutFindPSF(config, view, "PSPHOT.INPUT", goodSources)) {
        // This is likely a data quality issue
        // XXX Split into multiple cases using error codes?
        psErrorStackPrint(stderr, "Unable to determine PSF");
        psWarning("Unable to determine PSF --- suspect bad data quality.");
        ppSubDataQuality(data, PSPHOT_ERR_PSF, PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
        psFree(view);
        psFree(goodSources);
        return true;
    }

    // save the resulting PSF information on the pmFPAfile PSPHOT.PSF.LOAD
    pmFPAfile *psfFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.PSF.LOAD"); // PSF file
    if (!ppSubCopyPSF(psfFile, photFile, view)) {
        psErrorStackPrint(stderr, "PSF was not generated");
        psWarning("PSF was not generated --- suspect bad data quality.");
        ppSubDataQuality(data, psErrorCodeLast(), PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
    }

    // Get rid of the generated header; it will be regenerated by the real photometry run
    psMetadataRemoveKey(photRO->analysis, "PSPHOT.HEADER");

    psFree(goodSources);
    psFree(view);

    return true;
}

bool ppSubCopyPSF(pmFPAfile *output, pmFPAfile *input, pmFPAview *view)
{
    pmChip *inputChip   = pmFPAviewThisChip(view, input->fpa); // Chip with PSF info
    pmChip *outputChip  = pmFPAviewThisChip(view, output->fpa); // Chip to store PSF info

    pmReadout *inputRO  = pmFPAviewThisReadout(view, input->fpa); // Readout with PSF info
    pmReadout *outputRO = pmFPAviewThisReadout(view, output->fpa); // Readout to store PSF info

    if (!outputRO) {
        pmCell *outputCell  = pmFPAviewThisCell(view, output->fpa);
        outputRO = pmReadoutAlloc(outputCell);
        outputRO->image = psMemIncrRefCounter(inputRO->image);
	psFree(outputRO); // I have a copy on the outputCell
    }

    // Copy the PSF-related data
    psMetadataIterator *iter = psMetadataIteratorAlloc(inputRO->analysis, PS_LIST_HEAD, "^PSF.*");
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psMetadataAddItem(outputRO->analysis, item, PS_LIST_TAIL, PS_META_REPLACE);
    }
    psFree(iter);

    // copy the PSF model data
    pmPSF *psf = psMetadataLookupPtr(NULL, inputChip->analysis, "PSPHOT.PSF"); // PSF for photometry
    if (!psf) {
        psErrorStackPrint(stderr, "No PSF available");
        return false;
    }

    psMetadataAddPtr(outputChip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN | PS_META_REPLACE,
                     "PSF from ppSubMakePSF", psf);

    return true;
}


// XXX hardwired MAX for now
# define MAX_NPSF 500

psArray *ppSubSelectPSFSources(psArray *sources){

    sources = psArraySort (sources, pmSourceSortByFlux);

    psArray *subset = psArrayAllocEmpty(MAX_NPSF);

    int nPSF = 0;
    for (int i = 0; (nPSF < MAX_NPSF) && (i < sources->n); i++) {

        pmSource *source = sources->data[i];
        if (!source) continue;

        // skip non-astronomical objects (very likely defects)
        if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
        if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
        if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;

        psArrayAdd (subset, 100, source);
        nPSF++;
    }

    return subset;
}
