/** @file pswarpLoop.c
 *
 *  ** this function is not used... **
 *  @brief mail processing loop for pswarp
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"
#include <ppStats.h>
#include "pswarpFileNames.h"            // Lists of file rules used at different stages

#define WCS_NONLIN_TOL 0.001            // Non-linear tolerance for header WCS
#define TESTING 0                       // Testing output?

// Loop over the inputs, warp them to the output skycell and then write out the output.
bool pswarpLoopSkycell(pmConfig *config, psMetadata *stats)
{
    bool status;
    bool mdok;                          // Status of MD lookup

    const char *skyCamera = psMetadataLookupStr(NULL, config->arguments,
                                                "SKYCELL.CAMERA");  // Name of camera for skycell
    pmConfigCamerasCull(config, skyCamera);
    pmConfigRecipesCull(config, "PSWARP,PPSTATS,PSPHOT,PSASTRO,MASKS,JPEG");

    // load the recipe
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    if (!pswarpSetMaskBits(config)) {
        psError(psErrorCodeLast(), false, "failed to set mask bits");
        return NULL;
    }

    // output mask bits
    psImageMaskType maskValue = psMetadataLookupImageMask(&status, recipe, "MASK.OUTPUT");
    psAssert (status, "MASK.OUTPUT was not defined");

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PSWARP.INPUT");
    if (!input) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find input data!\n");
        return false;
    }

    // use the external astrometry source if supplied
    pmFPAfile *astrom = psMetadataLookupPtr(NULL, config->files, "PSWARP.ASTROM");
    if (!astrom) {
        astrom = input;
    }

    if (astrom->camera != input->camera) {
        psError(PSWARP_ERR_DATA, true, "Input camera and astrometry camera do not match.");
        return false;
    }

    // select the output readout
    pmFPAview *viewT0 = pmFPAviewAlloc(0);
    viewT0->chip = 0;
    viewT0->cell = 0;
    viewT0->readout = 0;
    pmReadout *output = pmFPAfileThisReadout(config->files, viewT0, "PSWARP.OUTPUT");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find output data!\n");
	psFree (viewT0);
        return false;
    }
    psFree (viewT0);

    // Turn all skycell files on to generate them, and then turn them off for the loop over the input images
    // the input, which is in a different format.
    // XXX why is this being done here, doesn't it duplicate the work in pswarpDefine.c??
    pswarpFileActivation(config, detectorFiles, false);
    pswarpFileActivation(config, photFiles, false);
    pswarpFileActivation(config, independentFiles, false);
    pswarpFileActivation(config, skycellFiles, false);
    
    // pswarpFileActivation(config, skycellFiles, true);
    // if (0) {
    //   if (!pswarpIOChecksBefore(config)) {
    //     psError(psErrorCodeLast(), false, "Unable to read files.");
    //     goto DONE;
    //   }
    // }
   
    // Read the input astrometry
    // XXX rather than use the activations here, this should just explicitly loop over the desired filerule
    {
        pmFPAfileActivate(config->files, true, "PSWARP.ASTROM");

        pmChip *chip;
        pmFPAview *viewT1 = pmFPAviewAlloc(0);
        if (!pmFPAfileIOChecks(config, viewT1, PM_FPA_BEFORE)) {
            psError(psErrorCodeLast(), false, "Unable to read files.");
	    psFree(viewT1);
            goto DONE;
        }
        while ((chip = pmFPAviewNextChip (viewT1, input->fpa, 1)) != NULL) {
            psTrace ("pswarp", 4, "Chip %d: %x %x\n", viewT1->chip, chip->file_exists, chip->process);
            if (!chip->process || !chip->file_exists) { continue; }
            if (!pmFPAfileIOChecks(config, viewT1, PM_FPA_BEFORE)) {
                psError(psErrorCodeLast(), false, "Unable to read files.");
		psFree(viewT1);
                goto DONE;
            }
            pmCell *cell;
            while ((cell = pmFPAviewNextCell (viewT1, input->fpa, 1)) != NULL) {
                psTrace ("pswarp", 4, "Cell %d: %x %x\n", viewT1->cell, cell->file_exists, cell->process);
                if (!cell->process || !cell->file_exists) { continue; }
                if (!pmFPAfileIOChecks (config, viewT1, PM_FPA_BEFORE) ||
                    !pmFPAfileIOChecks (config, viewT1, PM_FPA_AFTER)) {
                    psError(psErrorCodeLast(), false, "Unable to read files.");
		    psFree(viewT1);
                    goto DONE;
                }
            }
            if (!pmFPAfileIOChecks (config, viewT1, PM_FPA_AFTER)) {
                psError(psErrorCodeLast(), false, "Unable to write files.");
		psFree(viewT1);
                goto DONE;
            }
        }
        if (!pmFPAfileIOChecks (config, viewT1, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "Unable to write files.");
	    psFree(viewT1);
            goto DONE;
        }
        psFree(viewT1);

        pswarpFileActivation(config, detectorFiles, true);
        pmFPAfileActivate(config->files, false, "PSWARP.ASTROM");
    }

    // Turn on the source output --- we need to get rid of these so that we can measure the PSF
    pmFPAfileActivate(config->files, true, "PSWARP.OUTPUT.SOURCES");

    // Don't care about the skycell anymore --- we've read it, and that's all we need to do.
    pmFPAfileActivate(config->files, false, "PSWARP.SKYCELL");
    pmFPAview *viewT2 = pmFPAviewAlloc(0);

    // find the FPA phu
    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU(viewT2, astrom->fpa);
    if (phu) {
        char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
        }
    }
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelMosaic(input->fpa, phu->header)) {
            psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for input FPA.");
            psFree(viewT2);
            goto DONE;
        }
    }

    psList *cells = psListAlloc(NULL);  // List of cells, for concepts averaging

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_BEFORE)) {
        psError(psErrorCodeLast(), false, "Unable to read files.");
            psFree(viewT2);
        goto DONE;
    }

    // *** main transformation block
    pmChip *chip;
    while ((chip = pmFPAviewNextChip (viewT2, input->fpa, 1)) != NULL) {
        psTrace ("pswarp", 4, "Chip %d: %x %x\n", viewT2->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_BEFORE)) {
            psError(psErrorCodeLast(), false, "Unable to read files.");
            psFree(viewT2);
            goto DONE;
        }

        // read WCS data from the corresponding header
        pmHDU *hdu = pmFPAviewThisHDU (viewT2, astrom->fpa);

	
        if (bilevelAstrometry) {
            if (!pmAstromReadBilevelChip (chip, hdu->header)) {
                psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for input FPA.");
                psFree(viewT2);
                goto DONE;
            }
        } else {
            // we use a default FPA pixel scale of 1.0
            if (!pmAstromReadWCS (input->fpa, chip, hdu->header, 1.0)) {
                psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input FPA.");
                psFree(viewT2);
                goto DONE;
            }
        }
	
        pmCell *cell;
        while ((cell = pmFPAviewNextCell (viewT2, input->fpa, 1)) != NULL) {
            psTrace ("pswarp", 4, "Cell %d: %x %x\n", viewT2->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
                psError(psErrorCodeLast(), false, "Unable to read files.");
            psFree(viewT2);
                goto DONE;
            }

            psListAdd(cells, PS_LIST_TAIL, cell);

            // process each of the readouts
            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout(view, input->fpa, 1)) != NULL) {
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    psError(psErrorCodeLast(), false, "Unable to read files.");
            psFree(viewT2);
                    goto DONE;
                }
                if (!readout->data_exists) {
                    continue;
                }

                // Copy the detections from the astrometry carrier to the input, so they can be accessed by
                // pswarpTransformReadout
                pmReadout *astromRO = pmFPAviewThisReadout(view, astrom->fpa); // Readout for astrometry
                pmDetections *detections = psMetadataLookupPtr(&mdok, astromRO->analysis, "PSPHOT.DETECTIONS"); // Sources from astrometry
                if (detections) {
                    psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_ARRAY, "Sources from input astrometry", detections);
                }

                pswarpTransformReadout(output, readout, config);
		
                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    psError(psErrorCodeLast(), false, "Unable to write files.");
            psFree(viewT2);
                    goto DONE;
                }
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                psError(psErrorCodeLast(), false, "Unable to write files.");
            psFree(viewT2);
                goto DONE;
            }
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "Unable to write files.");
            psFree(viewT2);
            goto DONE;
        }
    }

    if (!output->data_exists) {
        psWarning("No overlap between input and skycell.");
        if (stats) {
            psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                             "No overlap between input and skycell", PSWARP_ERR_NO_OVERLAP);
        }
        psphotFilesActivate(config, false);
        psFree(cells);
        psFree(viewT2);
        goto DONE;
    }
    
    pmCell *outCell = output->parent;   ///< Output cell
    pmChip *outChip = outCell->parent;  ///< Output chip
    pmFPA *outFPA = outChip->parent;    ///< Output FP

    if (!pswarpPixelsLit(output, stats, config)) {
        psError(psErrorCodeLast(), false, "Unable to calculate pixel regions.");
        psFree(cells);
        psFree(viewT2);
        goto DONE;
    }
    bool doStats = psMetadataLookupBool(&mdok,recipe,"MASK.STATS");
    if (doStats) {
      if (!pswarpMaskStats(output, stats, config)) {
	psError(psErrorCodeLast(), false, "Unable to calculate mask stats.");
	psFree(cells);
	psFree(viewT2);
	goto DONE;
      }
    }
    // Set covariance matrix for output
    {
        psList *covariances = psMetadataLookupPtr(&mdok, output->analysis,
                                                  PSWARP_ANALYSIS_COVARIANCES); // Covariance matrices
        psAssert(covariances, "Should be there");
        psArray *covars = psListToArray(covariances); // Array of covariance matrices
        psKernel *covar = psImageCovarianceAverage(covars);
        psFree(covars);
        psMetadataRemoveKey(output->analysis, PSWARP_ANALYSIS_COVARIANCES);

        // Correct covariance matrix scale for the mean (square root of the) Jacobian
        double jacobian = psMetadataLookupF64(NULL, output->analysis, PSWARP_ANALYSIS_JACOBIAN); // Jacobian
        int goodPixels = psMetadataLookupS32(NULL, output->analysis, PSWARP_ANALYSIS_GOODPIX);   // Good pixels
        jacobian /= goodPixels;
        output->covariance = psImageCovarianceScale(covar, jacobian);
        psFree(covar);

        if (output->variance) {
            psImageCovarianceTransfer(output->variance, output->covariance);
        }
    }

    if (!pmConceptsAverageCells(outCell, cells, NULL, NULL, false)) {
        psError(psErrorCodeLast(), false, "Unable to average cell concepts.");
        psFree(cells);
        psFree(viewT2);
        goto DONE;
    }
    psFree(cells);

    psRegion *trimsec = psMetadataLookupPtr(NULL, outCell->concepts, "CELL.TRIMSEC"); ///< Trim section
    trimsec->x0 = trimsec->x1 = trimsec->y0 = trimsec->y1 = 0; ///< All pixels

    if (!psMetadataCopy(outFPA->concepts, input->fpa->concepts)) {
        psError(psErrorCodeLast(), false, "Unable to copy FPA concepts from input to output.");
        psFree(viewT2);
        goto DONE;
    }

    // Update ZP from the astrometry
    {
        psMetadataItem *item = psMetadataLookup(outFPA->concepts, "FPA.ZP");
        item->data.F32 = psMetadataLookupF32(NULL, astrom->fpa->concepts, "FPA.ZP");
    }

    pmHDU *hdu = outFPA->hdu;           ///< HDU for the output warped image

    // Copy header from target
    {
        pmFPAview *skyView = pmFPAviewAlloc(0); ///< View into skycell
        skyView->chip = skyView->cell = 0;
        pmCell *cell = pmFPAfileThisCell(config->files, skyView, "PSWARP.SKYCELL"); // Skycell cell
        psFree(skyView);
        pmHDU *skyHDU = pmHDUFromCell(cell); ///< HDU
        if (!skyHDU) {
            psError(PSWARP_ERR_DATA, false, "Unable to find skycell HDU.");
            psFree(viewT2);
            goto DONE;
        }
        hdu->header = psMetadataCopy(hdu->header, skyHDU->header);
    }

    pswarpVersionHeader(hdu->header);
    
    if (!pmAstromWriteWCS(hdu->header, outFPA, outChip, WCS_NONLIN_TOL)) {
        psError(psErrorCodeLast(), false, "Unable to generate WCS header.");
            psFree(viewT2);
        goto DONE;
    }

    if (!pmFPAfileIOChecks(config, viewT2, PM_FPA_AFTER)) {
        psError(psErrorCodeLast(), false, "Unable to write files.");
            psFree(viewT2);
        goto DONE;
    }

    // Done with the detector side of things
    pswarpFileActivation(config, detectorFiles, false);
    pswarpFileActivation(config, independentFiles, false);


    // We need a new PSF model for the warped frame.  It would be good to generate this analytically, but
    // that's going to be tricky.  We have a list of sources, so we use those to redetermine the PSF model.

    if (psMetadataLookupBool(&mdok, recipe, "PSF")) {
        pswarpFileActivation(config, photFiles, true);
        if (!pswarpIOChecksBefore(config)) {
            psError(psErrorCodeLast(), false, "Unable to read files.");
            psFree(viewT2);
            goto DONE;
        }

        // supply the readout and fpa of interest to psphot
        pmFPAfile *photFile = psMetadataLookupPtr(NULL, config->files, "PSPHOT.INPUT");
        pmFPACopy(photFile->fpa, outFPA);

        pmFPAview *viewT3 = pmFPAviewAlloc(0); ///< View into skycell
        viewT3->chip = viewT3->cell = viewT3->readout = 0;

        // grab the sources of interest from the storage location (pmFPAfile PSPHOT.INPUT.CMF)
        psArray *sources = psphotLoadPSFSources (config, view);
        if (!sources) {
            psError(psErrorCodeLast(), false, "No sources supplied to measure PSF");
            psFree(viewT2);
            goto DONE;
        }

        pmModelClassSetLimits(PM_MODEL_LIMITS_STRICT);

        // measure the PSF using these sources
        if (!psphotReadoutFindPSF(config, view, "PSPHOT.INPUT", sources)) {
            // This is likely a data quality issue
            // XXX Split into multiple cases using error codes?
            psErrorStackPrint(stderr, "Unable to determine PSF");
            psWarning("Unable to determine PSF --- suspect bad data quality.");
            if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
                psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                                 "Unable to determine PSF", psErrorCodeLast());
            }
            psErrorClear();
            psphotFilesActivate(config, false);
        }

        // Ensure seeing is carried over
        pmChip *photChip = pmFPAviewThisChip(view, photFile->fpa);                 // Chip with seeing
        psMetadataItem *item = psMetadataLookup(outChip->concepts, "CHIP.SEEING"); // Concept with seeing
        item->data.F32 = psMetadataLookupF32(NULL, photChip->concepts, "CHIP.SEEING");

// XXX EAM : put this in a visualization function
#if (TESTING)
        {
            #define PSF_SIZE 20         ///< Half-size of PSF
            #define PSF_FLUX 10000      ///< Central flux for PSF
            pmChip *photChip = pmFPAviewThisChip(view, photFile->fpa);
            pmPSF *psf = psMetadataLookupPtr(NULL, photChip->analysis, "PSPHOT.PSF");
            psImage *image = psImageAlloc(2 * PSF_SIZE + 1, 2 * PSF_SIZE + 1, PS_TYPE_F32);
            psImageInit(image, 0);
            pmModel *model = pmModelFromPSFforXY(psf, PSF_SIZE, PSF_SIZE, PSF_FLUX);
            pmModelAdd(image, NULL, model, PM_MODEL_OP_FULL, 0);
            psFree(model);
            psFits *fits = psFitsOpen("psf.fits", "w");
            psFitsWriteImage(fits, NULL, image, 0, NULL);
            psFitsClose(fits);
            psFree(image);
        }
#endif

        psFree(view);
    }

    // Perform statistics on the output image
    if (stats) {
        if (!ppStatsFPA(stats, output->parent->parent->parent, view, maskValue, config)) {
            psWarning("Unable to perform statistics on warped image.");
        }
    }
    

    // Add MD5 information for readout
    const char *chipName = psMetadataLookupStr(NULL, output->parent->parent->concepts, "CHIP.NAME");
    const char *cellName = psMetadataLookupStr(NULL, output->parent->concepts, "CELL.NAME");
    psString headerName = NULL; ///< Header name for MD5
    psStringAppend(&headerName, "MD5_%s_%s_%d", chipName, cellName, viewT2->readout);
    psVector *md5 = psImageMD5(output->image); ///< md5 hash
    psString md5string = psMD5toString(md5); ///< String
    psFree(md5);
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, headerName, PS_META_REPLACE,
                     "Image MD5", md5string);
    psFree(md5string);
    psFree(headerName);
    psFree(viewT2);

 DONE:

    return true;
}
