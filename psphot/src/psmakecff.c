# ifdef HAVE_CONFIG_H
# include <config.h>
# endif

// psmakecff : A program to make read a cmf file and write a cff file
// The real work is done in psModules.

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include "psphot.h"

// For simplicilty, this program's (simple) functions are all contained in this file.
static pmConfig* psmakecffArguments(int, char**);
static bool psmakecffParseCamera(pmConfig *);
static bool psmakecffImageLoop(pmConfig*);

int main (int argc, char **argv) {

    psMemInit();
    psTimerStart ("complete");
    pmErrorRegister();                  // register psModule's error codes/messages
    psphotInit();

    // load command-line arguments, options, and system config data
    pmConfig *config = psmakecffArguments (argc, argv);
    assert(config);

//    psphotVersionPrint();

    if (!psmakecffParseCamera (config)) {
        psErrorStackPrint(stderr, "Error setting up the camera\n");
        exit (1);
    }

    if (!psmakecffImageLoop (config)) {
        psErrorStackPrint(stderr, "Error in the psphot image loop\n");
        exit(1);
    }

    exit (0);
}

// all functions which return to this level must raise one of the top-level error codes if they
// exit with an error.  these error codes are used to specify the program exit status

void usage() {
    fprintf(stderr, "usage: psmakecff -sources <input cmf> <output>\n");
    exit (1);
}

static pmConfig* psmakecffArguments(int argc, char **argv) {

    pmConfig *config =  pmConfigRead(&argc, argv, PSPHOT_RECIPE);
    int N;

    if (config == NULL) {
      psErrorStackPrint(stderr, "Can't read site configuration");
	exit(PS_EXIT_CONFIG_ERROR);
    }
    if ((N = psArgumentGet (argc, argv, "-sources"))) {
        pmConfigFileSetsMD(config->arguments, &argc, argv, "SOURCES", "-sources", "-sourceslist");
    } else {
        usage();
    }
    if (argc < 2) {
        usage();
    }
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", PS_DATA_STRING, "", argv[1]);

    return config;
}

static bool psmakecffParseCamera(pmConfig *config) {
    bool status = false;

    pmFPAfile *sources = pmFPAfileDefineFromArgs(&status, config, "PSPHOT.INPUT.CMF", "SOURCES");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to build FPA from sources file");
        return false;
    }

    pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, sources, "PSPHOT.OUTPUT.CFF");
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "failed to build output FPA");
        return false;
    }
    output->save = true;

    return true;
}

# define ESCAPE(MESSAGE) {				\
	psError(PSPHOT_ERR_DATA, false, MESSAGE);	        \
	psFree (view);					\
	return false;					\
    }

bool psmakecffImageLoop (pmConfig *config) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.INPUT.CMF");
    if (!status) {
        psError(PSPHOT_ERR_PROG, false, "Can't find input data!");
        return false;
    }

    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSPHOT.OUTPUT.CFF");
    if (!output) {
        psError(PSPHOT_ERR_PROG, false, "Can't find output data!");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc (0);
    pmHDU *lastHDU = NULL;              // Last HDU updated

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for fpa in psphot.");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");


    // for psphot, we force data to be read at the chip level
    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psLogMsg ("psmakecff", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (! chip->process || ! chip->file_exists) { continue; }

        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip in psmakecff.");

        // there is now only a single chip (multiple readouts?). loop over it and process
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("psmakecff", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                psLogMsg ("psmakecff", 6, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (! readout->data_exists) { continue; }

		pmHDU *hdu = pmHDUGetHighest(input->fpa, chip, cell);
		if (hdu && hdu != lastHDU) {
                    // XXX: probably should do this
		    // psphotVersionHeaderFull(hdu->header);
		    lastHDU = hdu;
                }
            }

        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed pmFPAfileIOChecks for Chip in psmakecff.");
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed pmFPAfileIOChecks for FPA in psphot.");

    // fail if we encountered an unhandled error
    if (psErrorCodeLast() != PS_ERR_NONE) psAbort ("failed to handle an error!");

    psFree (view);
    return true;
}
