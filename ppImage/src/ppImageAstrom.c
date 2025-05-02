#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

// this function is mostly equivalent to the top-level of psastro, with some
// modifications since the data has already been loaded.
bool ppImageAstrom (pmConfig *config, psMetadata *stats) {

    bool status;

    // select recipe options supplied on command line
    // XXX move these options to the "PSASTRO" recipe?
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);

    // find or define a pmFPAfile PSPHOT.INPUT
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSASTRO.INPUT");
    PS_ASSERT (input, false);

    ppImageMemoryDump("astrom");

    // convert the output sources created by psphot into astrometry objects
    if (!psastroConvertFPA (config, input->fpa, recipe)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "error reading input data\n");
        return false;
    }

    if (!psastroAnalysis(config, stats)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failure in psastro analysis\n");
        return false;
    }

    // deactivate the psastro files, reactive when needed
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.OUTPUT");

    // loop over all chips and perform IO needed
    pmChip *chip = NULL;
    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppImageLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);

        ppImageMemoryDump("astrom");

        // Output and Close at Chip level
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psFree(view);
            return false;
        }
    }

    // Output and Close FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psFree(view);
        return false;
    }

    // deactivate the PSASTRO files, re-active all else
    // XXX do we need a way to activate / deactivate other groups?
    pmFPAfileActivate (config->files, true, NULL);
    pmFPAfileActivate (config->files, false, "PSASTRO.OUTPUT");

    psFree(view);
    return true;
}
