/** @file  pmSourceIO_MatchedRefs.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 02:44:19 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

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

#include "pmSourceIO.h"

#include "pmAstrometryObjects.h"
#include "pmAstrometryWCS.h"

bool pmSourceIO_WriteMatchedRefs (psFits *fits, pmFPA *fpa, pmConfig *config) {

    bool status = true;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    // first, check if there are any matches to be written

    // determine the output table format
    // XXX move this elsewhere? (psastro recipe? filerules?)
    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PSASTRO");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "missing recipe PSASTRO in config data");
        return false;
    }

    bool REFS_OUTPUT = psMetadataLookupBool(&status, recipe, "PSASTRO.SAVE.REFMATCH");
    if (!REFS_OUTPUT) return true;

    psArray *table = psMetadataLookupPtr (&status, fpa->analysis, "MATCHED_REFS");
    psMemIncrRefCounter (table);

    if (!table) {
        table = psArrayAllocEmpty (0x1000);
        pmFPAview *view = pmFPAviewAlloc (0);

        // this loop selects the matched stars for all chips
        while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
            if (!chip->process || !chip->file_exists) continue;

            char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");

            while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
                psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (!cell->process || !cell->file_exists) continue;

                // process each of the readouts
                // XXX there can only be one readout per chip, right?
                while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                    if (! readout->data_exists) continue;

                    // select the raw objects for this readout
                    psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
                    if (rawstars == NULL) continue;

                    // select the raw objects for this readout
                    psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
                    if (refstars == NULL) continue;
                    psTrace ("psastro", 4, "Trying %ld refstars\n", refstars->n);

                    psArray *matches = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
                    if (matches == NULL) continue;

                    for (int i = 0; i < matches->n; i++) {
                        pmAstromMatch *match = matches->data[i];

                        pmAstromObj *raw = rawstars->data[match->raw];
                        pmAstromObj *ref = refstars->data[match->ref];

                        psMetadata *row = psMetadataAlloc ();
                        psMetadataAdd (row, PS_LIST_TAIL, "RA_REF",     PS_DATA_F64, "right ascension (deg, J2000)", PM_DEG_RAD*ref->sky->r);
                        psMetadataAdd (row, PS_LIST_TAIL, "DEC_REF",    PS_DATA_F64, "declination (deg, J2000)",     PM_DEG_RAD*ref->sky->d);
                        psMetadataAdd (row, PS_LIST_TAIL, "RA_RAW",     PS_DATA_F64, "right ascension (deg, J2000)", PM_DEG_RAD*raw->sky->r);
                        psMetadataAdd (row, PS_LIST_TAIL, "DEC_RAW",    PS_DATA_F64, "declination (deg, J2000)",     PM_DEG_RAD*raw->sky->d);
                        psMetadataAdd (row, PS_LIST_TAIL, "X_CHIP_REF", PS_DATA_F32, "x fitted coord on chip",       ref->chip->x);
                        psMetadataAdd (row, PS_LIST_TAIL, "Y_CHIP_REF", PS_DATA_F32, "y fitted coord on chip",       ref->chip->y);
                        psMetadataAdd (row, PS_LIST_TAIL, "X_CHIP_RAW", PS_DATA_F32, "x coord on chip",              raw->chip->x);
                        psMetadataAdd (row, PS_LIST_TAIL, "Y_CHIP_RAW", PS_DATA_F32, "y coord on chip",              raw->chip->y);
                        psMetadataAdd (row, PS_LIST_TAIL, "X_FPA_RAW",  PS_DATA_F32, "x coord on focal plane",       raw->FP->x);
                        psMetadataAdd (row, PS_LIST_TAIL, "Y_FPA_RAW",  PS_DATA_F32, "y coord on focal plane",       raw->FP->y);
                        psMetadataAdd (row, PS_LIST_TAIL, "X_TPA_RAW",  PS_DATA_F32, "x coord on focal plane",       raw->TP->x);
                        psMetadataAdd (row, PS_LIST_TAIL, "Y_TPA_RAW",  PS_DATA_F32, "y coord on focal plane",       raw->TP->y);
                        psMetadataAdd (row, PS_LIST_TAIL, "MAG_INST",   PS_DATA_F32, "instrumental magnitude",       raw->Mag);
                        psMetadataAdd (row, PS_LIST_TAIL, "MAG_REF",    PS_DATA_F32, "reference star magnitude",     ref->Mag);
                        psMetadataAdd (row, PS_LIST_TAIL, "COLOR_REF",  PS_DATA_F32, "reference star color",         ref->Color);
                        psMetadataAdd (row, PS_LIST_TAIL, "CHIP_ID",    PS_DATA_STRING, "chip identifier",           chipName);
                        // XXX need to add the reference color, but this needs getstar / dvo.photcodes for the reference to be refined.

                        psArrayAdd (table, 100, row);
                        psFree (row);
                    }
                }
            }
        }
        psFree (view);

        if (table->n == 0) {
            psFree(table);
            return true;
        }
    }

    if (!psFitsWriteTable(fits, NULL, table, "MATCHED_REFS")) {
        psError(psErrorCodeLast(), false, "writing MATCHED_REFS\n");
        psFree(table);
        return false;
    }

    psFree(table);
    return true;
}

bool pmSourceIO_ReadMatchedRefs (psFits *fits, pmFPA *fpa, const pmConfig *config) {

    bool status = true;

    // check if we've already read (attempted to read) REFMATCH
    bool readMatchedRefs = psMetadataLookupBool (&status, fpa->analysis, "READ.REFMATCH");
    if (readMatchedRefs) return true;

    // try find the MATCHED_REFS extension.  if non-existent, note that we tried, and move on.
    // It is not an error to lack this entry -- psFitsMoveExtNameClean does not raise an error
    if (!psFitsMoveExtNameClean (fits, "MATCHED_REFS")) {
        psMetadataAddBool (fpa->analysis, PS_LIST_TAIL, "READ.REFMATCH", PS_META_REPLACE, "attempted to read MATCHED_REFS", true);
        return true;
    }

    // We get the size of the table, and allocate the array of sources first because the table
    // is large and ephemeral --- when the table gets blown away, whatever is allocated after
    // the table is read blocks the free.  In fact, it's better to read the table row by row.
    long numRows = psFitsTableSize(fits); // Number of sources in table
    psArray *rows = psArrayAlloc(numRows); // Array of sources, to return

    // first, check if there are any matches to be written
    for (int i = 0; i < numRows; i++) {
        psMetadata *row = psFitsReadTableRow(fits, i); // Table row
        if (!row) {
            psError(psErrorCodeLast(), false, "Unable to read row %d of matched references.", i);
            psFree(rows);
            return false;
        }
        rows->data[i] = row;
    }

    psMetadataAddArray (fpa->analysis, PS_LIST_TAIL, "MATCHED_REFS", PS_META_REPLACE, "MATCHED_REFS", rows);
    psFree (rows);

    // note that we have already read this dataa
    psMetadataAddBool (fpa->analysis, PS_LIST_TAIL, "READ.REFMATCH", PS_META_REPLACE, "attempted to read MATCHED_REFS", true);

    return true;
}
