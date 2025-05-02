/** @file ppSubLoop.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

// Test functions defined below, if needed
bool dumpout(pmConfig *config, char *name);
bool check_psphot_input (pmConfig *config, char *name);

bool ppSubLoop(ppSubData *data)
{
    bool mdok = false;
    bool success = true;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration.");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");
    bool noConvolve = psMetadataLookupBool(&mdok, recipe, "NOCONVOLVE"); // Do not use convolved images.

    pmConfigCamerasCull(config, NULL);
    pmConfigRecipesCull(config, "PPSUB,PPSTATS,PSPHOT,PSASTRO,MASKS,JPEG");

    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PPSUB.INPUT");
    pmFPAfile *reference = psMetadataLookupPtr(NULL, config->files, "PPSUB.REF");
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PPSUB.OUTPUT");
    psAssert(input && reference && output, "Require files");

    if (!ppSubFilesIterateDown(config, PPSUB_FILES_INPUT | PPSUB_FILES_CONV)) {
        psError(PPSUB_ERR_IO, false, "Unable to load files.");
        return false;
    }

    psTimerStart("PPSUB_MATCH");

    if (!ppSubSetMasks(config)) {
        psError(psErrorCodeLast(), false, "Unable to set masks.");
        return false;
    }

    if (data->forcedPhot1) {
	bool foundDetections = false;
	if (!ppSubInputDetections(&foundDetections, "PPSUB.POS1.SOURCES", "PPSUB.INPUT", data)) {
	    psError(psErrorCodeLast(), false, "Unable to measure positive detections (1)");
	    success = false;
            goto ESCAPE;
	}
	// if nothing was found, don't bother doing the forced photometry below
	if (!foundDetections) {
	    psWarning ("no sources found in positive image 1, skipping forced photometry");
	    data->forcedPhot1 = false;
	}
    }
    if (data->forcedPhot2) {
        // Change the recipe to use a higher nsigma limit and quit after pass1
        psMetadata *psphotRecipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);

        psF32 nsigma_peak_save = psMetadataLookupF32 (NULL, psphotRecipe, "PEAKS_NSIGMA_LIMIT");
        char *breakPt_save =  psMetadataLookupStr (NULL, psphotRecipe, "BREAK_POINT");

        psF32 pos2_nsigma_peak = psMetadataLookupF32 (&mdok, psphotRecipe, "PEAKS_POS2_NSIGMA_LIMIT");
        if (!mdok) {
            psWarning("PEAKS_POS2_NSIGMA_LIMIT not found in psphotRecipe. Will use 25.\n");
            pos2_nsigma_peak = 25.;
        }
        psMetadataAddF32(psphotRecipe, PS_LIST_TAIL, "PEAKS_NSIGMA_LIMIT", PS_META_REPLACE, "", pos2_nsigma_peak);
        psMetadataAddStr(psphotRecipe, PS_LIST_TAIL, "BREAK_POINT", PS_META_REPLACE, "", "PASS1");

	bool foundDetections = false;
	if (!ppSubInputDetections(&foundDetections, "PPSUB.POS2.SOURCES", "PPSUB.REF", data)) {
	    psError(psErrorCodeLast(), false, "Unable to measure positive detections (2)");
            psMetadataAddF32(psphotRecipe, PS_LIST_TAIL, "PEAKS_NSIGMA_LIMIT", PS_META_REPLACE, "", nsigma_peak_save);
            psMetadataAddStr(psphotRecipe, PS_LIST_TAIL, "BREAK_POINT", PS_META_REPLACE, "", breakPt_save);
	    success = false;
            goto ESCAPE;
	}
        psMetadataAddF32(psphotRecipe, PS_LIST_TAIL, "PEAKS_NSIGMA_LIMIT", PS_META_REPLACE, "", nsigma_peak_save);
        psMetadataAddStr(psphotRecipe, PS_LIST_TAIL, "BREAK_POINT", PS_META_REPLACE, "", breakPt_save);
	// if nothing was found, don't bother doing the forced photometry below
	if (!foundDetections) {
	    psWarning ("no sources found in positive image 2, skipping forced photometry");
	    data->forcedPhot2 = false;
	}
    }

    // XXX if it exists, use the POS1, POS2 successs for the FWHMs
    if (!ppSubMatchPSFs(data)) {
	psError(psErrorCodeLast(), false, "Unable to match PSFs.");
	success = false;
	goto ESCAPE;
    }

    if (data->quality) {
        // Can't do anything at all
        success = false;
        goto ESCAPE;
    }
    // generate the residual stamp grid for visualization
    if (!ppSubResidualSampleJpeg(config)) {
        psError(psErrorCodeLast(), false, "Unable to update.");
        success = false;
        goto ESCAPE;
    }

    // XXX add in a positive image detection step here (if needed)
    

    psMetadataAddF32(data->stats, PS_LIST_TAIL, "TIME_MATCH", 0, "Time to match PSFs", psTimerClear("PPSUB_MATCH"));

    // Close input files (freeing up space) : for the noConvolve case, we cannot close the
    // inputs since we will use them for subtraction below.  wait until later to do the work
    if (!noConvolve) {
	if (!ppSubFilesIterateUp(config, PPSUB_FILES_INPUT)) {
	    psError(PPSUB_ERR_IO, false, "Unable to close input files.");
	    success = false;
	    goto ESCAPE;
	}
    }
    if (!ppSubLowThreshold(data)) {
        psError(psErrorCodeLast(), false, "Unable to threshold images.");
        success = false;
        goto ESCAPE;
    }

    // Set up subtraction files
    if (!ppSubFilesIterateDown(config, PPSUB_FILES_SUB)) {
        psError(PPSUB_ERR_IO, false, "Unable to set up subtraction files.");
        success = false;
        goto ESCAPE;
    }

    if (!ppSubDefineOutput("PPSUB.OUTPUT", config)) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        success = false;
        goto ESCAPE;
    }

    if (!data->quality && !ppSubMakePSF(data)) {
        psError(psErrorCodeLast(), false, "Unable to generate PSF.");
        success = false;
        goto ESCAPE;
    }

    // Now we've got a PSF, blow away detections in case they're confused with real output detections
    {
        pmFPAview *view = ppSubViewReadout(); // View to readout
        pmReadout *out = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");
        if (psMetadataLookup(out->analysis, "PSPHOT.DETECTIONS")) {
            psMetadataRemoveKey(out->analysis, "PSPHOT.DETECTIONS");
        }
        psFree(view);
    }

    if (!ppSubReadoutSubtract(config)) {
        psError(psErrorCodeLast(), false, "Unable to subtract images.");
        success = false;
        goto ESCAPE;
    }

    // Close convolved files
    if (!ppSubFilesIterateUp(config, PPSUB_FILES_PSF | PPSUB_FILES_CONV)) {
        psError(PPSUB_ERR_IO, false, "Unable to close input files.");
        success = false;
        goto ESCAPE;
    }
    if (noConvolve) {
	if (!ppSubFilesIterateUp(config, PPSUB_FILES_INPUT)) {
	    psError(PPSUB_ERR_IO, false, "Unable to close input files.");
	    success = false;
	    goto ESCAPE;
	}
    }
    
    // Higher order background subtraction using psphot
    if (!ppSubBackground(config)) {
        psError(psErrorCodeLast(), false, "Unable to subtract background.");
        success = false;
        goto ESCAPE;
    }

    // Perform Variance correction (rescale within a modest range)
    if (!ppSubVarianceRescale(config, data)) {
        psError(psErrorCodeLast(), false, "Unable to rescale variance.");
        success = false;
        goto ESCAPE;
    }

    if (data->quality) {
        // Done all we can do up to this point
        success = false;
        goto ESCAPE;
    }

    if (!ppSubFilesIterateDown(config, PPSUB_FILES_PHOT_SUB)) {
        psError(PPSUB_ERR_IO, false, "Unable to set up photometry files.");
        success = false;
        goto ESCAPE;
    }

    if (!data->quality && !ppSubReadoutPhotometry("PPSUB.OUTPUT", data)) {
        psError(psErrorCodeLast(), false, "Unable to perform photometry.");
        success = false;
        goto ESCAPE;
    }

    // forced photometry for positive image 1
    if (data->forcedPhot1 && !data->quality) {
	if (!ppSubReadoutForcedPhot("PPSUB.FORCED1.SOURCES", "PPSUB.OUTPUT", "PPSUB.POS1.SOURCES", data)) {
	    psError(psErrorCodeLast(), false, "Unable to perform photometry.");
	    success = false;
            goto ESCAPE;
	}
    }

    // forced photometry for positive image 2
    if (data->forcedPhot2 && !data->quality) {
	if (!ppSubReadoutForcedPhot("PPSUB.FORCED2.SOURCES", "PPSUB.OUTPUT", "PPSUB.POS2.SOURCES", data)) {
	    psError(psErrorCodeLast(), false, "Unable to perform photometry.");
	    success = false;
            goto ESCAPE;
	}
    }

    if (!ppSubFilesIterateUp(config, PPSUB_FILES_PHOT_SUB)) {
        psError(PPSUB_ERR_IO, false, "Unable to set up photometry files.");
        success = false;
        goto ESCAPE;
    }

    // Perform statistics on the cell
    if (!ppSubReadoutStats(data)) {
        psError(psErrorCodeLast(), false, "Unable to collect statistics");
        success = false;
        goto ESCAPE;
    }

    // Do Mask Stats
    {
	pmFPAview *view = ppSubViewReadout(); // View to readout
	if (!ppSubMaskStats(config, view,data->stats)) {
	    psError(psErrorCodeLast(), false, "Unable to generate mask statistics");
	    success = false;
	    psFree(view);
	    goto ESCAPE;
	}
	psFree(view);
    }
    
    // generate the binned image used to write the jpeg
    if (!ppSubReadoutJpeg(config)) {
        psError(psErrorCodeLast(), false, "Unable to update.");
        success = false;
        goto ESCAPE;
    }

    if (data->inverse) {
        // Set up inverse subtraction files
        if (!ppSubFilesIterateDown(config, PPSUB_FILES_INV)) {
            psError(PPSUB_ERR_IO, false, "Unable to set up inverse files.");
            success = false;
            goto ESCAPE;
        }

        if (data->inverse && !ppSubDefineOutput("PPSUB.INVERSE", config)) {
            psError(psErrorCodeLast(), false, "Unable to define inverse.");
            success = false;
            goto ESCAPE;
        }

        if (!ppSubReadoutInverse(config)) {
            psError(psErrorCodeLast(), false, "Unable to invert images.");
            success = false;
            goto ESCAPE;
        }

        // Close subtraction files and open inverse photometry files
        if (!ppSubFilesIterateUp(config, PPSUB_FILES_SUB)) {
            psError(PPSUB_ERR_IO, false, "Unable to close subtraction files.");
            success = false;
            goto ESCAPE;
        }

        if (!ppSubFilesIterateDown(config, PPSUB_FILES_PHOT_INV)) {
            psError(PPSUB_ERR_IO, false, "Unable to set up inverse files.");
            success = false;
            goto ESCAPE;
        }

        if (!data->quality && !ppSubReadoutPhotometry("PPSUB.INVERSE", data)) {
            psError(psErrorCodeLast(), false, "Unable to perform photometry.");
            success = false;
            goto ESCAPE;
        }

        // Close inverse subtraction files
        if (!ppSubFilesIterateUp(config, PPSUB_FILES_INV | PPSUB_FILES_PHOT_INV)) {
            psError(PPSUB_ERR_IO, false, "Unable to close subtraction files.");
            success = false;
            goto ESCAPE;
        }

    } else {
        // Close subtraction files
        if (!ppSubFilesIterateUp(config, PPSUB_FILES_SUB)) {
            psError(PPSUB_ERR_IO, false, "Unable to close subtraction files.");
            success = false;
            goto ESCAPE;
        }
    }

 ESCAPE:
    pmFPAfileDropInternal(config->files, "PSPHOT.BACKGND");
    pmFPAfileDropInternal(config->files, "PSPHOT.BACKMDL");
    pmFPAfileDropInternal(config->files, "PSPHOT.BACKMDL.STDEV");

    return success;
}

bool check_psphot_input (pmConfig *config, char *name) {

  return true;

  bool mdok = true;
    pmFPAfile *photFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.INPUT"); // Photometry file

    pmFPA *fpa = photFile ? photFile->fpa : NULL;
    pmChip *chip = fpa && fpa->chips->n ? fpa->chips->data[0] : NULL;

    psPlaneTransform *toFPA = chip ? chip->toFPA : NULL;
    int nCount = toFPA ? psMemGetRefCounter (toFPA) : 0;

    fprintf (stderr, "PSPHOT.INPUT %s chip %p toFPA %p Count %d\n", name, chip, toFPA, nCount);
    return true;
}

bool dumpout(pmConfig *config, char *name) 
{
    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *out = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");
    psphotSaveImage (NULL, out->image, name);
    psFree(view);
    return true;
}


