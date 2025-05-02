/** @file ppSubReadoutPhotometry.c
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

bool ppSubReadoutPhotometry(const char *name, ppSubData *data)
{
    bool mdok = false;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    if (!data->photometry) {
        return true;
    }

    // select the view of interest
    pmFPAview *view = ppSubViewReadout(); // View to readout

    // psphotReadoutMinimal performs the photometry analysis on PSPHOT.INPUT; we need to move
    // around the pointers so PSPHOT.INPUT corresponds to the output image of interest (on one
    // pass this is the subtraction image, in another it is negative of the subtraction
    pmFPAfile *photFile = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.INPUT"); // Photometry file
    pmFPAfile *inFile = psMetadataLookupPtr(&mdok, config->files, name); // Input file
    if (!pmFPACopy(photFile->fpa, inFile->fpa)) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to copy FPA for photometry");
        psFree(view);
        return false;
    }

    // psphot may need the PHU header
    if (!photFile->fpa->hdu) {
      photFile->fpa->hdu = psMemIncrRefCounter (inFile->fpa->hdu);
    }

    // drop references to PSPHOT.DETECTIONS on both of these  (why is this needed for both??)
    pmReadout *photRO = pmFPAviewThisReadout(view, photFile->fpa); // Readout to photometer
    if (psMetadataLookup(photRO->analysis, "PSPHOT.DETECTIONS")) {
        psMetadataRemoveKey(photRO->analysis, "PSPHOT.DETECTIONS");
    }
    pmReadout *inRO = pmFPAfileThisReadout(config->files, view, name); // Readout with image and sources
    if (psMetadataLookup(inRO->analysis, "PSPHOT.DETECTIONS")) {
        psMetadataRemoveKey(inRO->analysis, "PSPHOT.DETECTIONS");
    }

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

    if (!psphotReadoutMinimal(config, view, "PSPHOT.INPUT")) {
        // This is likely a data quality issue
        // XXX Split into multiple cases using error codes?
        psErrorStackPrint(stderr, "Unable to perform photometry on image");
        psWarning("Unable to perform photometry on image --- suspect bad data quality.");
        ppSubDataQuality(data, psErrorCodeLast(), PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
    }

    // If no sources were found, there's no error,  but we want to trigger 'bad quality'
    psWarning("no sources found: why is this being set to bad quality??");
    pmDetections *detections = psMetadataLookupPtr(NULL, photRO->analysis, "PSPHOT.DETECTIONS"); // Sources
    if (!detections) {
        ppSubDataQuality(data, PSPHOT_ERR_DATA, PPSUB_FILES_PHOT_SUB | PPSUB_FILES_PHOT_INV);
    }
    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    // a likely source of false positives is poor subtractions.  this results in
    // detections in the wings (or cores) of bright(er) stars found in both images.
    // flag detections based on their distance from the bright(er) input sources.
    bool subInverse = !strcasecmp(name, "PPSUB.INVERSE");
    ppSubFlagNeighbors (config, view, sources, subInverse);

    if (data->stats) {
        bool mdok;
        int numSources = psMetadataLookupS32(&mdok, data->stats, "NUM_SOURCES"); // Number of sources
        numSources += sources ? sources->n : 0;
        psMetadataAddS32(data->stats, PS_LIST_TAIL, "NUM_SOURCES", PS_META_REPLACE,
                         "Total number of sources detected", numSources);
        float newTime = psTimerClear("PPSUB_PHOT"); // Time for photometry
        float oldTime = psMetadataLookupF32(&mdok, data->stats, "TIME_PHOT"); // Previous time for photometry
        psMetadataAddF32(data->stats, PS_LIST_TAIL, "TIME_PHOT", PS_META_REPLACE, "Time to do photometry",
                         isfinite(oldTime) ? oldTime + newTime : newTime);
    }

    if (!data->quality) {
        if (!psMetadataCopySingle(inRO->analysis, photRO->analysis, "PSPHOT.DETECTIONS")) {
            psError(PPSUB_ERR_PROG, false, "Unable to copy PSPHOT.DETECTIONS");
	    psFree(view);
            return false;
        }
        if (!psMetadataCopySingle(inRO->analysis, photRO->analysis, "PSPHOT.HEADER")) {
            psError(PPSUB_ERR_PROG, false, "Unable to copy PSPHOT.HEADER");
	    psFree(view);
            return false;
        }
        if (!psMetadataCopySingle(inRO->analysis, photRO->analysis, PM_DETEFF_ANALYSIS)) {
            psError(PPSUB_ERR_PROG, false, "Unable to copy Detection Efficiency");
	    psFree(view);
            return false;
        }

        // Ensure photometry information is put in the header
        pmHDU *hdu = pmHDUFromReadout(inRO); // HDU for readout
        if (hdu) {
            psMetadata *photHeader = psMetadataLookupMetadata(NULL, inRO->analysis, "PSPHOT.HEADER"); // Header
            hdu->header = psMetadataCopy(hdu->header, photHeader);
        }
    }

    psFree(view);
    return true;
}

#ifdef TESTING
// Record data about sources: not everything gets into the output CMF files
    {
        pmReadout *photRO = pmFPAviewThisReadout(view, photFile->fpa); // Readout with the sources
        pmDetections *detections = psMetadataLookupPtr(NULL, photRO->analysis, "PSPHOT.DETECTIONS"); // Sources
        psArray *sources = detections->allSources;
        FILE *sourceFile = fopen("sources.dat", "w"); // File for sources
        fprintf(sourceFile,
                "# x y mag mag_err psf_chisq cr_nsigma ext_nsigma psf_qf flags m_x m_y m_xx m_xy m_yy\n");
        for (int i = 0; i < sources->n; i++) {
            pmSource *source = sources->data[i];
            if (!source) {
                continue;
            }

            float x, y;             // Position of source
            float chi2;             // chi^2 for source
            if (source->modelPSF) {
                x = source->modelPSF->params->data.F32[PM_PAR_XPOS];
                y = source->modelPSF->params->data.F32[PM_PAR_YPOS];
                chi2 = source->modelPSF->chisq;
            } else if (source->peak) {
                x = source->peak->xf;
                y = source->peak->yf;
                chi2 = NAN;
            } else {
                psWarning("No position available for source.");
                continue;
            }

            float xMoment = NAN, yMoment = NAN, xxMoment = NAN, xyMoment = NAN, yyMoment = NAN;
            if (source->moments) {
                xMoment = source->moments->Mx;
                yMoment = source->moments->My;
                xxMoment = source->moments->Mxx;
                xyMoment = source->moments->Mxy;
                yyMoment = source->moments->Myy;
            }

            fprintf(sourceFile, "%f %f %f %f %f %f %f %f %d %f %f %f %f %f\n",
                    x, y, source->psfMag, source->psfMagErr, chi2, source->crNsigma, source->extNsigma,
                    source->pixWeight, source->mode, xMoment, yMoment, xxMoment, xyMoment, yyMoment);
        }
        fclose(sourceFile);
    }
#endif
