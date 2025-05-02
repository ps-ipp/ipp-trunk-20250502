/** @file pswarpUpdateStatistics.c
 *
 *  @brief generate output statistics
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

// once the output fpa elements have been built, loop over the fpa and generate stats
// for each readout
bool pswarpUpdateStatistics (pmFPA *output, psMetadata *fpaStats, pmFPA *input, pmFPA *astrom, pmConfig *config)  {

    if (!fpaStats) {
	psLogMsg("pswarp", PS_LOG_INFO, "stats not requested, skipping");
	return true;
    }

    // load the recipe
    bool status = false;
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // output mask bits
    psImageMaskType maskValue = psMetadataLookupImageMask(&status, recipe, "MASK.OUTPUT");
    psAssert (status, "MASK.OUTPUT was not defined");

    int Nreadout = 0; // count the number of readouts with data

    bool doMaskStats = psMetadataLookupBool(&status, recipe, "MASK.STATS"); 

    pmFPAview *view = pmFPAviewAlloc(0);
    
    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, output, 1)) != NULL) {
        psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

	// use this in output metadata info (MD5 sums)
	const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");

	psMetadata *chipStats = psMetadataLookupPtr (&status, fpaStats, chipName);
	if (!chipStats) {
	  chipStats = psMetadataAlloc ();
	  psMetadataAdd (fpaStats, PS_LIST_TAIL, chipName, PS_DATA_METADATA, "chip stats folder", chipStats);
	  psFree (chipStats);
	}

        pmCell *cell;
        while ((cell = pmFPAviewNextCell (view, output, 1)) != NULL) {
            psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

	    // use this in output metadata info (MD5 sums)
	    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");

	    psMetadata *cellStats = psMetadataLookupPtr (&status, chipStats, cellName);
	    if (!cellStats) {
	      cellStats = psMetadataAlloc ();
	      psMetadataAdd (chipStats, PS_LIST_TAIL, cellName, PS_DATA_METADATA, "cell stats folder", cellStats);
	      psFree (cellStats);
	    }

            // process each of the readouts
            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout(view, output, 1)) != NULL) {
	      // skip empty output readouts
                if (!readout->data_exists) {
		  continue;
		}
		Nreadout ++;
    
		psString readoutName = NULL;
		psStringAppend (&readoutName, "READOUT.%02d", view->readout);

		psMetadata *readoutStats = psMetadataLookupPtr (&status, cellStats, readoutName);
		if (!readoutStats) {
		  readoutStats = psMetadataAlloc ();
		  psMetadataAdd (cellStats, PS_LIST_TAIL, readoutName, PS_DATA_METADATA, "readout stats folder", readoutStats);
		  psFree (readoutStats);
		}

		if (!pswarpPixelsLit(readout, readoutStats, config)) {
		    psError(psErrorCodeLast(), false, "Unable to calculate pixel regions.");
		    psFree(view);
		    return false;
		}

		if (doMaskStats && !pswarpMaskStats(readout, readoutStats, config)) {
		  psError(psErrorCodeLast(), false, "Unable to calculate mask stats.");
		  psFree(view);
		  return false;
		}

		psFree (readoutName);
	    }
	}
    }

    // Perform statistics on the output image (ppStatsFPA loops down the hierarchy)
    pmFPAviewReset (view);
    if (!ppStatsFPA(fpaStats, output, view, maskValue, config)) {
	psWarning("Unable to perform statistics on warped image.");
    }
    psFree(view);

    if (Nreadout == 0) {
      psWarning("No overlap between input and skycell.");
      if (fpaStats) {
	psMetadataAddS32(fpaStats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "No overlap between input and skycell", PSWARP_ERR_NO_OVERLAP);
      }
    }      

    return true;
}
