# include "fpcamera.h"
# define NONLIN_TOL 0.001 /* tolerance in pixels */

bool fpcameraAstrometryBilevel (pmFPAfile *astrom) {
    myAssert (astrom, "programming error");

    pmFPA *fpa = astrom->fpa;
    myAssert (fpa, "programming error");

    bool useBilevelWCS = (fpa->toTPA->x->nX > 1) || (fpa->toTPA->x->nY > 1);
    return useBilevelWCS;
}

// this is saved on the readout->analysis, but we assume a chip has only one cell & readout
bool fpcameraAstrometryChipHeader (pmConfig *config, pmFPAview *view, pmReadout *readout, pmFPAfile *astrom) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, FPCAMERA_RECIPE);

    // save WCS and analysis metadata in update header
    // (pull or create local view to entry on readout->analysis)
    psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
    if (!updates) {
	updates = psMetadataAlloc ();
	psMetadataAddMetadata (readout->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
	psFree (updates);
    }

    bool useBilevelWCS = fpcameraAstrometryBilevel(astrom);

    pmFPA *fpa = astrom->fpa;
    pmChip *astromChip = pmFPAviewThisChip(view, astrom->fpa);

    // if the toTPA distortion model uses higher-order terms, then we need to use BiLevel astrometry 
    if (useBilevelWCS) {
	// create the header keywords to descripe the results
	if (!pmAstromWriteBilevelChip (updates, astromChip, NONLIN_TOL)) {
	    psError(PS_ERR_UNKNOWN, false, "invalid solution for %d,%d,%d\n", view->chip, view->cell, view->readout);
	    psErrorStackPrint(stderr, "failure to generate WCS keywords for one chip\n");
	    psErrorClear();
	    return false;
	}
    } else {
	fpa->wcsCDkeys = psMetadataLookupBool(&status, recipe, "PSASTRO.WCS.USECDKEYS");
	pmAstromWriteWCS (updates, fpa, astromChip, NONLIN_TOL);
    }
    
    return true;
}

bool fpcameraAstrometryFPAHeader(pmFPA *fpa, pmFPAfile *astrom, psMetadata *stats) {

    bool status;

    bool useBilevelWCS = fpcameraAstrometryBilevel(astrom);
    if (!useBilevelWCS) return true; // no fpa-level headers if not bilevel
    
    // save WCS and analysis metadata in update header.
    // (pull or create local view to entry on readout->analysis)
    psMetadata *updates = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
    if (!updates) {
        updates = psMetadataAlloc ();
        psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
        psFree (updates);
    }

    if (!pmAstromWriteBilevelMosaic (updates, astrom->fpa, NONLIN_TOL)) {
	// error here is a data quality problem (and too bad to write smf)
	psWarning ("Failed to save header terms");
	if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
	    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Mosaic astrometry failed", FPCAMERA_ERR_DATA);
	}
	return false;
    }
    return true;
}
