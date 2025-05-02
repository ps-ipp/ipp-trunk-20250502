/** @file  pmAstrometryModel.c
 *
 *  @brief Functions to read and write astrometric model
 *
 *  The generic model does not specify the location of the boresite on the sky, and it includes
 *  a model for the rotator and motion of the boresite.
 *
 *  @ingroup AstroImage
 *
 *  @author EAM, IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-09-17 23:07:02 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>   // for unlink
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAExtent.h"
#include "pmFPAfileFitsIO.h"
#include "pmConcepts.h"
#include "pmAstrometryWCS.h"
#include "pmAstrometryUtils.h"
#include "pmAstrometryRegions.h"
#include "pmAstrometryModel.h"

# define REQUIRE(TEST,MESSAGE){ if (!(TEST)) { psAbort (MESSAGE); }}

/********************* CheckDataStatus functions *****************************/

bool pmAstromModelCheckDataStatusForView (const pmFPAview *view, pmFPAfile *file) {

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        bool exists = pmAstromModelCheckDataStatusForFPA (fpa);
        return exists;
    }
    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool exists = pmAstromModelCheckDataStatusForChip (chip);
        return exists;
    }
    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    psError(PS_ERR_IO, false, "Astrometry only valid at the chip level");
    return false;
}

bool pmAstromModelCheckDataStatusForFPA (const pmFPA *fpa) {

    if (!fpa->toTPA) return false;
    if (!fpa->fromTPA) return false;
    if (!fpa->toSky) return false;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        if (pmAstromModelCheckDataStatusForChip (chip)) return true;
    }
    return false;
}

bool pmAstromModelCheckDataStatusForChip (const pmChip *chip) {

    if (!chip->toFPA) return false;
    if (!chip->fromFPA) return false;  // XXX not strictly needed?
    return true;
}

/********************* Write Data functions *****************************/

bool pmAstromModelWriteForView (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    // write the full model in one pass: require the level to be FPA
    if (view->chip != -1) {
        psError(PS_ERR_IO, false, "Astrometry must be written at the FPA level");
        return false;
    }

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (!pmAstromModelWriteFPA(file, fpa)) {
        psError(PS_ERR_IO, false, "Failed to write Astrometry for fpa");
        psFree(fpa);
        return false;
    }

    psFree(fpa);

    return true;
}

// write out all chip-level Astrometry data for this FPA
bool pmAstromModelWriteFPA (pmFPAfile *file, const pmFPA *fpa)
{


    if (!pmAstromModelWritePHU (file, fpa)) {
        psError(PS_ERR_IO, false, "Failed to write PHU for Astrometry model");
        return false;
    }

    if (!pmAstromModelWriteChips (file)) {
        psError(PS_ERR_IO, false, "Failed to write Astrometry for chips");
        return false;
    }

    if (!pmAstromModelWriteFP (file)) {
        psError(PS_ERR_IO, false, "Failed to write Sky for Astrometry model");
        return false;
    }

    if (!pmAstromModelWriteTP (file)) {
        psError(PS_ERR_IO, false, "Failed to write Sky for Astrometry model");
        return false;
    }

    if (!pmAstromModelWriteSky (file)) {
        psError(PS_ERR_IO, false, "Failed to write Sky for Astrometry model");
        return false;
    }

    return true;
}

bool pmAstromModelWritePHU (pmFPAfile *file, const pmFPA *fpa) {
    // Need to have an FPA suitable for writing, so that the headers are all kosher

    // output header data
    psMetadata *outhead = psMetadataAlloc();

    // use the FPA phu to generate the PHU header
    pmHDU *phu = fpa->hdu;

    // if there is no FPA PHU, this is a single header+image (extension-less) file. This could be
    // the case for an input SPLIT set of files being written out as a MEF.  if there is a PHU,
    // write it out as a 'blank'
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    } else {
        pmConfigConformHeader (outhead, file->format);
    }

    psMetadataAddBool (outhead, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
    psFitsWriteBlank (file->fits, outhead, "");
    file->wrote_phu = true;

    psTrace ("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree (outhead);

    return true;
}

// fourth layer holds the chips
bool pmAstromModelWriteChips (pmFPAfile *file) {

    psMetadata *header = psMetadataAlloc();
    psMetadataAddStr(header, PS_LIST_TAIL, "COORD",    PS_META_REPLACE, "name of this layer",   "CHIPS");
    psMetadataAddStr(header, PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "FOCAL_PLANE");
    psMetadataAddStr(header, PS_LIST_TAIL, "BOUNDARY", PS_META_REPLACE, "validity region",      "RECTANGLE");
    psMetadataAddStr(header, PS_LIST_TAIL, "TRANSFRM", PS_META_REPLACE, "mapping to parent",    "POLYNOMIAL");

    psArray *model = psArrayAllocEmpty (1);

    pmFPAview *view = pmFPAviewAlloc (0);

    pmChip *chip = NULL;
    while ((chip = pmFPAviewNextChip (view, file->fpa, 1)) != NULL) {

        if (!chip->toFPA) continue;
        assert (chip->toFPA->x);
        assert (chip->toFPA->y);

        psRegion *region = pmChipPixels (chip);

        // set the chip name
        char *chiprule = psStringCopy ("{CHIP.NAME}");
        char *chipname = pmFPAfileNameFromRule (chiprule, file, view);

        for (int i = 0; i <= chip->toFPA->x->nX; i++) {
            for (int j = 0; j <= chip->toFPA->x->nY; j++) {
                psMetadata *row = psMetadataAlloc ();

                psMetadataAddStr(row,    PS_LIST_TAIL, "SEGMENT",  PS_META_REPLACE, "name of this segment", chipname);
                psMetadataAddStr(row,    PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "FOCAL_PLANE");
                psMetadataAddF32(row,    PS_LIST_TAIL, "MINX",     PS_META_REPLACE, "range", region->x0);
                psMetadataAddF32(row,    PS_LIST_TAIL, "MAXX",     PS_META_REPLACE, "range", region->x1);
                psMetadataAddF32(row,    PS_LIST_TAIL, "MINY",     PS_META_REPLACE, "range", region->y0);
                psMetadataAddF32(row,    PS_LIST_TAIL, "MAXY",     PS_META_REPLACE, "range", region->y1);

                psMetadataAddS32(row,    PS_LIST_TAIL, "XORDER",   PS_META_REPLACE, "", i);
                psMetadataAddS32(row,    PS_LIST_TAIL, "YORDER",   PS_META_REPLACE, "", j);
                psMetadataAddS32(row,    PS_LIST_TAIL, "NXORDER",  PS_META_REPLACE, "", chip->toFPA->x->nX);
                psMetadataAddS32(row,    PS_LIST_TAIL, "NYORDER",  PS_META_REPLACE, "", chip->toFPA->x->nY);
                psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_X",   PS_META_REPLACE, "", chip->toFPA->x->coeff[i][j]);
                psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_Y",   PS_META_REPLACE, "", chip->toFPA->y->coeff[i][j]);
                psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_X",  PS_META_REPLACE, "", chip->toFPA->x->coeffErr[i][j]);
                psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_Y",  PS_META_REPLACE, "", chip->toFPA->y->coeffErr[i][j]);
                psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_X",   PS_META_REPLACE, "", chip->toFPA->x->coeffMask[i][j]);
                psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_Y",   PS_META_REPLACE, "", chip->toFPA->y->coeffMask[i][j]);
                psArrayAdd (model, 100, row);
                psFree (row);
            }
        }
        psFree (chiprule);
        psFree (chipname);
        psFree (region);
    }

    if (!psFitsWriteTable (file->fits, header, model, "CHIPS")) {
        psError(PS_ERR_IO, false, "writing sky data\n");
        psFree(model);
        return false;
    }

    psFree (view);
    psFree (model);
    psFree (header);
    return true;
}

// third layer is the focal plane
bool pmAstromModelWriteFP (pmFPAfile *file) {

    psMetadata *header = psMetadataAlloc();
    psMetadataAddStr(header, PS_LIST_TAIL, "COORD",    PS_META_REPLACE, "name of this layer",   "FOCAL_PLANE");
    psMetadataAddStr(header, PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "TANGENT_PLANE");
    psMetadataAddStr(header, PS_LIST_TAIL, "BOUNDARY", PS_META_REPLACE, "validity region",      "RECTANGLE");
    psMetadataAddStr(header, PS_LIST_TAIL, "TRANSFRM", PS_META_REPLACE, "mapping to parent",    "POLYNOMIAL");

    psArray *model = psArrayAllocEmpty (1);

    // region over which the fromTPA projection is valid
    psRegion *region = pmAstromFPInTP (file->fpa);

    psPlaneTransform *toTPA   = file->fpa->toTPA;

    for (int i = 0; i <= toTPA->x->nX; i++) {
        for (int j = 0; j <= toTPA->x->nY; j++) {
            psMetadata *row = psMetadataAlloc ();
            psMetadataAddStr(row,    PS_LIST_TAIL, "SEGMENT",  PS_META_REPLACE, "name of this segment", "FOCAL_PLANE");
            psMetadataAddStr(row,    PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "TANGENT_PLANE");
            psMetadataAddF32(row,    PS_LIST_TAIL, "MINX",     PS_META_REPLACE, "range", region->x0);
            psMetadataAddF32(row,    PS_LIST_TAIL, "MAXX",     PS_META_REPLACE, "range", region->x1);
            psMetadataAddF32(row,    PS_LIST_TAIL, "MINY",     PS_META_REPLACE, "range", region->y0);
            psMetadataAddF32(row,    PS_LIST_TAIL, "MAXY",     PS_META_REPLACE, "range", region->y1);

            psMetadataAddS32(row,    PS_LIST_TAIL, "XORDER",   PS_META_REPLACE, "", i);
            psMetadataAddS32(row,    PS_LIST_TAIL, "YORDER",   PS_META_REPLACE, "", j);
            psMetadataAddS32(row,    PS_LIST_TAIL, "NXORDER",  PS_META_REPLACE, "", toTPA->x->nX);
            psMetadataAddS32(row,    PS_LIST_TAIL, "NYORDER",  PS_META_REPLACE, "", toTPA->x->nY);
            psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_X",   PS_META_REPLACE, "", toTPA->x->coeff[i][j]);
            psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_Y",   PS_META_REPLACE, "", toTPA->y->coeff[i][j]);
            psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_X",  PS_META_REPLACE, "", toTPA->x->coeffErr[i][j]);
            psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_Y",  PS_META_REPLACE, "", toTPA->y->coeffErr[i][j]);
            psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_X",   PS_META_REPLACE, "", toTPA->x->coeffMask[i][j]);
            psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_Y",   PS_META_REPLACE, "", toTPA->y->coeffMask[i][j]);

            psArrayAdd (model, 100, row);
            psFree (row);
        }
    }

    if (!psFitsWriteTable (file->fits, header, model, "FP")) {
        psError(PS_ERR_IO, false, "writing sky data\n");
        psFree(model);
        psFree (header);
        psFree (region);
        return false;
    }

    psFree (model);
    psFree (header);
    psFree (region);
    return true;
}

// second layer is the tangent plane
bool pmAstromModelWriteTP (pmFPAfile *file) {

    bool status;

    // get the boresite model parameters.  these track the position of the boresite
    // as a function of the rotator angle
    float Xo = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.X0");
    float Yo = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.Y0");
    float RX = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.RX");
    float RY = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.RY");
    float To = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.T0");
    float Po = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.BORE.P0");

    // the PosZero is the offset between the reported and actual POSANGLE values
    float PosZero = psMetadataLookupF32 (&status, file->fpa->concepts, "FPA.POS_ZERO");  /// XXX be consistent with degrees v radians
    // Indicate which direction the position angle goes
    int rotParity = psMetadataLookupS32 (&status, file->fpa->concepts, "FPA.ROT_PARITY");
    char *refChip = psMetadataLookupStr (&status, file->fpa->concepts, "FPA.REF.CHIP");

    psMetadata *header = psMetadataAlloc();
    psMetadataAddStr(header, PS_LIST_TAIL, "COORD",    PS_META_REPLACE, "name of this layer",   "TANGENT_PLANE");
    psMetadataAddStr(header, PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "SKY");
    psMetadataAddStr(header, PS_LIST_TAIL, "BOUNDARY", PS_META_REPLACE, "validity region",      "RECTANGLE");
    psMetadataAddStr(header, PS_LIST_TAIL, "TRANSFRM", PS_META_REPLACE, "mapping to parent",    "PROJECTION");

    psArray *model = psArrayAllocEmpty (1);
    psMetadata *row = psMetadataAlloc ();
    psMetadataAddStr(row,    PS_LIST_TAIL, "SEGMENT",  PS_META_REPLACE, "name of this segment", "TANGENT_PLANE");
    psMetadataAddStr(row,    PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "SKY");

    psRegion *region = pmAstromFPAExtent (file->fpa);
    psMetadataAddF32(row,    PS_LIST_TAIL, "MINX",     PS_META_REPLACE, "range", region->x0);
    psMetadataAddF32(row,    PS_LIST_TAIL, "MAXX",     PS_META_REPLACE, "range", region->x1);
    psMetadataAddF32(row,    PS_LIST_TAIL, "MINY",     PS_META_REPLACE, "range", region->y0);
    psMetadataAddF32(row,    PS_LIST_TAIL, "MAXY",     PS_META_REPLACE, "range", region->y1);

    psMetadataAddF32(row,    PS_LIST_TAIL, "XSCALE",   PS_META_REPLACE, "", file->fpa->toSky->Xs * PS_DEG_RAD);
    psMetadataAddF32(row,    PS_LIST_TAIL, "YSCALE",   PS_META_REPLACE, "", file->fpa->toSky->Ys * PS_DEG_RAD);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_X0",  PS_META_REPLACE, "boresite parameter", Xo);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_Y0",  PS_META_REPLACE, "boresite parameter", Yo);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_RX",  PS_META_REPLACE, "boresite parameter", RX);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_RY",  PS_META_REPLACE, "boresite parameter", RY);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_T0",  PS_META_REPLACE, "boresite parameter", To);
    psMetadataAddF32(row,    PS_LIST_TAIL, "BORE_P0",  PS_META_REPLACE, "boresite parameter", Po);

    psMetadataAddF32(row,    PS_LIST_TAIL, "POS_ZERO", PS_META_REPLACE, "POSANGLE offset (degrees)", PosZero);
    psMetadataAddS32(row,    PS_LIST_TAIL, "ROT_PARITY", PS_META_REPLACE, "rotator parity", rotParity);
    psMetadataAddStr(row,    PS_LIST_TAIL, "REF_CHIP", PS_META_REPLACE, "reference chip for model", refChip);

    psArrayAdd (model, 100, row);
    psFree (row);

    if (!psFitsWriteTable (file->fits, header, model, "TP")) {
        psError(PS_ERR_IO, false, "writing sky data\n");
        psFree (region);
        psFree (model);
        psFree (header);
        return false;
    }

    psFree (region);
    psFree (model);
    psFree (header);
    return (true);
}

// first layer is the sky
bool pmAstromModelWriteSky (pmFPAfile *file) {

    psMetadata *header = psMetadataAlloc();
    psMetadataAddStr(header, PS_LIST_TAIL, "COORD",    PS_META_REPLACE, "name of this layer",   "SKY");
    psMetadataAddStr(header, PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "NONE");
    psMetadataAddStr(header, PS_LIST_TAIL, "BOUNDARY", PS_META_REPLACE, "validity region",      "NONE");
    psMetadataAddStr(header, PS_LIST_TAIL, "TRANSFRM", PS_META_REPLACE, "mapping to parent",    "NONE");

    psArray *model = psArrayAllocEmpty (1);
    psMetadata *row = psMetadataAlloc ();
    psMetadataAddStr(row,    PS_LIST_TAIL, "SEGMENT",  PS_META_REPLACE, "name of this segment", "SKY");
    psMetadataAddStr(row,    PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "NONE");

    psArrayAdd (model, 100, row);
    psFree (row);

    if (!psFitsWriteTable (file->fits, header, model, "SKY")) {
        psError(PS_ERR_IO, false, "writing sky data\n");
        psFree(model);
        return false;
    }

    psFree (model);
    psFree (header);
    return (true);
}

/********************* Read Data functions *****************************/

bool pmAstromModelReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
    {

        // write the full model in one pass: require the level to be FPA
        if (view->chip != -1) {
            psError(PS_ERR_IO, false, "Astrometry must be read at the FPA level");
            return false;
        }

        if (!pmAstromModelReadFPA (file)) {
            psError(PS_ERR_IO, false, "Failed to read Astrometry for fpa");
            return false;
        }
        return true;
    }

// read out all chip-level Astrometry data for this FPA
bool pmAstromModelReadFPA (pmFPAfile *file) {

    if (!pmAstromModelReadPHU (file)) {
        psError(PS_ERR_IO, false, "Failed to read PHU for Astrometry model");
        return false;
    }

    if (!pmAstromModelReadChips (file)) {
        psError(PS_ERR_IO, false, "Failed to read Astrometry for chips");
        return false;
    }

    if (!pmAstromModelReadFP (file)) {
        psError(PS_ERR_IO, false, "Failed to read Sky for Astrometry model");
        return false;
    }

    // NOTE : TP must come after FP as it applies the POS, ROT boresite corrections to the
    // transformation determined in FP
    if (!pmAstromModelReadTP (file)) {
        psError(PS_ERR_IO, false, "Failed to read Sky for Astrometry model");
        return false;
    }

    if (!pmAstromModelReadSky (file)) {
        psError(PS_ERR_IO, false, "Failed to read Sky for Astrometry model");
        return false;
    }

    return true;
}

bool pmAstromModelReadPHU (pmFPAfile *file) {

    // not necessary to read the PHU
    return true;
}

// first layer converts Chip to Focal Plane
bool pmAstromModelReadChips (pmFPAfile *file) {

    bool status;

    int ExtraOrders = pmAstrometryGetExtraOrders();

    // set FITS cursor
    if (!psFitsMoveExtName (file->fits, "CHIPS")) {
        psError(PS_ERR_IO, false, "missing CHIPS extension in astrometry model\n");
        return false;
    }

    // free exising tranformations in prep for new alloc below
    for (int i = 0; i < file->fpa->chips->n; i++) {
        pmChip *chip = file->fpa->chips->data[i];
        psFree (chip->toFPA);
        chip->toFPA = NULL;
    }

    // load the header
    psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!header) psAbort("cannot read model header");

    // load the full model in one shot
    psArray *model = psFitsReadTable (file->fits);
    if (!model) psAbort("cannot read model");
    psLogMsg ("psModules.astrom", 4, "read %ld rows from FP\n", model->n);

    // parse the model entries
    for (int i = 0; i < model->n; i++) {
        psMetadata *row = model->data[i];

        // name of the chip for this row.
        char *chipname = psMetadataLookupStr (&status, row, "SEGMENT");

        // get chip from name
        pmChip *chip = pmConceptsChipFromName (file->fpa, chipname);
        REQUIRE (chip, "invalid chip name");

        // define the toFPA transform if not already defined
        int nX = psMetadataLookupS32(&status, row, "NXORDER"); REQUIRE (status, "missing NXORDER");
        int nY = psMetadataLookupS32(&status, row, "NYORDER"); REQUIRE (status, "missing NYORDER");
        if (chip->toFPA == NULL) {
	    chip->toFPA = psPlaneTransformAlloc(nX, nY, PS_POLYNOMIAL_ORD); // chip->fpa uses ordinary poly
        } else {
            REQUIRE (chip->toFPA->x->nX == nX, "mismatch in chip order");
            REQUIRE (chip->toFPA->x->nY == nY, "mismatch in chip order");
            REQUIRE (chip->toFPA->y->nX == nX, "mismatch in chip order");
            REQUIRE (chip->toFPA->y->nY == nY, "mismatch in chip order");
        }

        int ix = psMetadataLookupS32(&status, row, "XORDER");  REQUIRE (status, "missing XORDER");
        int iy = psMetadataLookupS32(&status, row, "YORDER");  REQUIRE (status, "missing YORDER");

        chip->toFPA->x->coeff[ix][iy]    = psMetadataLookupF32(&status, row, "POLY_X");
        chip->toFPA->y->coeff[ix][iy]    = psMetadataLookupF32(&status, row, "POLY_Y");
        chip->toFPA->x->coeffErr[ix][iy] = psMetadataLookupF32(&status, row, "ERROR_X");
        chip->toFPA->y->coeffErr[ix][iy] = psMetadataLookupF32(&status, row, "ERROR_Y");
        chip->toFPA->x->coeffMask[ix][iy] = psMetadataLookupU8(&status, row, "MASK_X");
        chip->toFPA->y->coeffMask[ix][iy] = psMetadataLookupU8(&status, row, "MASK_Y");
    }

    // convert the toFPA transfomations to fromFPA transformations
    for (int i = 0; i < file->fpa->chips->n; i++) {
        pmChip *chip = file->fpa->chips->data[i];
        if (!chip->toFPA) continue;
        psRegion *region = pmChipPixels (chip);

	// as of r40806, psPlaneTransformInvert supplies the extra order (if non-linear)
        psFree (chip->fromFPA);
        chip->fromFPA = psPlaneTransformInvert(NULL, chip->toFPA, *region, 50, ExtraOrders);
        psFree (region);
    }

    psFree (model);
    psFree (header);
    return true;
}

// second layer converts Focal Plane to Tangent Plane (unrotated)
bool pmAstromModelReadFP (pmFPAfile *file) {

    bool status;

    int ExtraOrders = pmAstrometryGetExtraOrders();

    if (!psFitsMoveExtName (file->fits, "FP")) {
        psError(PS_ERR_IO, false, "missing FP extension in astrometry model\n");
        return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!header) psAbort("cannot read model header");

    // free the old
    psFree (file->fpa->toTPA);
    file->fpa->toTPA = NULL;

    // read the complete model data at one shot
    psArray *model = psFitsReadTable (file->fits);
    psLogMsg ("psModules.astrom", 4, "read %ld rows from FP\n", model->n);

    // parse the model
    for (int i = 0; i < model->n; i++) {
        psMetadata *row = model->data[i];

        // there is only one transformation in this model; the order is defined in the header
        int nX = psMetadataLookupS32(&status, row, "NXORDER"); REQUIRE (status, "missing NXORDER");
        int nY = psMetadataLookupS32(&status, row, "NYORDER"); REQUIRE (status, "missing NYORDER");
        if (file->fpa->toTPA == NULL) {
            // allocate the new transformation
            file->fpa->toTPA = psPlaneTransformAlloc(nX, nY, PS_POLYNOMIAL_ORD); // fpa->tpa uses ORD
        } else {
            REQUIRE (file->fpa->toTPA->x->nX == nX, "mismatch in chip order");
            REQUIRE (file->fpa->toTPA->x->nY == nY, "mismatch in chip order");
            REQUIRE (file->fpa->toTPA->y->nX == nX, "mismatch in chip order");
            REQUIRE (file->fpa->toTPA->y->nY == nY, "mismatch in chip order");
        }

        int ix = psMetadataLookupS32(&status, row, "XORDER"); REQUIRE (status, "missing XORDER");
        int iy = psMetadataLookupS32(&status, row, "YORDER"); REQUIRE (status, "missing YORDER");
        file->fpa->toTPA->x->coeff[ix][iy]     = psMetadataLookupF32(&status, row, "POLY_X");  REQUIRE (status, "missing POLY_X");
        file->fpa->toTPA->y->coeff[ix][iy]     = psMetadataLookupF32(&status, row, "POLY_Y");  REQUIRE (status, "missing POLY_Y");
        file->fpa->toTPA->x->coeffErr[ix][iy]  = psMetadataLookupF32(&status, row, "ERROR_X"); REQUIRE (status, "missing ERROR_X");
        file->fpa->toTPA->y->coeffErr[ix][iy]  = psMetadataLookupF32(&status, row, "ERROR_Y"); REQUIRE (status, "missing ERROR_Y");
        file->fpa->toTPA->x->coeffMask[ix][iy] = psMetadataLookupU8 (&status, row, "MASK_X");  REQUIRE (status, "missing MASK_X");
        file->fpa->toTPA->y->coeffMask[ix][iy] = psMetadataLookupU8 (&status, row, "MASK_Y");  REQUIRE (status, "missing MASK_Y");
    }

    psRegion *region = pmAstromFPAExtent (file->fpa);

    // as of r40806, psPlaneTransformInvert supplies the extra order (if non-linear)
    psFree (file->fpa->fromTPA);
    file->fpa->fromTPA = psPlaneTransformInvert(NULL, file->fpa->toTPA, *region, 50, ExtraOrders);

    psFree (model);
    psFree (header);
    psFree (region);
    return true;
}

# define TRANSFER(TO,FROM,NAME) { \
    psMetadataItem *item = psMetadataLookup(FROM,NAME); \
    if (!item) psAbort ("cannot find %s", NAME); \
    psMetadataItem *newItem = psMetadataItemCopy(item); \
    if (!psMetadataAddItem(TO, newItem, PS_LIST_TAIL, PS_META_REPLACE)) { \
        psAbort ("cannot copy %s", NAME); \
    } \
    psFree (newItem); }

// third layer applies boresite corrections and converts tangent plane to sky
bool pmAstromModelReadTP (pmFPAfile *file) {

    if (!psFitsMoveExtName (file->fits, "TP")) {
        psError(PS_ERR_IO, false, "missing TP extension in astrometry model\n");
        return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!header) psAbort("cannot read model header");

    psArray *model = psFitsReadTable (file->fits);
    psLogMsg ("psModules.astrom", 4, "read %ld rows from TP\n", model->n);
    if (model->n != 1) psAbort("invalid number of rows in TP model (%ld)", model->n);

    psMetadata *row = model->data[0];

    // move needed items to the concepts
    TRANSFER (file->fpa->concepts, row, "XSCALE");
    TRANSFER (file->fpa->concepts, row, "XSCALE");
    TRANSFER (file->fpa->concepts, row, "YSCALE");
    TRANSFER (file->fpa->concepts, row, "BORE_X0");
    TRANSFER (file->fpa->concepts, row, "BORE_Y0");
    TRANSFER (file->fpa->concepts, row, "BORE_RX");
    TRANSFER (file->fpa->concepts, row, "BORE_RY");
    TRANSFER (file->fpa->concepts, row, "BORE_T0");
    TRANSFER (file->fpa->concepts, row, "BORE_P0");
    TRANSFER (file->fpa->concepts, row, "POS_ZERO");
    //   TRANSFER (file->fpa->concepts, row, "ROT_PARITY");
    TRANSFER (file->fpa->concepts, row, "REF_CHIP");

    // DEFAULT value of +1 for ROT_PARITY (do not abort on missing value)
    psMetadataItem *item = psMetadataLookup(row, "ROT_PARITY");
    if (!item) {
      psMetadataAddS32(row, PS_LIST_TAIL, "ROT_PARITY", PS_META_REPLACE, "rotator parity", +1);
      psLogMsg ("psModules.astrom", 4, "setting default ROT_PARITY of +1\n");
    } else {
      psMetadataItem *newItem = psMetadataItemCopy(item); 
      if (!psMetadataAddItem(file->fpa->concepts, newItem, PS_LIST_TAIL, PS_META_REPLACE)) {
        psAbort ("cannot copy %s", "ROT_PARITY"); 
      }
      psFree (newItem); 
    }

    psFree (model);
    psFree (header);
    return (true);
}

// first layer is the sky
bool pmAstromModelReadSky (pmFPAfile *file) {

    if (!psFitsMoveExtName (file->fits, "SKY")) {
        psError(PS_ERR_IO, false, "missing SKY extension in astrometry model\n");
        return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!header) psAbort("cannot read model header");

    psArray *model = psFitsReadTable (file->fits);
    psLogMsg ("psModules.astrom", 4, "read %ld rows from SKY\n", model->n);
    if (model->n != 1) psAbort("invalid number of rows in SKY model (%ld)", model->n);

    // XXX not much information of interest in this table...

    // generate a template projection for comparisons
    psFree (file->fpa->toSky);
    file->fpa->toSky = psProjectionAlloc (0.0, 0.0, PS_RAD_DEG/3600.0, PS_RAD_DEG/3600.0, PS_PROJ_DIS);

    psFree (model);
    psFree (header);
    return (true);
}

// third layer applies boresite corrections and converts tangent plane to sky
bool pmAstromModelSetTP (pmFPAfile *file, psMetadata *concepts) {

    bool status;

    int ExtraOrders = pmAstrometryGetExtraOrders();

    // these externally supplied values are used to set the final transformation terms
    double RA  = psMetadataLookupF64 (&status, concepts, "FPA.RA"); REQUIRE (status, "missing FPA.RA");
    double DEC = psMetadataLookupF64 (&status, concepts, "FPA.DEC"); REQUIRE (status, "missing FPA.DEC");
    double POS = PM_RAD_DEG * psMetadataLookupF64 (&status, concepts, "FPA.POSANGLE"); REQUIRE (status, "missing FPA.POSANGLE");

    // get projection scale; center is supplied
    float Xs = psMetadataLookupF32(&status, file->fpa->concepts, "XSCALE") * PM_RAD_DEG; REQUIRE (status, "missing XSCALE");
    float Ys = psMetadataLookupF32(&status, file->fpa->concepts, "YSCALE") * PM_RAD_DEG; REQUIRE (status, "missing YSCALE");

    // allocate a new toSky projection using the reported position
    psFree (file->fpa->toSky);
    file->fpa->toSky = psProjectionAlloc (RA, DEC, Xs, Ys, PS_PROJ_DIS);

    // get boresite correction terms.  RX,RY,To,Po define an ellipse along which the boresite travels
    double Xo = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_X0"); REQUIRE (status, "missing ");
    double Yo = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_Y0"); REQUIRE (status, "missing ");
    double RX = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_RX"); REQUIRE (status, "missing ");
    double RY = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_RY"); REQUIRE (status, "missing ");
    double To = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_T0"); REQUIRE (status, "missing ");
    double Po = psMetadataLookupF32(&status, file->fpa->concepts, "BORE_P0"); REQUIRE (status, "missing ");

    // the true rotation of the instrument is POSANGLE - POS_ZERO
    double PosZero = PM_RAD_DEG * psMetadataLookupF32(&status, file->fpa->concepts, "POS_ZERO"); REQUIRE (status, "missing POS_ZERO");
    char *refChip  = psMetadataLookupStr(&status, file->fpa->concepts, "REF_CHIP"); REQUIRE (status, "missing REF_CHIP");

    int rotatorParity = psMetadataLookupS32(&status, file->fpa->concepts, "ROT_PARITY"); 
    // REQUIRE (status, "missing ROT_PARITY");
    if (!status) {
      rotatorParity = +1;
      psLogMsg ("psModules.astrom", 4, "setting default ROT_PARITY of +1\n");
    }
    
    // EAM 20200208 note: the toTPA versions of the transformation are more fundamental (the are used for the WCS conversion)
    // we should rotate the toTPA transformation and not the fromFPA.  I believe the sign is reversed, so test this

    // XXX we've swapped the sign and parity of POS (add to model)
    // apply true posangle = -(POS - POS_ZERO)
    psLogMsg ("psModules.astrom", 4, "Position Angle: %f, Model Position Angle Zero Point: %f\n", POS, PosZero);

    psPlaneTransform *toTPA = psPlaneTransformRotate (NULL, file->fpa->toTPA, rotatorParity * (POS - PosZero));
    psFree (file->fpa->toTPA);
    file->fpa->toTPA = toTPA;

    // the inverse transformation requires a higher order model to fit the direct transformation sufficiently well
    psRegion *region = pmAstromFPAExtent (file->fpa);

    psFree (file->fpa->fromTPA);
    file->fpa->fromTPA = psPlaneTransformInvert(NULL, file->fpa->toTPA, *region, 50, ExtraOrders);

    psFree (region);

    // current position of the nominal boresite in refChip coordinates
    double X = Xo + RX*cos(POS - To)*cos(Po) + RY*sin(POS - To)*sin(Po);
    double Y = Yo + RY*sin(POS - To)*cos(Po) - RX*cos(POS - To)*sin(Po);
    psLogMsg ("psModules.astrom", 4, "Boresite coords on reference chip: %f, %f pix = %f, %f sky\n", X, Y, PM_DEG_RAD*RA, PM_DEG_RAD*DEC);

    // get reference chip from name
    pmChip *chip = pmConceptsChipFromName (file->fpa, refChip);
    if (!chip) psAbort ("invalid chip name for reference");

    psPlane *boreCH = psPlaneAlloc();
    psPlane *boreFP = psPlaneAlloc();
    psPlane *boreTP = psPlaneAlloc();
    psSphere *boreSky = psSphereAlloc();

    // find the FP coord of the reported boresite location
    boreCH->x = X;
    boreCH->y = Y;
    psPlaneTransformApply (boreFP, chip->toFPA, boreCH);

    // find the true RA,DEC coord of the mirror of the reported boresite FP location
    boreFP->x = -boreFP->x;
    boreFP->y = -boreFP->y;
    psPlaneTransformApply (boreTP, file->fpa->toTPA, boreFP);
    psDeproject (boreSky, boreTP, file->fpa->toSky);

    // modify the projection to account for offset between true and reported boresite
    file->fpa->toSky->R = boreSky->r;
    file->fpa->toSky->D = boreSky->d;

    psTrace ("psModules.astrom", 5, "actual boresite coordinates: %lf, %lf\n", file->fpa->toSky->R*PS_DEG_RAD, file->fpa->toSky->D*PS_DEG_RAD);
    psTrace ("psModules.astrom", 5, "plate scale used: %lf, %lf\n", file->fpa->toSky->Xs*PS_DEG_RAD*3600.0, file->fpa->toSky->Ys*PS_DEG_RAD*3600.0);

    psFree (boreCH);
    psFree (boreFP);
    psFree (boreTP);
    psFree (boreSky);

    return (true);
}

