/** @file pswarpHeadersLoad.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

/** XXX this function should load all of the PSWARP.INPUT headers
 * it should examine the overlap between each chip in PSWARP.INPUT
 * and the output (pswarpMatchRange) and select/de-select the chips
 * and/or cell which contribute pixels.
 *
 * pswarpDataLoad should then load the pixel of the needed chips
 *
 * all of the different astrometry analysis modes use the same data load loop
 */
bool pswarpHeadersLoad (pmConfig *config) {

    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;
    pmFPAview *view;

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSWARP.INPUT");
    if (!input) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find input data!\n");
        return false;
    }

    // use the external astrometry source if supplied
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSWARP.ASTROM");
    if (!astrom) {
        astrom = input;
    }

    // select the output readout
    view = pmFPAviewAlloc (0);
    view->chip = 0;
    view->cell = 0;
    view->readout = 0;
    pmReadout  *output = pmFPAfileThisReadout (config->files, view, "PSWARP.OUTPUT");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find output data!\n");
        return false;
    }
    psFree (view);

    // de-activate PSWARP.SKYCELL and PSWARP.OUTPUT
    pmFPAfileActivate(config->files, false, "PSWARP.SKYCELL");
    pmFPAfileActivate(config->files, false, "PSWARP.OUTPUT");

    view = pmFPAviewAlloc (0);

    // XXX need to read only the headers for the skycell
    // XXX these pmAstromReadBilevel functions seem to be broken

    // find the FPA phu
    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU(view, astrom->fpa);
    if (phu) {
        char *ctype = psMetadataLookupStr (NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp(&ctype[4], "-DIS");
        }
    }
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelMosaic(input->fpa, phu->header)) {
            psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for input.");
            psFree(view);
            return false;
        }
    }

    // files associated with the science image
    pmFPAfileIOChecks (config, view, PM_FPA_BEFORE);

    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        pmFPAfileIOChecks (config, view, PM_FPA_BEFORE);

        // read WCS data from the corresponding header
        pmHDU *hdu = pmFPAviewThisHDU (view, astrom->fpa);
        if (bilevelAstrometry) {
            if (!pmAstromReadBilevelChip (chip, hdu->header)) {
                psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for input.");
                psFree(view);
                return false;
            }
        } else {
            // we use a default FPA pixel scale of 1.0
            if (!pmAstromReadWCS(input->fpa, chip, hdu->header, 1.0)) {
                psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input.");
                psFree(view);
                return false;
            }
        }

        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            pmFPAfileIOChecks (config, view, PM_FPA_BEFORE);

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                pmFPAfileIOChecks (config, view, PM_FPA_BEFORE);
                if (! readout->data_exists) { continue; }

                // XXX Replace with a function to examine the overlap and turn on/off chips
                // based on that result
                // pswarpTransformReadout_Opt (output, readout, config);

                pmFPAfileIOChecks (config, view, PM_FPA_AFTER);
            }
            pmFPAfileIOChecks (config, view, PM_FPA_AFTER);
        }
        pmFPAfileIOChecks (config, view, PM_FPA_AFTER);
    }
    pmFPAfileIOChecks (config, view, PM_FPA_AFTER);
    psFree (view);
    return true;
}
