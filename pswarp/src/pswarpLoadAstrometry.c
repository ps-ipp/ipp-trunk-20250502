/** @file pswarpLoadAstrometry.c
 *
 *  @brief load the headers and construct astrometric transformations 
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

// we have a pmFPAfile which defines the astrometry via headers.  we force it to have type
// WCS and read the headers.  We place the resulting info on the target astrometry
// containers
bool pswarpLoadAstrometry (pmFPAfile *target, pmFPAfile *astrom, pmConfig *config) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;

    // XXX assert that astrom be WCS or CMF?

    // XXX set the type to be WCS
    pmFPAfileType saveType = astrom->type;
    astrom->type = PM_FPA_FILE_WCS;

    // **** read in all of the headers from the astrometry source

    pmFPAview *view = pmFPAviewAlloc(0);

    if (!pmFPAfileRead (astrom, view, config)) {
	psError(PS_ERR_IO, false, "failed READ at FPA %s", astrom->name);
	psFree(view);
	return false;
    }

    while ((chip = pmFPAviewNextChip (view, astrom->fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process || !chip->file_exists) { continue; }
        pmChip *targetChip = pmFPAviewThisChip(view, target->fpa); // < Chip in the output
	if (!targetChip || !targetChip->process || !targetChip->file_exists) {
            continue; // only load astrometry into output chips which exist!
        }
	if (!pmFPAfileRead (astrom, view, config)) {
	    psError(psErrorCodeLast(), false, "failed READ at CHIP %s", astrom->name);
	    psFree(view);
	    return false;
	}
	while ((cell = pmFPAviewNextCell (view, astrom->fpa, 1)) != NULL) {
	    psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	    if (!cell->process) { continue; }
	    if (!pmFPAfileRead (astrom, view, config)) {
		psError(psErrorCodeLast(), false, "failed READ at CELL %s", astrom->name);
		psFree(view);
		return false;
	    }
	    if (!pmFPAfileClose(astrom, view)) {
		psError(psErrorCodeLast(), false, "failed CLOSE at CELL %s", astrom->name);
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
	}
	if (!pmFPAfileClose(astrom, view)) {
	    psError(psErrorCodeLast(), false, "failed CLOSE at CHIP %s", astrom->name);
	    psFree (view);
	    return false;
	}
    }
    if (!pmFPAfileClose(astrom, view)) {
	psError(psErrorCodeLast(), false, "failed CLOSE at FPA %s", astrom->name);
	psFree (view);
	return false;
    }

    // **** convert the header astrometry info (in astrom) to the pmFPA astrometry
    // structures, saving them on the target fpa structure.

    pmFPAviewReset (view);

    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU(view, astrom->fpa); ///< Astrometry PHU
    if (phu) {
        char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp(&ctype[4], "-DIS");
        }
    }

    // apply the bilevel astrometry elements to the target
    if (bilevelAstrometry) {
	if (!pmAstromReadBilevelMosaic(astrom->fpa, phu->header)) {
	    psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for skycell.");
	    psFree(view);
	    return false;
	}
    }
    psMetadataAddBool (astrom->fpa->analysis, PS_LIST_TAIL, "ASTROMETRY.BILEVEL", PS_META_REPLACE, "bilevel astrometry?", bilevelAstrometry);

    // for pswarpLocalFrame, I need transformations and HDUs on a single fpa (so set on astrom as well as target)
    // target->fpa->toTPA = psMemIncrRefCounter (astrom->fpa->toTPA);
    // target->fpa->fromTPA = psMemIncrRefCounter (astrom->fpa->fromTPA);
    // target->fpa->toSky = psMemIncrRefCounter (astrom->fpa->toSky);

    while ((chip = pmFPAviewNextChip (view, astrom->fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process || !chip->file_exists) { continue; }

        pmChip *targetChip = pmFPAviewThisChip(view, target->fpa); // < Chip in the output
	if (!targetChip || !targetChip->process || !targetChip->file_exists) {
            continue; // only load astrometry into output chips which exist!
        }

	// we've got the astrom header
	pmHDU *hdu = pmHDUFromChip(chip); ///< HDU for source
	if (!hdu || !hdu->header) {
	    psError(PM_ERR_PROG, false, "Unable to find header for sky cell.");
	    psFree(view);
	    return false;
	}

	if (bilevelAstrometry) {
	    if (!pmAstromReadBilevelChip(chip, hdu->header)) {
		psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for skycell.");
		psFree(view);
		return false;
	    }
	} else {
	    // we use a default FPA pixel scale of 1.0
	    if (!pmAstromReadWCS(astrom->fpa, chip, hdu->header, 1.0)) {
		psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for skycell.");
		psFree(view);
		return false;
	    }
	}
	if (targetChip != chip) {
	  psAssert (!targetChip->toFPA, "oops");
	  psAssert (!targetChip->fromFPA, "oops");
	  targetChip->toFPA = psMemIncrRefCounter (chip->toFPA);
	  targetChip->fromFPA = psMemIncrRefCounter (chip->fromFPA);
	}
    }

    // for pswarpLocalFrame, I need transformations and HDUs on a single fpa (so set on astrom as well as target)
    // But: do not increment the ref counter if this is the same entry
    if (target->fpa != astrom->fpa) {
      psAssert (!target->fpa->toTPA, "oops");
      psAssert (!target->fpa->fromTPA, "oops");
      psAssert (!target->fpa->toSky, "oops");
      target->fpa->toTPA = psMemIncrRefCounter (astrom->fpa->toTPA);
      target->fpa->fromTPA = psMemIncrRefCounter (astrom->fpa->fromTPA);
      target->fpa->toSky = psMemIncrRefCounter (astrom->fpa->toSky);
    }

    // reset the type to the original value
    astrom->type = saveType;

    psFree(view);
    return true;
}

