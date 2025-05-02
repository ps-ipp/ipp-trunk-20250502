/** @file  pmPSF_IO.c
 *
 * This file contains functions to read and write PSF models using the psMetadata Config file
 * format.
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.37 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/

#include <string.h>
#include <strings.h>            /* for strn?casecmp */

#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"
#include "pmErrorCodes.h"

#include "pmHDU.h"
#include "pmFPA.h"

#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFitsIO.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmPSF_IO.h"
#include "pmSourceIO.h"

bool pmPSFmodelReadPSFClump (psMetadata *analysis, psMetadata *header);
bool pmPSFmodelRead_ApTrend (pmPSF *psf, pmFPAfile *file);
bool pmPSFmodelWrite_ApTrend (pmFPAfile *file, pmPSF *psf);

bool pmPSFmodelRead_GrowthCurve (pmPSF *psf, pmFPAfile *file);
bool pmPSFmodelWrite_GrowthCurve (pmFPAfile *file, pmPSF *psf);

bool pmPSFmodelCheckDataStatusForView (const pmFPAview *view, const pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa->chips, false);
    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        bool exists = pmPSFmodelCheckDataStatusForFPA (fpa);
        return exists;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }

    pmChip *chip = fpa->chips->data[view->chip];
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);

    if (view->cell == -1) {
        bool exists = pmPSFmodelCheckDataStatusForChip (chip);
        return exists;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }

    psError(PS_ERR_IO, false, "PSF only valid at the chip level");
    return false;
}

bool pmPSFmodelCheckDataStatusForFPA (const pmFPA *fpa) {

    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        if (pmPSFmodelCheckDataStatusForChip (chip)) return true;
    }
    return false;
}

bool pmPSFmodelCheckDataStatusForChip (const pmChip *chip) {
    PS_ASSERT_PTR_NON_NULL(chip, false);

    bool status;

    // select the psf of interest
    pmPSF *psf = psMetadataLookupPtr(&status, chip->analysis, "PSPHOT.PSF");
    return psf ? true : false;
}

bool pmPSFmodelWriteForView (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa->chips, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        if (!pmPSFmodelWriteFPA(fpa, view, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write PSF for fpa");
            return false;
        }
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        if (!pmPSFmodelWriteChip (chip, view, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write PSF for chip");
            return false;
        }
        return true;
    }

    psError(PM_ERR_CONFIG, true, "PSF must be written at the chip level");
    return false;
}

// read in all chip-level PSFmodel files for this FPA
bool pmPSFmodelWriteFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);
    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmPSFmodelWriteChip (chip, thisView, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write PSF for %dth chip", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

// read in all cell-level PSFmodel files for this chip
bool pmPSFmodelWriteChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(chip, false);

    // We need the readout as well, because that has the PSF analysis data (e.g., clumps)
    // There is only one, because photometry is done on chip-mosaicked data.
    pmFPAview *roView = pmFPAviewAlloc(0); // View to readout
    *roView = *view;
    roView->cell = 0;
    roView->readout = 0;
    pmReadout *ro = pmFPAviewThisReadout(roView, chip->parent); // Readout with analysis data
    psFree(roView);

    if (!pmPSFmodelWrite(chip->analysis, ro ? ro->analysis : NULL, view, file, config)) {
        psError(psErrorCodeLast(), false, "Failed to write PSF for chip");
        return false;
    }
    return true;
}

// XXX we save the model term identifiers (item) as S32, but they probably should be more flexible
bool pmTrend2DtoTable (psArray *table, pmTrend2D *trend, char *label, int item) {

    if (trend == NULL) return true; 

    if (trend->mode == PM_TREND_MAP) {
	// write the image components into a table: this is needed because they may each be a different size
	psImageMap *map = trend->map;
	for (int ix = 0; ix < map->map->numCols; ix++) {
	    for (int iy = 0; iy < map->map->numRows; iy++) {
		psMetadata *row = psMetadataAlloc ();
		psMetadataAddS32 (row, PS_LIST_TAIL, label,        0, "", item);
		psMetadataAddS32 (row, PS_LIST_TAIL, "X_POWER",    0, "", ix);
		psMetadataAddS32 (row, PS_LIST_TAIL, "Y_POWER",    0, "", iy);
		psMetadataAddF32 (row, PS_LIST_TAIL, "VALUE",      0, "", map->map->data.F32[iy][ix]);
		psMetadataAddF32 (row, PS_LIST_TAIL, "ERROR",      0, "", map->error->data.F32[iy][ix]);
		psMetadataAddU8  (row, PS_LIST_TAIL, "MASK",       0, "", 0); // no cells are masked

		psArrayAdd (table, 100, row);
		psFree (row);
	    }
	}
    } else {
	// write the polynomial components into a table
	psPolynomial2D *poly = trend->poly;
	for (int ix = 0; ix <= poly->nX; ix++) {
	    for (int iy = 0; iy <= poly->nY; iy++) {
		psMetadata *row = psMetadataAlloc ();
		psMetadataAddS32 (row, PS_LIST_TAIL, label,        0, "", item);
		psMetadataAddS32 (row, PS_LIST_TAIL, "X_POWER",    0, "", ix);
		psMetadataAddS32 (row, PS_LIST_TAIL, "Y_POWER",    0, "", iy);
		psMetadataAddF32 (row, PS_LIST_TAIL, "VALUE",      0, "", poly->coeff[ix][iy]);
		psMetadataAddF32 (row, PS_LIST_TAIL, "ERROR",      0, "", poly->coeffErr[ix][iy]);
		psMetadataAddU8  (row, PS_LIST_TAIL, "MASK",       0, "", poly->coeffMask[ix][iy]);

		psArrayAdd (table, 100, row);
		psFree (row);
	    }
	}
    }
    return true;
}

// extra trend2D elements from a row
bool pmTrend2DfromTableRow (pmTrend2D *trend, psMetadata *row) {

    bool status = false;

    int xPow = psMetadataLookupS32 (&status, row, "X_POWER");
    int yPow = psMetadataLookupS32 (&status, row, "Y_POWER");

    if (trend->mode == PM_TREND_MAP) {
	psImageMap *map = trend->map;
	assert (map);
	assert (map->map);
	assert (map->error);
	assert (xPow >= 0);
	assert (yPow >= 0);
	assert (xPow < map->map->numCols);
	assert (yPow < map->map->numRows);
	map->map->data.F32[yPow][xPow]    = psMetadataLookupF32 (&status, row, "VALUE");
	map->error->data.F32[yPow][xPow]  = psMetadataLookupF32 (&status, row, "ERROR");
    } else {
	psPolynomial2D *poly = trend->poly;
	assert (poly);
	assert (xPow >= 0);
	assert (yPow >= 0);
	assert (xPow <= poly->nX);
	assert (yPow <= poly->nY);
	poly->coeff[xPow][yPow]     = psMetadataLookupF32 (&status, row, "VALUE");
	poly->coeffErr[xPow][yPow]  = psMetadataLookupF32 (&status, row, "ERROR");
	poly->coeffMask[xPow][yPow] = psMetadataLookupU8  (&status, row, "MASK");
    }
    return true;
}

// for a pmPSF supplied on the analysis metadata, we write out
// if needed:
//   - a PHU blank header
// - image header        : FITS Image NAXIS = 0
// if (trendMode == MAP)
//   - psf resid (+header) : FITS Image
// else
//   - psf table (+header) : FITS Table
bool pmPSFmodelWrite (const psMetadata *chipAnalysis, const psMetadata *roAnalysis, const pmFPAview *view,
                      pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    bool status;
    char *headName, *tableName, *residName;

    if (!chipAnalysis) {
        psError(PM_ERR_PROG, true, "No analysis metadata for chip.");
        return false;
    }
    if (!roAnalysis) {
        psWarning("No analysis metadata for PSF, clump parameters cannot be saved.");
    }

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, "PSPHOT");
    if (!recipe) {
        psError(PM_ERR_CONFIG, false, "missing recipe %s", "PSPHOT");
        return false;
    }

    // write a PHU? (only if input image is MEF)
    // write a header? (only if this is the first readout for cell)
    //   note that the file->header is set to track the last hdu->header written
    // write the data? (always?)

    // get the current header
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    if (!fpa) {
        psError(psErrorCodeLast(), false, "Unable to get FPA for writing.");
        return false;
    }
    pmHDU *hdu = psMemIncrRefCounter(pmFPAviewThisHDU(view, fpa));
    psFree(fpa);
    if (!hdu) {
        psError(PM_ERR_CONFIG, false, "Unable to find HDU");
        return false;
    }

    // if file does not yet have a PHU, attempt to write one to disk
    // we only need a PHU if chips->n > 1 and file->fileLevel == FPA
    // otherwise, the chip header fills the PHU location
    // XXX this code could be placed in a 'pmPSF_WritePHU' function and called
    // from pmFPAfileIO.c.

    // define the EXTNAME values used for image header, table data, and residual image segments
    {
        // lookup the EXTNAME values used for table data and image header segments
        char *rule = NULL;

        // Menu of EXTNAME rules
        psMetadata *menu = psMetadataLookupMetadata(&status, file->camera, "EXTNAME.RULES");
        if (!menu) {
            psError(PM_ERR_CONFIG, true, "missing EXTNAME.RULES in camera.config");
            psFree(hdu);
            return false;
        }

        // EXTNAME for image header
        rule = psMetadataLookupStr(&status, menu, "PSF.HEAD");
        if (!rule) {
            psError(PM_ERR_CONFIG, false, "missing entry for PSF.HEAD in EXTNAME.RULES in camera.config");
            psFree(hdu);
            return false;
        }
        headName = pmFPAfileNameFromRule (rule, file, view);

        // EXTNAME for table data
        rule = psMetadataLookupStr(&status, menu, "PSF.TABLE");
        if (!rule) {
            psError(PM_ERR_CONFIG, false, "missing entry for PSF.TABLE in EXTNAME.RULES in camera.config");
            psFree (headName);
            psFree(hdu);
            return false;
        }
        tableName = pmFPAfileNameFromRule (rule, file, view);

        // EXTNAME for resid data
        rule = psMetadataLookupStr(&status, menu, "PSF.RESID");
        if (!rule) {
            psError(PM_ERR_CONFIG, false, "missing entry for PSF.RESID in EXTNAME.RULES in camera.config");
            psFree (headName);
            psFree (tableName);
            psFree(hdu);
            return false;
        }
        residName = pmFPAfileNameFromRule (rule, file, view);

        // EXTNAME for psf image
        // rule = psMetadataLookupStr(&status, menu, "PSF.RESID");
        // if (!rule) {
        //     psError(PS_ERR_UNKNOWN, false, "missing entry for PSF.RESID in EXTNAME.RULES in camera.config");
        //     psFree (headName);
        //     psFree (tableName);
        //     psFree(hdu);
        //     return false;
        // }
        // residName = pmFPAfileNameFromRule (rule, file, view);
    }

    // write out the IMAGE header segment
    // if this header block is new, write it to disk
    if (hdu->header != file->header) {
        // add EXTNAME, EXTHEAD, EXTTYPE to header
        psMetadataAddStr (hdu->header, PS_LIST_TAIL, "EXTTABLE", PS_META_REPLACE, "name of table extension", tableName);
        psMetadataAddStr (hdu->header, PS_LIST_TAIL, "EXTRESID", PS_META_REPLACE, "name of resid extension", residName);
        psMetadataAddStr (hdu->header, PS_LIST_TAIL, "EXTTYPE", PS_META_REPLACE, "extension type", "IMAGE");
        if (!file->wrote_phu) {
            // this hdu->header acts as the PHU: set EXTEND to be true
            psMetadataAddBool (hdu->header, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
            file->wrote_phu = true;
        }

        if (!psFitsWriteBlank(file->fits, hdu->header, headName)) {
            psError(psErrorCodeLast(), false, "Unable to write PSF PHU.");
            psFree(hdu);
            return false;
        }
        psTrace ("pmFPAfile", 5, "wrote ext head %s (type: %d)\n", file->filename, file->type);
        file->header = hdu->header;
        psFree (headName);
    }
    psFree(hdu);

    // select the psf of interest
    pmPSF *psf = psMetadataLookupPtr (&status, chipAnalysis, "PSPHOT.PSF");
    if (!psf) {
        psError(PM_ERR_PROG, true, "missing PSF for this analysis metadata");
        psFree (tableName);
        psFree (residName);
        return false;
    }

    // write the PSF model parameters in a FITS table
    {
        // we need to write a header for the table,
        psMetadata *header = psMetadataAlloc();

        char *modelName = pmModelClassGetName (psf->type);
        psMetadataAddStr (header, PS_LIST_TAIL, "PSF_NAME", 0, "PSF model name", modelName);

        psMetadataAddBool (header, PS_LIST_TAIL, "ERR_LMM",  0, "Use Poisson errors in fits?", psf->poissonErrorsPhotLMM);
        psMetadataAddBool (header, PS_LIST_TAIL, "ERR_LIN",  0, "Use Poisson errors in fits?", psf->poissonErrorsPhotLin);
        psMetadataAddBool (header, PS_LIST_TAIL, "ERR_PAR",  0, "Use Poisson errors in fits?", psf->poissonErrorsParams);

        int nPar = pmModelClassParameterCount (psf->type);
        psMetadataAdd (header, PS_LIST_TAIL, "PSF_NPAR", PS_DATA_S32, "PSF model parameter count", nPar);

        psMetadataAddS32 (header, PS_LIST_TAIL, "IMAXIS1", 0, "Image X Size", psf->fieldNx);
        psMetadataAddS32 (header, PS_LIST_TAIL, "IMAXIS2", 0, "Image Y Size", psf->fieldNy);
        psMetadataAddS32 (header, PS_LIST_TAIL, "IMREF1",  0, "Image X Ref",  psf->fieldXo);
        psMetadataAddS32 (header, PS_LIST_TAIL, "IMREF2",  0, "Image Y Ref",  psf->fieldYo);

        // extract PSF Clump info
        pmPSFClump psfClump;

        // we now save clump parameters for each region : need to save all of those
        if (roAnalysis) {
            int nRegions = psMetadataLookupS32 (&status, roAnalysis, "PSF.CLUMP.NREGIONS");
            psMetadataAddS32 (header, PS_LIST_TAIL, "PSF_CLN", PS_META_REPLACE, "number of psf clump regions", nRegions);
            for (int i = 0; i < nRegions; i++) {
                char regionName[PS_BIGWORD];
                snprintf (regionName, PS_BIGWORD, "PSF.CLUMP.REGION.%03d", i);
                psMetadata *regionMD = psMetadataLookupPtr (&status, roAnalysis, regionName);

                psfClump.X  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");   assert (status);
                psfClump.Y  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");   assert (status);
                psfClump.dX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");  assert (status);
                psfClump.dY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");  assert (status);

                char key[PS_SMALLWORD];
                ps_snprintf_nowarn (key, PS_SMALLWORD, "CLX_%03d", i);
                psMetadataAddF32 (header, PS_LIST_TAIL, key, PS_META_REPLACE, "psf clump center", psfClump.X);
                ps_snprintf_nowarn (key, PS_SMALLWORD, "CLY_%03d", i);
                psMetadataAddF32 (header, PS_LIST_TAIL, key, PS_META_REPLACE, "psf clump center", psfClump.Y);
                ps_snprintf_nowarn (key, PS_SMALLWORD, "CLDX_%03d", i);
                psMetadataAddF32 (header, PS_LIST_TAIL, key, PS_META_REPLACE, "psf clump size", psfClump.dX);
                ps_snprintf_nowarn (key, PS_SMALLWORD, "CLDY_%03d", i);
                psMetadataAddF32 (header, PS_LIST_TAIL, key, PS_META_REPLACE, "psf clump size", psfClump.dY);
            }
        }

        // save the dimensions of each parameter
        for (int i = 0; i < nPar; i++) {
            char name[PS_SMALLWORD];
            int nX, nY;

            pmTrend2D *trend = psf->params->data[i];
            if (trend == NULL) continue;

            if (trend->mode == PM_TREND_MAP) {
		nX = trend->map->map->numCols;
		nY = trend->map->map->numRows;
            } else {
		nX = trend->poly->nX;
		nY = trend->poly->nY;
            }
            ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_NX", i);
            psMetadataAddS32 (header, PS_LIST_TAIL, name, 0, "", nX);
            ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_NY", i);
            psMetadataAddS32 (header, PS_LIST_TAIL, name, 0, "", nY);
            ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_MD", i);
            char *modeName = pmTrend2DModeToString (trend->mode);
            psMetadataAddStr (header, PS_LIST_TAIL, name, 0, "", modeName);
            psFree (modeName);
        }

        // other required information describing the PSF
        psMetadataAddF32 (header, PS_LIST_TAIL, "AP_RESID", 0, "aperture residual", psf->ApResid);
        psMetadataAddF32 (header, PS_LIST_TAIL, "AP_ERROR", 0, "aperture residual scatter", psf->dApResid);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CHISQ",    0, "chi-square for fit", psf->chisq);
        psMetadataAddS32 (header, PS_LIST_TAIL, "NSTARS",   0, "number of stars used to measure PSF", psf->nPSFstars);

        // XXX can we drop this now?
        psMetadataAddF32 (header, PS_LIST_TAIL, "SKY_BIAS", PS_DATA_F32, "sky bias level", psf->skyBias);

	if (roAnalysis) {
	  float PSF_APERTURE =  psMetadataLookupF32(&status, roAnalysis, "PSF_APERTURE");
	  if (status) {
	    psMetadataAddF32 (header, PS_LIST_TAIL, "PSF_APERTURE", PS_DATA_F32, "aperture for psf objects", PSF_APERTURE);
	  }
	  float PSF_FIT_RADIUS =  psMetadataLookupF32(&status, roAnalysis, "PSF_FIT_RADIUS");
	  if (status) {
	    psMetadataAddF32 (header, PS_LIST_TAIL, "PSF_FIT_RADIUS", PS_DATA_F32, "aperture for psf objects", PSF_FIT_RADIUS);
	  }
	}

        // build a FITS table of the PSF parameters
        psArray *psfTable = psArrayAllocEmpty (100);
        for (int i = 0; i < nPar; i++) {
            pmTrend2D *trend = psf->params->data[i];
	    pmTrend2DtoTable (psfTable, trend, "MODEL_TERM", i);
        }

        // write an empty FITS segment if we have no PSF information
        if (psfTable->n == 0) {
            psError(PM_ERR_PROG, true, "No PSF data to write.");
            psFree(tableName);
            psFree(residName);
            psFree(psfTable);
            psFree(header);
            return false;
        } else {
            psTrace ("pmFPAfile", 5, "writing psf data %s\n", tableName);
            if (!psFitsWriteTable(file->fits, header, psfTable, tableName)) {
                psError(psErrorCodeLast(), false, "Error writing psf table data %s\n", tableName);
                psFree (tableName);
                psFree (residName);
                psFree (psfTable);
                psFree (header);
                return false;
            }
        }
        psFree (tableName);
        psFree (psfTable);
        psFree (header);
    }

    // write the residual images (3D)
    {
        psMetadata *header = psMetadataAlloc ();
        if (psf->residuals == NULL) {
            // set some header keywords to make it clear there are no residuals?
            if (!psFitsWriteBlank(file->fits, header, residName)) {
                psError(psErrorCodeLast(), false, "Unable to write blank PSF residual image.");
                psFree(residName);
                psFree(header);
                return false;
            }
            psFree (residName);
            psFree (header);
            return true;
        }

        psMetadataAddS32 (header, PS_LIST_TAIL, "XBIN",    0, "", psf->residuals->xBin);
        psMetadataAddS32 (header, PS_LIST_TAIL, "YBIN",    0, "", psf->residuals->yBin);
        psMetadataAddS32 (header, PS_LIST_TAIL, "XCENTER", 0, "", psf->residuals->xCenter);
        psMetadataAddS32 (header, PS_LIST_TAIL, "YCENTER", 0, "", psf->residuals->yCenter);

        // write the residuals as planes of the image
        psArray *images = psArrayAllocEmpty (1);
        psArrayAdd (images, 1, psf->residuals->Ro);  // z = 0 is Ro

        if (psf->residuals->Rx) {
            psArrayAdd (images, 1, psf->residuals->Rx);
            psArrayAdd (images, 1, psf->residuals->Ry);
        }

        // note that all N plane are implicitly of the same type, so we convert the mask
        if (psf->residuals->mask) {
            psImage *mask = psImageCopy (NULL, psf->residuals->mask, psf->residuals->Ro->type.type);
            psArrayAdd (images, 1, mask);
            psFree (mask);
        }

        // psFitsWriteImageCube (file->fits, header, images, residName);
        // psFree (images);

        if (!psFitsWriteImageCube (file->fits, header, images, residName)) {
            psError(psErrorCodeLast(), false, "Unable to write PSF residuals.");
            psFree(images);
            psFree(residName);
            psFree(header);
            return false;
        }
        psFree (images);
        psFree (residName);
        psFree (header);
    }

    if (!pmPSFmodelWrite_ApTrend(file, psf)) {
	psError(psErrorCodeLast(), false, "Unable to write PSF ApTrend");
	return false;
    }

    if (!pmPSFmodelWrite_GrowthCurve(file, psf)) {
	psError(psErrorCodeLast(), false, "Unable to write PSF Growth Curve");
	return false;
    }

    // write a representation of the psf model
    {
        psMetadata *header = psMetadataAlloc ();

        int DX = 65;
        int DY = 65;

        psImage *psfMosaic = psImageAlloc (DX, DY, PS_TYPE_F32);
        psImageInit (psfMosaic, 0.0);

        pmModel *modelRef = pmModelAlloc(psf->type);

        // use the center of the center pixel of the image
        float xc = 0.5*psf->fieldNx;
        float yc = 0.5*psf->fieldNy;

        // assign the x and y coords to the image center
        // create an object with center intensity of 1000
        modelRef->params->data.F32[PM_PAR_SKY] = 0;
        modelRef->params->data.F32[PM_PAR_I0] = 1.000;
        modelRef->params->data.F32[PM_PAR_XPOS] = xc;
        modelRef->params->data.F32[PM_PAR_YPOS] = yc;

        // create modelPSF from this model
        pmModel *model = pmModelFromPSF (modelRef, psf);
        if (model) {
            // place the reference object in the image center
            pmModelAddWithOffset (psfMosaic, NULL, model, PM_MODEL_OP_FULL | PM_MODEL_OP_CENTER, 0, 0.0, 0.0);
            psFree (model);

            if (false) {
                // this call creates an extension with NAXIS3 = 3
                psArray *images = psArrayAllocEmpty (3);
                psArrayAdd (images, 1, psfMosaic);
                // psArrayAdd (images, 1, psfModel);
                // psArrayAdd (images, 1, psfModel);

                if (!psFitsWriteImageCube (file->fits, header, images, "PSF_MODEL")) {
                    psError(psErrorCodeLast(), false, "Unable to write PSF representation.");
                    psFree(images);
                    psFree(psfMosaic);
                    psFree(modelRef);
                    psFree(header);
                    return false;
                }
                psFree (images);
            } else {
                // this call creates an extension with NAXIS3 = 1
                // XXX need to replace PSF_MODEL with rule-based name like residName
                if (!psFitsWriteImage(file->fits, header, psfMosaic, 0, "PSF_MODEL")) {
                    psError(psErrorCodeLast(), false, "Unable to write PSF representation.");
                    psFree(psfMosaic);
                    psFree(modelRef);
                    psFree(header);
                    return false;
                }
            }
        }

        psFree (psfMosaic);
        psFree (modelRef);
        psFree (header);
    }

    return true;

    // XXX save the growth curve
    // XXX save ApTrend (as image?)
    // XXX write the ApTrend with the same API as will be used for the PSF parameters above

# if (0)
    // build a FITS table of the fit to the Aperture Residuals
    psArray *apresTable = psArrayAllocEmpty (100);
    psPolynomial4D *poly = psf->ApTrend;
    for (int ix = 0; ix < poly->nX; ix++) {
        for (int iy = 0; iy < poly->nY; iy++) {

            row = psMetadataAlloc ();
            psMetadataAddS32 (row, PS_LIST_TAIL, "X_POWER",    0, "", ix);
            psMetadataAddS32 (row, PS_LIST_TAIL, "Y_POWER",    0, "", iy);
            psMetadataAddF32 (row, PS_LIST_TAIL, "VALUE",      0, "", poly->coeff[ix][iy]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "ERROR",      0, "", poly->coeffErr[ix][iy]);
            psMetadataAddU8  (row, PS_LIST_TAIL, "MASK",       0, "", poly->mask[ix][iy]);

            psArrayAdd (psfTable, 100, row);
            psFree (row);
        }
    }
# endif
}



// if this file needs to have a PHU written out, write one
bool pmPSFmodelWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    // not needed if already written
    if (file->wrote_phu) return true;

    // not needed if not FPA
    // XXX this prevents us from defining a SPLIT/MEF CMF file...
    if (file->fileLevel != PM_FPA_LEVEL_FPA) return true;

    // not needed if only one chip
    if (file->fpa->chips->n == 1) return true;


    // find the FPA phu
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    if (!fpa) {
        psError(psErrorCodeLast(), false, "Unable to build FPA to write.");
        return false;
    }
    pmHDU *phu = psMemIncrRefCounter(pmFPAviewThisPHU(view, fpa));
    psFree(fpa);

    // if there is no PHU, this is a single header+image (extension-less) file. This could be
    // the case for an input SPLIT set of files being written out as a MEF.  if there is a PHU,
    // write it out as a 'blank'
    psMetadata *outhead = psMetadataAlloc();
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    } else {
        if (!pmConfigConformHeader (outhead, file->format)) {
            psError(psErrorCodeLast(), false, "Unable to conform header of PSF PHU.");
            psFree(phu);
            return false;
        }
    }
    psFree(phu);

    psMetadataAddBool (outhead, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
    if (!psFitsWriteBlank (file->fits, outhead, "")) {
        psError(psErrorCodeLast(), false, "Unable to write PHU for PSF.");
        psFree(outhead);
        return false;
    }
    file->wrote_phu = true;

    psTrace ("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree (outhead);

    return true;
}

bool pmPSFmodelReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        return pmPSFmodelReadFPA(fpa, view, file, config);
    }

    if (view->chip >= fpa->chips->n) {
        psAbort("Programming error: view does not apply to FPA.");
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        return pmPSFmodelReadChip(chip, view, file, config);
    }

    psError(PM_ERR_CONFIG, true, "PSF must be read at the chip level");
    return false;
}

// read in all chip-level PSFmodel files for this FPA
bool pmPSFmodelReadFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    bool success = true;                // Was everything successful?
    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        success &= pmPSFmodelReadChip(chip, view, file, config);
    }
    return success;
}

// read in all cell-level PSFmodel files for this chip
bool pmPSFmodelReadChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    // We need the readout as well, because that has the PSF analysis data (e.g., clumps)
    // There may be only one, because photometry is done on chip-mosaicked data.
    if (chip->cells->n != 1) {
        psError(PM_ERR_PROG, true, "Chip to receive PSF has %ld cells (should be only one)",
                chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[0]; // Cell to receive PSF
    pmReadout *ro = NULL;                // Readout to receive PSF
    if (cell->readouts->n == 0) {
        ro = pmReadoutAlloc(cell);
        psFree(ro);                     // Drop reference
    } else if (cell->readouts->n != 1) {
        psError(PM_ERR_PROG, true, "Cell to receive PSF has %ld readouts (should be only one)",
                cell->readouts->n);
        return false;
    } else {
        ro = cell->readouts->data[0];
    }
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    if (!ro->analysis) {
        ro->analysis = psMetadataAlloc();
    }

    if (!pmPSFmodelRead(chip->analysis, ro->analysis, view, file, config)) {
        psError(psErrorCodeLast(), false, "Failed to write PSF for chip");
        return false;
    }
    return true;
}

// for each Readout (ie, analysed image), we write out: header + table with PSF model parameters,
// and header + image for the PSF residual images
bool pmPSFmodelRead (psMetadata *chipAnalysis, psMetadata *roAnalysis, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_METADATA_NON_NULL(chipAnalysis, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    bool status;
    char *rule = NULL;
    psMetadata *header = NULL;

    psTrace ("psModules.objects", 5, "read psf model for %s\n", file->filename);

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, "PSPHOT");
    if (!recipe) {
        psError(PM_ERR_CONFIG, false, "missing recipe %s", "PSPHOT");
        return false;
    }

    // Menu of EXTNAME rules
    psMetadata *menu = psMetadataLookupMetadata(&status, file->camera, "EXTNAME.RULES");
    if (!menu) {
        psError(PM_ERR_CONFIG, true, "missing EXTNAME.RULES in camera.config");
        return false;
    }
    // EXTNAME for table data
    rule = psMetadataLookupStr(&status, menu, "PSF.TABLE");
    if (!rule) {
        psError(PM_ERR_CONFIG, true, "missing entry for PSF.TABLE in EXTNAME.RULES in camera.config");
        return false;
    }
    char *tableName = pmFPAfileNameFromRule (rule, file, view);
    // EXTNAME for residual images
    rule = psMetadataLookupStr(&status, menu, "PSF.RESID");
    if (!rule) {
        psError(PM_ERR_CONFIG, true, "missing entry for PSF.RESID in EXTNAME.RULES in camera.config");
        return false;
    }
    char *imageName = pmFPAfileNameFromRule (rule, file, view);

    // move fits pointer to table and read header
    // advance to the table data extension
    // since we have read the IMAGE header, the TABLE header should exist
    if (!psFitsMoveExtName(file->fits, tableName)) {
        psError(psErrorCodeLast(), false, "cannot find data extension %s in %s", tableName, file->filename);
        return false;
    }

    // load the PSF model table header
    header = psFitsReadHeader (NULL, file->fits);
    if (!header) {
        psError(psErrorCodeLast(), false, "Cannot read PSF table header.");
        return false;
    }

    pmPSFOptions *options = pmPSFOptionsAlloc();

    // load the PSF model parameters from the FITS table
    char *modelName = psMetadataLookupStr (&status, header, "PSF_NAME");
    if (!modelName) {
        psError(PS_ERR_UNKNOWN, true, "missing model name in psf file %s", file->filename);
        return false;
    }

    options->type = pmModelClassGetType (modelName);
    if (options->type == -1) {
        psError(PS_ERR_UNKNOWN, true, "invalid model name %s in psf file %s", modelName, file->filename);
        return false;
    }

    // read the psf clump data for each region
    status = false;
    if (roAnalysis) {
	status = pmPSFmodelReadPSFClump (roAnalysis, header);
	if (!status) {
	    psMetadataAddS32 (roAnalysis, PS_LIST_TAIL, "PSF.CLUMP.NREGIONS",  PS_META_REPLACE, "psf clump regions", 0);
	}
    } 
    if (!roAnalysis || !status) {
	psWarning ("no PSF.CLUMP data available for PSF model");
    }

    options->poissonErrorsPhotLMM = psMetadataLookupBool (&status, header, "ERR_LMM");
    options->poissonErrorsPhotLin = psMetadataLookupBool (&status, header, "ERR_LIN");
    options->poissonErrorsParams  = psMetadataLookupBool (&status, header, "ERR_PAR");

    options->psfFieldNx = psMetadataLookupS32 (&status, header, "IMAXIS1");
    options->psfFieldNy = psMetadataLookupS32 (&status, header, "IMAXIS2");
    options->psfFieldXo = psMetadataLookupS32 (&status, header, "IMREF1");
    options->psfFieldYo = psMetadataLookupS32 (&status, header, "IMREF2");

    psImageBinning *binning = psImageBinningAlloc();
    binning->nXfine = options->psfFieldNx;
    binning->nYfine = options->psfFieldNy;

    // we determine the PSF parameter polynomials from the MD-defined polynomials
    pmPSF *psf = pmPSFAlloc (options);

    // check the number of expected parameters
    int nPar = psMetadataLookupS32 (&status, header, "PSF_NPAR");
    if (!status) {
	psError(PS_ERR_UNKNOWN, true, "PSF file %s missing PSF_NPAR value", file->filename);
	return false;
    }

    if (nPar != pmModelClassParameterCount (psf->type))
        psAbort("mismatch model par count");

    // load the trend mode and dimensions of each parameter
    for (int i = 0; i < nPar; i++) {
        char name[PS_SMALLWORD];
        ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_NX", i);
        binning->nXruff = psMetadataLookupS32 (&status, header, name);
        if (!status) continue;          // not all parameters are defined

	ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_NY", i);
        binning->nYruff = psMetadataLookupS32 (&status, header, name);
        if (!status) {
            psError(PS_ERR_UNKNOWN, true, "inconsistent PSF header: NX defined for PAR %d, but not NY", i);
            return false;
        }

        ps_snprintf_nowarn (name, PS_SMALLWORD, "PAR%02d_MD", i);
        char *modeName = psMetadataLookupStr (&status, header, name);
        if (!status) {
            psError(PM_ERR_PROG, true, "inconsistent PSF header: NX & NY defined for PAR %d, but not MD", i);
            return false;
        }
        pmTrend2DMode psfTrendMode = pmTrend2DModeFromString (modeName);
        if (psfTrendMode == PM_TREND_NONE) {
            psfTrendMode = PM_TREND_POLY_ORD;
        }

        // XXX Attempting to guard against failing assertions on nXruff and nYruff in psImageBinningSetScale.
        // This replicates code in psphotCheckStarDistribution, where these values are generated.  Not sure
        // it's correct, though.
        if (psfTrendMode != PM_TREND_MAP) {
            binning->nXruff++;
            binning->nYruff++;
        }

        psImageBinningSetScale (binning, PS_IMAGE_BINNING_CENTER);
        psImageBinningSetSkipByOffset (binning, options->psfFieldXo, options->psfFieldYo);
        psf->params->data[i] = pmTrend2DNoImageAlloc (psfTrendMode, binning, NULL);
    }
    psFree (binning);
    psFree(options);

    // other required information describing the PSF
    psf->ApResid   = psMetadataLookupF32 (&status, header, "AP_RESID");
    psf->dApResid  = psMetadataLookupF32 (&status, header, "AP_ERROR");
    psf->chisq     = psMetadataLookupF32 (&status, header, "CHISQ");
    psf->nPSFstars = psMetadataLookupS32 (&status, header, "NSTARS");

    // XXX can we drop this now?
    psf->skyBias   = psMetadataLookupF32 (&status, header, "SKY_BIAS");

    if (roAnalysis) {
	float PSF_APERTURE =  psMetadataLookupF32(&status, header, "PSF_APERTURE");
	if (status) {
	    psMetadataAddF32 (roAnalysis, PS_LIST_TAIL, "PSF_APERTURE", PS_DATA_F32, "aperture for psf objects", PSF_APERTURE);
	}
	float PSF_FIT_RADIUS =  psMetadataLookupF32(&status, header, "PSF_FIT_RADIUS");
	if (status) {
	    psMetadataAddF32 (roAnalysis, PS_LIST_TAIL, "PSF_FIT_RADIUS", PS_DATA_F32, "aperture for psf objects", PSF_FIT_RADIUS);
	}
    } else {
	psWarning ("unable to read PSF_APERTURE or PSF_FIT_RADIUS");
    }

    // read the raw table data
    psArray *table = psFitsReadTable (file->fits);
    if (!table) {
        psError(psErrorCodeLast(), false, "Unable to read PSF table.");
        psFree(header);
        return false;
    }

    // fill in the matching psf->params entries
    for (int i = 0; i < table->n; i++) {
        psMetadata *row = table->data[i];

        int iPar = psMetadataLookupS32 (&status, row, "MODEL_TERM");

        pmTrend2D *trend = psf->params->data[iPar];
        if (trend == NULL) {
            psError(PS_ERR_UNKNOWN, true, "parameter %d not available", iPar);
            return false;
        }

	pmTrend2DfromTableRow(trend, row);
    }
    psFree (header);
    psFree (table);

    // move fits pointer to residual image and read header
    // advance to the table data extension
    // since we have read the IMAGE header, the TABLE header should exist
    if (!psFitsMoveExtName (file->fits, imageName)) {
        psError(psErrorCodeLast(), false, "Cannot find PSF data extension %s in %s",
                imageName, file->filename);
        return false;
    }

    header = psFitsReadHeader (NULL, file->fits);
    if (!header) {
        psError(psErrorCodeLast(), false, "Unable to read PSF header.");
        return false;
    }
    int Naxis = psMetadataLookupS32 (&status, header, "NAXIS");
    if (Naxis != 0) {

        int Nx = psMetadataLookupS32 (&status, header, "NAXIS1");
        int Ny = psMetadataLookupS32 (&status, header, "NAXIS2");
        int Nz = psMetadataLookupS32 (&status, header, "NAXIS3");

        int xBin  = psMetadataLookupS32 (&status, header, "XBIN");
	if (!status) {
            psError(psErrorCodeLast(), false, "XBIN not set in PSF residual image header.");
            return false;
        }
        int yBin  = psMetadataLookupS32 (&status, header, "YBIN");
	if (!status) {
            psError(psErrorCodeLast(), false, "YBIN not set in PSF residual image header.");
            return false;
        }

        int xSize = Nx / xBin;
        int ySize = Ny / yBin;

        psf->residuals = pmResidualsAlloc (xSize, ySize, xBin, yBin);

        psf->residuals->xCenter = psMetadataLookupS32 (&status, header, "XCENTER");
        psf->residuals->yCenter = psMetadataLookupS32 (&status, header, "YCENTER");

        psRegion fullImage = {0, 0, 0, 0};
        if (!psFitsReadImageBuffer(psf->residuals->Ro, file->fits, fullImage, 0)) {
            psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
            return false;
        }

        // note that all N plane are implicitly of the same type, so we convert the mask
        psImage *mask = psImageCopy(NULL, psf->residuals->mask, psf->residuals->Ro->type.type);
        psImageInit (psf->residuals->mask, 0);
        psImageInit (psf->residuals->Rx, 0.0);
        psImageInit (psf->residuals->Ry, 0.0);
        switch (Nz) {
          case 1: // Ro only
            break;
          case 2: // Ro and mask
            if (!psFitsReadImageBuffer(mask, file->fits, fullImage, 1)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            psImageCopy (psf->residuals->mask, mask, PM_TYPE_RESID_MASK);
            break;
          case 3: // Ro, Rx and Ry, no mask
            if (!psFitsReadImageBuffer(psf->residuals->Rx, file->fits, fullImage, 1)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            if (!psFitsReadImageBuffer(psf->residuals->Ry, file->fits, fullImage, 2)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            break;
          case 4: // Ro, Rx, Ry, and mask:
            if (!psFitsReadImageBuffer(psf->residuals->Rx, file->fits, fullImage, 1)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            if (!psFitsReadImageBuffer(psf->residuals->Ry, file->fits, fullImage, 2)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            if (!psFitsReadImageBuffer(mask, file->fits, fullImage, 3)) {
                psError(psErrorCodeLast(), false, "Unable to read PSF residual image.");
                return false;
            }
            psImageCopy (psf->residuals->mask, mask, PM_TYPE_RESID_MASK);
            break;
        }
        psFree (mask);
    }

    if (!pmPSFmodelRead_ApTrend (psf, file)) {
	psError(psErrorCodeLast(), false, "Unable to read PSF ApTrend data.");
	return false;
    }

    if (!pmPSFmodelRead_GrowthCurve(psf, file)) {
	psError(psErrorCodeLast(), false, "Unable to read PSF Growth Curve");
	return false;
    }

    psMetadataAdd (chipAnalysis, PS_LIST_TAIL, "PSPHOT.PSF",     PS_DATA_UNKNOWN,  "psphot psf", psf);
    psFree (psf);

    psFree (tableName);
    psFree (imageName);
    psFree (header);

    return true;
}

// write aperture trend to a FITS table
bool pmPSFmodelWrite_ApTrend (pmFPAfile *file, pmPSF *psf) {

    pmTrend2D *trend = psf->ApTrend;
    if (trend == NULL) { 
	psWarning ("no PSF ApTrend to write out, skipping");
	return true; 
    }

    // we need to write a header for the table,
    psMetadata *header = psMetadataAlloc();

    int nX = 0, nY = 0;
    if (trend->mode == PM_TREND_MAP) {
	nX = trend->map->map->numCols;
	nY = trend->map->map->numRows;
    } else {
	nX = trend->poly->nX;
	nY = trend->poly->nY;
    }
    psMetadataAddS32 (header, PS_LIST_TAIL, "TREND_NX", 0, "", nX);
    psMetadataAddS32 (header, PS_LIST_TAIL, "TREND_NY", 0, "", nY);
    char *modeName = pmTrend2DModeToString (trend->mode);
    psMetadataAddStr (header, PS_LIST_TAIL, "TREND_MD", 0, "", modeName);
    psFree (modeName);

    // build a FITS table of the ApTrend (only 1)
    psArray *table = psArrayAllocEmpty (100);
    pmTrend2DtoTable (table, trend, "APTREND", 0);

    // write an empty FITS segment if we have no PSF information
    if (table->n == 0) {
	psError(PM_ERR_PROG, true, "No PSF data to write.");
	psFree(table);
	psFree(header);
	return false;
    } 

    psTrace ("pmFPAfile", 5, "writing psf ApTrend data %s\n", "AP_TREND");
    if (!psFitsWriteTable(file->fits, header, table, "AP_TREND")) {
	psError(psErrorCodeLast(), false, "Error writing psf table data %s\n", "AP_TREND");
	psFree(table);
	psFree(header);
	return false;
    }

    psFree (table);
    psFree (header);
    return true;
}

// read aperture trend to a FITS table
bool pmPSFmodelRead_ApTrend (pmPSF *psf, pmFPAfile *file) {

    bool status;

    // move fits pointer to AP_TREND section
    // advance to the table data extension
    if (!psFitsMoveExtNameClean (file->fits, "AP_TREND")) {
	psWarning ("no Aperture Trend data in PSF file, skipping");
	return true;
    }

    psMetadata *header = psFitsReadHeader (NULL, file->fits);
    if (!header) {
	psError(psErrorCodeLast(), false, "Unable to read AP_TREND header.");
	return false;
    }
	
    // read the raw table data
    psArray *table = psFitsReadTable (file->fits);
    if (!table) {
	psError(psErrorCodeLast(), false, "Unable to read AP_TREND table.");
	psFree(header);
	return false;
    }

    // XXX allow user to set this optionally?
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);

    psImageBinning *binning = psImageBinningAlloc();
    binning->nXfine = psf->fieldNx;
    binning->nYfine = psf->fieldNy;
    binning->nXruff = psMetadataLookupS32 (&status, header, "TREND_NX");
    binning->nYruff = psMetadataLookupS32 (&status, header, "TREND_NY");
    psImageBinningSetScale (binning, PS_IMAGE_BINNING_CENTER);
    char *modeName  = psMetadataLookupStr (&status, header, "TREND_MD");
    if (!status) {
	psError(PM_ERR_PROG, true, "inconsistent PSF header: NX & NY defined for AP TREND, but not MD");
	psFree (header);
	psFree (stats);
	psFree (table);
	return false;
    }
    pmTrend2DMode psfTrendMode = pmTrend2DModeFromString (modeName);
    if (psfTrendMode == PM_TREND_NONE) {
	psfTrendMode = PM_TREND_POLY_ORD;
    }

    // measure Trend2D for the current spatial scale
    pmTrend2D *apTrend = pmTrend2DNoImageAlloc (PM_TREND_MAP, binning, stats);

    // fill in the matching psf->params entries
    for (int i = 0; i < table->n; i++) {
	psMetadata *row = table->data[i];
	pmTrend2DfromTableRow(apTrend, row);
    }
    psf->ApTrend = apTrend;

    psFree (binning);
    psFree (header);
    psFree (stats);
    psFree (table);
    return true;
}

// write aperture trend to a FITS table
bool pmPSFmodelWrite_GrowthCurve (pmFPAfile *file, pmPSF *psf) {

    pmGrowthCurve *growth = psf->growth;
    if (growth == NULL) { 
	psWarning ("no PSF Growth Curve to write out, skipping");
	return true; 
    }

    // we need to write a header for the table,
    psMetadata *header = psMetadataAlloc();

    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_MIN_RAD", 0, "", growth->radius->data.F32[0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_MAX_RAD", 0, "", growth->maxRadius);
    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_REF_RAD", 0, "", growth->refRadius);
    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_AP_LOSS", 0, "", growth->apLoss);
    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_AP_REF",  0, "", growth->apRef);
    psMetadataAddF32 (header, PS_LIST_TAIL, "GROWTH_FIT_MAG", 0, "", growth->fitMag);

    // build a FITS table of the ApTrend (only 1)
    psArray *table = psArrayAllocEmpty (100);
    for (int i = 0; i < growth->apMag->n; i++) {
	psMetadata *row = psMetadataAlloc ();
	psMetadataAddF32 (row, PS_LIST_TAIL, "RADIUS", 0, "", growth->radius->data.F32[i]);
	psMetadataAddF32 (row, PS_LIST_TAIL, "AP_MAG", 0, "", growth->apMag->data.F32[i]);
	psArrayAdd (table, 100, row);
	psFree (row);
    }

    // write an empty FITS segment if we have no PSF information
    if (table->n == 0) {
	psError(PM_ERR_PROG, true, "No PSF data to write.");
	psFree(table);
	psFree(header);
	return false;
    } 

    psTrace ("pmFPAfile", 5, "writing psf Growth Curve data %s\n", "GROWTH_CURVE");
    if (!psFitsWriteTable(file->fits, header, table, "GROWTH_CURVE")) {
	psError(psErrorCodeLast(), false, "Error writing psf table data %s\n", "GROWTH_CURVE");
	psFree(table);
	psFree(header);
	return false;
    }

    psFree (table);
    psFree (header);
    return true;
}

// read aperture trend to a FITS table
bool pmPSFmodelRead_GrowthCurve (pmPSF *psf, pmFPAfile *file) {

    bool status;

    // move fits pointer to AP_TREND section
    // advance to the table data extension
    if (!psFitsMoveExtNameClean (file->fits, "GROWTH_CURVE")) {
	psWarning ("no Growth Curve data in PSF file, skipping");
	return true;
    }

    psMetadata *header = psFitsReadHeader (NULL, file->fits);
    if (!header) {
	psError(psErrorCodeLast(), false, "Unable to read GROWTH_CURVE header.");
	return false;
    }
	
    // read the raw table data
    psArray *table = psFitsReadTable (file->fits);
    if (!table) {
	psError(psErrorCodeLast(), false, "Unable to read GROWTH_CURVE table.");
	psFree(header);
	return false;
    }

    float minRadius = psMetadataLookupF32 (&status, header, "GROWTH_MIN_RAD"); if (!status) return false;
    float maxRadius = psMetadataLookupF32 (&status, header, "GROWTH_MAX_RAD"); if (!status) return false;
    float refRadius = psMetadataLookupF32 (&status, header, "GROWTH_REF_RAD"); if (!status) return false;

    psf->growth = pmGrowthCurveAlloc(minRadius, maxRadius, refRadius);

    psf->growth->apLoss = psMetadataLookupF32 (&status, header, "GROWTH_AP_LOSS"); if (!status) return false;
    psf->growth->apRef  = psMetadataLookupF32 (&status, header, "GROWTH_AP_REF"); if (!status) return false;
    psf->growth->fitMag = psMetadataLookupF32 (&status, header, "GROWTH_FIT_MAG"); if (!status) return false;

    // fill in the matching psf->params entries
    for (int i = 0; i < table->n; i++) {
	psMetadata *row = table->data[i];
	psf->growth->apMag->data.F32[i] = psMetadataLookupF32 (&status, row, "AP_MAG"); if (!status) return false;
    }

    psFree (header);
    psFree (table);
    return true;
}

bool pmPSFmodelReadPSFClump (psMetadata *analysis, psMetadata *header) {

    bool status = false;;

    int nRegions = psMetadataLookupS32 (&status, header, "PSF_CLN");
    if (!status) {
	// read old-style psf clump data

	char regionName[PS_BIGWORD];
	ps_snprintf_nowarn (regionName, PS_BIGWORD, "PSF.CLUMP.REGION.000");
	psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);

	if (!regionMD) {
	    regionMD = psMetadataAlloc();
	    psMetadataAddMetadata (analysis, PS_LIST_TAIL, regionName, PS_META_REPLACE, "psf clump region", regionMD);
	    psFree (regionMD);
	}

	// psf clump data
	pmPSFClump psfClump;
	psfClump.X  = psMetadataLookupF32 (&status, header, "PSF_CLX" );  if (!status) return false;
	psfClump.Y  = psMetadataLookupF32 (&status, header, "PSF_CLY" );  if (!status) return false;
	psfClump.dX = psMetadataLookupF32 (&status, header, "PSF_CLDX");  if (!status) return false;
	psfClump.dY = psMetadataLookupF32 (&status, header, "PSF_CLDY");  if (!status) return false;

	psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.X",  PS_META_REPLACE, "psf clump center", psfClump.X);
	psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.Y",  PS_META_REPLACE, "psf clump center", psfClump.Y);
	psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DX", PS_META_REPLACE, "psf clump center", psfClump.dX);
	psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DY", PS_META_REPLACE, "psf clump center", psfClump.dY);
	psMetadataAddS32 (analysis, PS_LIST_TAIL, "PSF.CLUMP.NREGIONS",  PS_META_REPLACE, "psf clump regions", 1);
    } else {
	for (int i = 0; i < nRegions; i++) {
	    char key[PS_SMALLWORD];
	    char regionName[PS_BIGWORD];
	    ps_snprintf_nowarn (regionName, PS_BIGWORD, "PSF.CLUMP.REGION.%03d", i);

	    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);
	    if (!regionMD) {
		regionMD = psMetadataAlloc();
		psMetadataAddMetadata (analysis, PS_LIST_TAIL, regionName, PS_META_REPLACE, "psf clump region", regionMD);
		psFree (regionMD);
	    }

	    // psf clump data
	    pmPSFClump psfClump;

	    ps_snprintf_nowarn (key, PS_SMALLWORD, "CLX_%03d", i);
	    psfClump.X  = psMetadataLookupF32 (&status, header, key);  if (!status) return false;
	    ps_snprintf_nowarn (key, PS_SMALLWORD, "CLY_%03d", i);
	    psfClump.Y  = psMetadataLookupF32 (&status, header, key);  if (!status) return false;
	    ps_snprintf_nowarn (key, PS_SMALLWORD, "CLDX_%03d", i);
	    psfClump.dX = psMetadataLookupF32 (&status, header, key);  if (!status) return false;
	    ps_snprintf_nowarn (key, PS_SMALLWORD, "CLDY_%03d", i);
	    psfClump.dY = psMetadataLookupF32 (&status, header, key);  if (!status) return false;

	    psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.X",  PS_META_REPLACE, "psf clump center", psfClump.X);
	    psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.Y",  PS_META_REPLACE, "psf clump center", psfClump.Y);
	    psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DX", PS_META_REPLACE, "psf clump center", psfClump.dX);
	    psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DY", PS_META_REPLACE, "psf clump center", psfClump.dY);
	}
	psMetadataAddS32 (analysis, PS_LIST_TAIL, "PSF.CLUMP.NREGIONS",  PS_META_REPLACE, "psf clump regions", nRegions);
    }
    return true;
}
