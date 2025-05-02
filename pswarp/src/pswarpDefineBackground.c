/** @file pswarpDefineBackground.c
 * 
 *  @brief generates the output background model
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

bool pswarpDefineBackground (pmConfig *config) {

    if (!psMetadataLookupBool(NULL,config->arguments,"BACKGROUND.MODEL")) return true;

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
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PSWARP.OUTPUT.BKGMODEL");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find output data!\n");
        return false;
    }
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PSWARP.INPUT");
    if (!input) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find input data!\n");
        return false;
    }
    
    // open the full skycell file; no need to defer different depths. only load the header data
    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPAfileOpen (skycell, view, config);

    // Read header and create target
    {
        if (!pmFPAReadHeaderSet(skycell->fpa, skycell->fits, config)) {
            psError(psErrorCodeLast(), false, "Unable to read headers for skycell.");
            psFree(view);
            return false;
        }
        view->chip = 0;
        view->cell = 0;
        view->readout = 0;
        pmCell *source = pmFPAfileThisCell(config->files, view, "PSWARP.SKYCELL"); ///< Source cell
        pmHDU *hdu = pmHDUFromCell(source); ///< HDU for source
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

        pmCell *target = pmFPAviewThisCell(view, output->fpa); ///< Target cell
        pmReadout *readout = pmReadoutAlloc(target); ///< Target readout
        readout->image = psImageAlloc(numCols / output->xBin, numRows / output->yBin, PS_TYPE_F32);
        psImageInit(readout->image, NAN);
        psFree(readout);                // Drop reference

        bool status = false;
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.XBIN");
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.YBIN");
 	// psMetadataAddS32(target->concepts,PS_LIST_TAIL,"CELL.XBIN",PS_META_REPLACE,"",output->xBin);
 	// psMetadataAddS32(target->concepts,PS_LIST_TAIL,"CELL.YBIN",PS_META_REPLACE,"",output->yBin);
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.XSIZE");
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.YSIZE");
 	// psMetadataAddS32(target->concepts,PS_LIST_TAIL,"CELL.XSIZE",PS_META_REPLACE,"",numCols / output->xBin); 
 	// psMetadataAddS32(target->concepts,PS_LIST_TAIL,"CELL.YSIZE",PS_META_REPLACE,"",numRows / output->yBin);
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.XPARITY");
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.YPARITY");
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.X0");
        psMetadataItemSupplement(&status, target->concepts, source->concepts, "CELL.Y0");
    }

    // XXX this is not a sufficient test
    view->chip = 0;
    view->cell = 0;
    view->readout = -1;
    pmHDU *phu = pmFPAviewThisPHU(view, skycell->fpa); ///< Skycell PHU
    pmHDU *hdu = pmFPAviewThisHDU(view, skycell->fpa); ///< Skycell header

    pmAstromWCS *WCS = pmAstromWCSfromHeader(hdu->header);

    double cd1f = 1.0 * output->xBin;
    double cd2f = 1.0 * output->yBin;
    
    WCS->crpix1 = WCS->crpix1 / cd1f;
    WCS->crpix2 = WCS->crpix2 / cd2f;
    
    WCS->cdelt1 *= cd1f;
    WCS->cdelt2 *= cd2f;

    WCS->trans->x->coeff[1][0] *= cd1f;
    WCS->trans->x->coeff[0][1] *= cd2f;
    WCS->trans->y->coeff[1][0] *= cd1f;
    WCS->trans->y->coeff[0][1] *= cd2f;
    
    
    pmAstromWCStoHeader (hdu->header,WCS);

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
