/* @file  pmAstrometryRefstars.c
 * @brief Functions to write (and read?) astrometric reference stars
 *
 * @ingroup AstroImage
 *
 * @author EAM, IfA
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-07-17 22:38:15 $
 * Copyright 2008 Institute for Astronomy, University of Hawaii
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
#include "pmFPAfileFitsIO.h"
#include "pmAstrometryObjects.h"
#include "pmAstrometryRefstars.h"

/********************* CheckDataStatus functions *****************************/

bool pmAstromRefstarsCheckDataStatusForView (const pmFPAview *view, pmFPAfile *file) {

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        bool exists = pmAstromRefstarsCheckDataStatusForFPA (fpa);
        return exists;
    }
    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool exists = pmAstromRefstarsCheckDataStatusForChip (chip);
        return exists;
    }
    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    psError(PS_ERR_IO, false, "Astrometry only valid at the chip level");
    return false;
}

// return true if data exists for any chip
bool pmAstromRefstarsCheckDataStatusForFPA (const pmFPA *fpa) {

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        if (pmAstromRefstarsCheckDataStatusForChip (chip)) return true;
    }
    return false;
}

// return true if data exists for any cell
bool pmAstromRefstarsCheckDataStatusForChip (const pmChip *chip) {

    for (int i = 0; i < chip->cells->n; i++) {
      pmCell *cell = chip->cells->data[i];
        if (!cell) continue;
        if (pmAstromRefstarsCheckDataStatusForCell (cell)) return true;
    }
    return false;
}

// return true if data exists for any readout
bool pmAstromRefstarsCheckDataStatusForCell (const pmCell *cell) {

    for (int i = 0; i < cell->readouts->n; i++) {
      pmReadout *readout = cell->readouts->data[i];
        if (!readout) continue;
        if (pmAstromRefstarsCheckDataStatusForReadout (readout)) return true;
    }
    return false;
}

// check if refstars array exists
bool pmAstromRefstarsCheckDataStatusForReadout (const pmReadout *readout) {

  if (!readout->analysis) return false;

  // select the raw objects for this readout
  psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
  if (refstars == NULL) return false;

  return true;
}

/********************* Write Data functions *****************************/

bool pmAstromRefstarsWriteForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        if (!pmAstromRefstarsWriteFPA (fpa, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write refstars for fpa");
            return false;
        }
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing chip == %d (>= chips->n == %ld)", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        if (!pmAstromRefstarsWriteChip (chip, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write refstars for chip");
            return false;
        }
        return true;
    }

    psError(PS_ERR_IO, false, "Astrometry must be written at the FPA level");
    return false;
}

bool pmAstromRefstarsWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config) {

    // output header data
    psMetadata *outhead = psMetadataAlloc();

    // use the FPA phu to generate the PHU header
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    pmHDU *phu = psMemIncrRefCounter(fpa->hdu);
    psFree(fpa);

    // if there is no FPA PHU, this is a single header+image (extension-less) file. This could be
    // the case for an input SPLIT set of files being written out as a MEF.  if there is a PHU,
    // write it out as a 'blank'
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    } else {
        pmConfigConformHeader (outhead, file->format);
    }
    psFree(phu);

    psMetadataAddBool (outhead, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
    psFitsWriteBlank (file->fits, outhead, "");
    file->wrote_phu = true;

    psTrace ("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree (outhead);

    return true;
}

// write out all chip-level Astrometry data for this FPA
bool pmAstromRefstarsWriteFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmAstromRefstarsWriteChip (chip, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth chip", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

bool pmAstromRefstarsWriteChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        if (!pmAstromRefstarsWriteCell (cell, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth cell", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

// read in all readout-level Objects files for this cell
bool pmAstromRefstarsWriteCell (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        if (!pmAstromRefstarsWriteReadout (readout, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth readout", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

// write out all refstars files for this readout
bool pmAstromRefstarsWriteReadout (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->analysis, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    // select the raw objects for this readout
    psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
    if (refstars == NULL) { return TRUE; }

    psMetadata *header = psMetadataAlloc();

    psArray *table = psArrayAllocEmpty (1);

    // set the extname : we are really only allowed one entry per chip; check this here?
    char *chiprule = psStringCopy ("{CHIP.NAME}");
    char *chipname = pmFPAfileNameFromRule (chiprule, file, view);

    for (int i = 0; i < refstars->n; i++) {
      psMetadata *row = psMetadataAlloc ();

      pmAstromObj *ref = refstars->data[i];

      psMetadataAddF64(row,    PS_LIST_TAIL, "RA",      PS_META_REPLACE, "degrees", PS_DEG_RAD*ref->sky->r);
      psMetadataAddF64(row,    PS_LIST_TAIL, "DEC",     PS_META_REPLACE, "degrees", PS_DEG_RAD*ref->sky->d);
      psMetadataAddF32(row,    PS_LIST_TAIL, "TP_X",    PS_META_REPLACE, "microns", ref->TP->x);
      psMetadataAddF32(row,    PS_LIST_TAIL, "TP_Y",    PS_META_REPLACE, "microns", ref->TP->y);
      psMetadataAddF32(row,    PS_LIST_TAIL, "FP_X",    PS_META_REPLACE, "microns", ref->FP->x);
      psMetadataAddF32(row,    PS_LIST_TAIL, "FP_Y",    PS_META_REPLACE, "microns", ref->FP->y);
      psMetadataAddF32(row,    PS_LIST_TAIL, "CHIP_X",  PS_META_REPLACE, "microns", ref->chip->x);
      psMetadataAddF32(row,    PS_LIST_TAIL, "CHIP_Y",  PS_META_REPLACE, "microns", ref->chip->y);
      psMetadataAddF32(row,    PS_LIST_TAIL, "MAG",     PS_META_REPLACE, "microns", ref->Mag);
      psMetadataAddF32(row,    PS_LIST_TAIL, "MAG_ERR", PS_META_REPLACE, "microns", ref->dMag);

      psArrayAdd (table, 100, row);
      psFree (row);
    }
    if (!psFitsWriteTable (file->fits, header, table, chipname)) {
        psError(PS_ERR_IO, false, "writing refstars\n");
        psFree (table);
        psFree (header);
        psFree (chiprule);
        psFree (chipname);
        return false;
    }

    psFree (chiprule);
    psFree (chipname);
    psFree (table);
    psFree (header);
    return true;
}

