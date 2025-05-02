/** @file ppSubInputDetections.c
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

pmHDU *getHDUandLevel (pmFPALevel *level, pmFPA *fpa, pmFPAview *view) {

    pmHDU *hdu = NULL;
    *level = PM_FPA_LEVEL_NONE;

    pmCell *cell = pmFPAviewThisCell(view, fpa);
    if (cell) {
	hdu = cell->hdu; // cell
	if (hdu) {
	    *level = PM_FPA_LEVEL_CELL;
	    return hdu;
	}
    }

    pmChip *chip = pmFPAviewThisChip(view, fpa);
    if (chip) {
	hdu = chip->hdu; // chip
	if (hdu) {
	    *level = PM_FPA_LEVEL_CHIP;
	    return hdu;
	}
    }

    hdu = fpa->hdu; // fpa
    if (hdu) {
	*level = PM_FPA_LEVEL_FPA;
	return hdu;
    }
    return NULL;
}      

pmHDU *setHDUatLevel (pmFPALevel level, pmFPA *fpa, pmFPAview *view, char *extname) {

    switch (level) {
      case (PM_FPA_LEVEL_FPA): {
	  if (!fpa->hdu) {
	      fpa->hdu = pmHDUAlloc(extname);
	  }
	  return fpa->hdu;
      }
      case (PM_FPA_LEVEL_CHIP): {
	  pmChip *chip = pmFPAviewThisChip(view, fpa);
	  if (!chip->hdu) {
	      chip->hdu = pmHDUAlloc(extname);
	  }
	  return chip->hdu;
      }
      case (PM_FPA_LEVEL_CELL): {
	  pmCell *cell = pmFPAviewThisCell(view, fpa);
	  if (!cell->hdu) {
	      cell->hdu = pmHDUAlloc(extname);
	  }
	  return cell->hdu;
      }
      default:
	return NULL;
    }
    return NULL;
}      

bool ppSubInputDetections (bool *foundDetections, const char *sourcesName, const char *imageName, ppSubData *data) {

    bool mdok = false;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    // select the view of interest
    pmFPAview *view = ppSubViewReadout(); // View to readout

    // point PSPHOT.INPUT at the positive image
    pmFPAfile *imageFile   = psMetadataLookupPtr(&mdok, config->files, imageName); // Image to photometer
    pmFPAfile *sourcesFile = psMetadataLookupPtr(&mdok, config->files, sourcesName); // Place results her
    pmFPAfile *photFile    = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.INPUT"); // holder for the working image

    // create a psphot working file, so we don't damage the image or affect existing sources
    if (!pmFPACopy(photFile->fpa, imageFile->fpa)) {
	psError(PPSUB_ERR_CONFIG, false, "Unable to copy FPA for photometry");
	psFree(view);
	return false;
    }

    pmFPALevel hduLevel = PM_FPA_LEVEL_NONE;
    pmHDU *imageHDU = getHDUandLevel (&hduLevel, imageFile->fpa, view);

    if (imageHDU) {
	pmFPALevel outLevel = PM_FPA_LEVEL_NONE;
	pmHDU *sourcesHDU = getHDUandLevel (&outLevel, sourcesFile->fpa, view);
	if (!sourcesHDU) {
	    sourcesHDU = setHDUatLevel (hduLevel, sourcesFile->fpa, view, imageHDU->extname);
	}
	
	if (!sourcesHDU->header) {
	    sourcesHDU->header = psMetadataAlloc();
	}
	if (!psMetadataCopy(sourcesHDU->header, imageHDU->header)) {
	    psError(PPSUB_ERR_PROG, false, "Unable to copy header");
	    psFree(view);
	    return false;
	}
    }

    if (!psphotReadout(config, view, "PSPHOT.INPUT")) {
	psErrorStackPrint(stderr, "Unable to perform photometry on image");
	psWarning("Unable to perform photometry on image --- suspect bad data quality.");
	ppSubDataQuality(data, psErrorCodeLast(), PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
    }
    // save the outputs on PPSUB.POS1.SOURCES
    if (!psphotCopyResults (foundDetections, sourcesFile, photFile, view)) {
	psError(PPSUB_ERR_PROG, false, "Unable to copy psphot outputs");
	psFree(view);
	return false;
    }
    // if no sources were found here, we report that back and let them handle it

    psFree(view);
    return true;
}

bool psphotCopyResults (bool *foundDetections, pmFPAfile *target, pmFPAfile *source, pmFPAview *view) {

    pmReadout *sourceRO = pmFPAviewThisReadout(view, source->fpa);
    psAssert(sourceRO, "programming error: source readout not defined");

    pmReadout *targetRO = pmFPAviewThisReadout(view, target->fpa);
    if (!targetRO) {
	pmCell *targetCell = pmFPAviewThisCell(view, target->fpa);
	targetRO = pmReadoutAlloc(targetCell);
	psAssert(targetRO, "programming error: could not make target readout");
    }

    // If no sources were found, there's no error, but we need to inform the calling function
    pmDetections *detections = psMetadataLookupPtr(NULL, sourceRO->analysis, "PSPHOT.DETECTIONS"); // Sources
    if (!detections) {
	*foundDetections = false;
	return true;
    } else {
	*foundDetections = true;
    }

    psArray *sources = detections->allSources; 
    psAssert (sources, "missing sources?");

    if (!psMetadataCopySingle(targetRO->analysis, sourceRO->analysis, "PSPHOT.DETECTIONS")) {
	psError(PPSUB_ERR_PROG, false, "Unable to copy PSPHOT.DETECTIONS");
	return false;
    }
    if (!psMetadataCopySingle(targetRO->analysis, sourceRO->analysis, "PSPHOT.HEADER")) {
	psError(PPSUB_ERR_PROG, false, "Unable to copy PSPHOT.HEADER");
	return false;
    }
    if (!psMetadataCopySingle(targetRO->analysis, sourceRO->analysis, PM_DETEFF_ANALYSIS)) {
	psError(PPSUB_ERR_PROG, false, "Unable to copy Detection Efficiency");
	return false;
    }

    // Ensure photometry information is put in the header
    // XXX create one if it does not exist?
    pmHDU *hdu = pmHDUFromReadout(targetRO); // HDU for readout
    if (hdu) {
	psMetadata *header = psMetadataLookupMetadata(NULL, targetRO->analysis, "PSPHOT.HEADER"); // Header
	hdu->header = psMetadataCopy(hdu->header, header);
    }

    targetRO->data_exists = true;
    targetRO->parent->data_exists = true;
    targetRO->parent->parent->data_exists = true;

    return true;
}

