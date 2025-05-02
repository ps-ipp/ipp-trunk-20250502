/** @file pswarpDefine.c
 *
 *  @brief generates the output images
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

bool pswarpDefine (pmConfig *config) {

    // load the PSWARP recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find PSWARP recipe!\n");
        return false;
    }

    // select the input data sources
    pmFPAfile *skycell = psMetadataLookupPtr (NULL, config->files, "PSWARP.SKYCELL");
    if (!skycell) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find skycell data!\n");
        return false;
    }
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PSWARP.OUTPUT");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find output data!\n");
        return false;
    }
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PSWARP.INPUT");
    if (!input) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find input data!\n");
        return false;
    }
    
    // Read the output astrometry
    // XXX this section loops over the input skycell to load the header info
    {
        pmFPAview *view = pmFPAviewAlloc(0);

        pmChip *chip;
	if (!pmFPAfileRead (skycell, view, config)) {
	    psError(PS_ERR_IO, false, "failed READ at FPA %s", skycell->name);
	    psFree(view);
	    return false;
	}
        while ((chip = pmFPAviewNextChip (view, skycell->fpa, 1)) != NULL) {
            psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
            if (!chip->process || !chip->file_exists) { continue; }
	    if (!pmFPAfileRead (skycell, view, config)) {
                psError(psErrorCodeLast(), false, "failed READ at CHIP %s", skycell->name);
		psFree(view);
		return false;
            }
            pmCell *cell;
            while ((cell = pmFPAviewNextCell (view, skycell->fpa, 1)) != NULL) {
                psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (!cell->process || !cell->file_exists) { continue; }
		if (!pmFPAfileRead (skycell, view, config)) {
		    psError(psErrorCodeLast(), false, "failed READ at CELL %s", skycell->name);
		    psFree(view);
		    return false;
                }
		if (!pmFPAfileClose(skycell, view)) {
		    psError(psErrorCodeLast(), false, "failed CLOSE at CELL %s", skycell->name);
		    psFree (view);
		    return false;
		}

		// we've got the output astrom header
		pmHDU *hdu = pmHDUFromCell(cell); ///< HDU for source
		if (!hdu || !hdu->header) {
		    psError(PM_ERR_PROG, false, "Unable to find header for sky cell.");
		    psFree(view);
		    return false;
		}
		int numCols = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); ///< Number of columns
		int numRows = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); ///< Number of rows
		if ((numCols == 0) || (numRows == 0)) {
		    psError(PSWARP_ERR_DATA, false, "skycell has invalid dimensions %d x %d", numCols, numRows);
		    psFree(view);
		    return false;
		}
		
		// XXX generate the output pixels
		pmCell *target = pmFPAviewThisCell(view, output->fpa); ///< Target cell
		pmReadout *readout = pmReadoutAlloc(target); ///< Target readout
		readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
		psImageInit(readout->image, NAN);
		psFree(readout);                // Drop reference
		
		bool status = false;
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.XBIN");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.YBIN");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.XSIZE");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.YSIZE");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.XPARITY");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.YPARITY");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.X0");
		psMetadataItemSupplement(&status, target->concepts, cell->concepts, "CELL.Y0");
            }
	    if (!pmFPAfileClose(skycell, view)) {
		psError(psErrorCodeLast(), false, "failed CLOSE at CHIP %s", skycell->name);
		psFree (view);
		return false;
	    }
	}
	if (!pmFPAfileClose(skycell, view)) {
	    psError(psErrorCodeLast(), false, "failed CLOSE at FPA %s", skycell->name);
	    psFree (view);
	    return false;
	}
	psFree(view);
    }

    // the section below converts the header astrometry info (in skycell) to the pmFPA
    // astrometry structures, saving them on the output fpa structure.

    // XXX this block should be modified to loop over all chips

    pmFPAview *view = pmFPAviewAlloc (0);
    view->chip = 0;
    view->cell = 0;
    view->readout = -1;
    pmHDU *phu = pmFPAviewThisPHU(view, skycell->fpa); ///< Skycell PHU
    pmHDU *hdu = pmFPAviewThisHDU(view, skycell->fpa); ///< Skycell header
    bool bilevelAstrometry = false;
    if (phu) {
        char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp(&ctype[4], "-DIS");
        }
    }

    // We read from the skycell into the output.  i.e., the output receives the desired astrometry.
    pmChip *outputChip = pmFPAviewThisChip(view, output->fpa); ///< Chip in the output
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelMosaic(output->fpa, phu->header)) {
            psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for skycell.");
            psFree(view);
            return false;
        }
        if (!pmAstromReadBilevelChip(outputChip, hdu->header)) {
            psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for skycell.");
            psFree(view);
            return false;
        }
    } else {
        // we use a default FPA pixel scale of 1.0
        if (!pmAstromReadWCS(output->fpa, outputChip, hdu->header, 1.0)) {
            psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for skycell.");
            psFree(view);
            return false;
        }
    }
    
    view->chip = view->cell = view->readout = -1;
    pmFPAAddSourceFromView(output->fpa, view, output->format);


    psFree (view);
    return true;
}

