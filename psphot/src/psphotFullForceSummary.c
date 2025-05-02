# ifdef HAVE_CONFIG_H
# include <config.h>
# endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include "psphot.h"

// For simplicilty, this program's (simple) functions are all contained in this file.
static pmConfig* psphotFullForceSummaryArguments(int, char**);
static bool psphotFullForceSummaryParseCamera(pmConfig *);
static bool psphotFullForceSummaryImageLoop(pmConfig*);

int main (int argc, char **argv) {

    psMemInit();
    psTimerStart ("complete");
    pmErrorRegister();                  // register psModule's error codes/messages
    psphotInit();

    // load command-line arguments, options, and system config data
    pmConfig *config = psphotFullForceSummaryArguments (argc, argv);
    assert(config);

//    psphotVersionPrint();

    if (!psphotFullForceSummaryParseCamera (config)) {
        psErrorStackPrint(stderr, "Error setting up the camera\n");
        exit (1);
    }

    if (!psphotFullForceSummaryImageLoop (config)) {
        psErrorStackPrint(stderr, "Error in the psphot image loop\n");
        exit(1);
    }

    // XXX:check for memory leaks
    exit (0);
}

// all functions which return to this level must raise one of the top-level error codes if they
// exit with an error.  these error codes are used to specify the program exit status

void usage() {
    fprintf(stderr, "usage: psphotFullForceSummary -inputs <input list> <output>\n");
    exit (1);
}

static pmConfig* psphotFullForceSummaryArguments(int argc, char **argv) {

    pmConfig *config =  pmConfigRead(&argc, argv, PSPHOT_RECIPE);
    int N;

    if (config == NULL) {
        psErrorStackPrint(stderr, "Can't read site configuration");
	exit(PS_EXIT_CONFIG_ERROR);
    }
    if ((N = psArgumentGet(argc, argv, "-input"))) {
        if (argc <= N+1) {
          psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
          usage();
        }
        psArgumentRemove(N, &argc, argv);

        unsigned int numBad = 0;                     // Number of bad lines
        psMetadata *inputs = psMetadataConfigRead(NULL, &numBad, argv[N], false); // Input file info
        if (!inputs || numBad > 0) {
            psErrorStackPrint(stderr, "Unable to cleanly read MDC file with inputs.");
            exit(PS_EXIT_CONFIG_ERROR);
        }
        psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "INPUTS", 0, "Metadata with input details", inputs);
        psFree(inputs);

        psArgumentRemove(N, &argc, argv);
    } else {
        psErrorStackPrint(stderr, "-input must be supplied.");
        usage();
    }
    if ((N = psArgumentGet(argc, argv, "-cff"))) {
        if (argc <= N+1) {
          psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
          usage();
        }
        psArgumentRemove(N, &argc, argv);

        // We read CFF file as table not as sources. All we want are the nominal parameters input to psphotFullForce
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "CFF_FILE", 0, "CFF file with nominal parameters", argv[N]);
        psArgumentRemove(N, &argc, argv);
    } else {
        psErrorStackPrint(stderr, "-cff must be supplied.");
        usage();
    }
 
    if (argc < 2) {
        psErrorStackPrint(stderr, "Output is required.");
        usage();
    }
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", PS_DATA_STRING, "", argv[1]);

    return config;
}

static bool psphotFullForceSummaryParseCamera(pmConfig *config) {
    bool status = false;

    psMetadata *inputs = psMetadataLookupMetadata(&status, config->arguments, "INPUTS"); // The inputs info
    if (!inputs) {
	psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find inputs.");
	return false;
    }

    int nInputs = inputs->list->n;

    for (int i = 0; i < nInputs; i++) {
        psMetadataItem *item = psMetadataGet(inputs, i);

        if (item->type != PS_DATA_STRING) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Component %s of the input metadata is not of type STRING", item->name);
	    return false;

        }
        psString sourcesFilename = item->data.str;

        psArray *dummy = psArrayAlloc(1);   // dummy array of filenames
        dummy->data[0] = psStringCopy(sourcesFilename);

        psMetadataAddArray(config->arguments, PS_LIST_TAIL, "FILENAMES", PS_META_REPLACE, 
            "Filenames for file rule definition", dummy);
        psFree(dummy);

        bool found = false;
        pmFPAfile *file = pmFPAfileDefineFromArgs(&found, config, "PSPHOT.INPUT.CMF", "FILENAMES");
        if (!file || !found) {
            psError(PS_ERR_UNKNOWN, false, "Unable to define file %s from %s", "PSPHOT.INPUT.CMF", sourcesFilename);
            return false;
        }
        if (file->type != PM_FPA_FILE_CMF) {
            psError(PS_ERR_IO, true, "%s is not of type %s", sourcesFilename, pmFPAfileStringFromType(PM_FPA_FILE_CMF));
            return false;
        }
    }
    psMetadataRemoveKey(config->arguments, "FILENAMES");
    psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.INPUT.CMF.NUM", PS_META_REPLACE, "number of inputs",
        nInputs);


    pmFPA *outputFPA = pmFPAConstruct(config->camera, config->cameraName);
        if (!outputFPA) {
        psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
        return false;
    }
    pmFPAfile *output = pmFPAfileDefineOutput(config, outputFPA, "PSPHOT.FULLFORCE.OUTPUT");
    psFree(outputFPA);                        // Drop reference
    if (!output) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PSPHOT.FULLFORCE.OUTPUT"));
        return false;
    }
    if (output->type != PM_FPA_FILE_CMF) {
        psError(PSPHOT_ERR_CONFIG, true, "PSPHOT.FULLFORCE.OUTPUT is not of type CMF");
        return false;
    }
    output->save = true;

    // We read cff file as a table not as sources
    psString cffName = psMetadataLookupStr(NULL, config->arguments, "CFF_FILE");
    if (!cffName) {
        psError(PSPHOT_ERR_CONFIG, true, "CFF_FILE is missing from arguments");
        return false;
    }
    psString resolvedName = pmConfigConvertFilename(cffName, config, false, false);
    if (!resolvedName) {
        psError(PSPHOT_ERR_CONFIG, false, "failed to resolve CFF_FILE %s", cffName);
        return false;
    }
    psFits *fits = psFitsOpen(resolvedName, "r");
    if (!fits) {
        psError(PSPHOT_ERR_CONFIG, false, "failed to open fits CFF_FILE %s", resolvedName);
        return false;
    }
    psArray *inTable = psFitsReadTable(fits);
    if (!inTable) {
        psError(PSPHOT_ERR_CONFIG, false, "failed to read cff fits table from CFF_FILE %s", resolvedName);
        return false;
    }
    psFitsClose(fits);

    // Convert to a set of arrays indexed by model type + 1
    // which contain pointers to arrays indexed by ID
#define MAX_MODEL_TYPE 10
    psArray *sortedTables = psArrayAlloc(MAX_MODEL_TYPE+1);
    for (int i=0; i<inTable->n; i++) {
        psMetadata *row = inTable->data[i];
        psS32 ID = psMetadataLookupS32(&status, row, "ID");
        psS32 modelType = psMetadataLookupS32(&status, row, "MODEL_TYPE");
        // XXX: need to use the lookup table functions to be ready for changes in the model type numbers
        if (modelType+1 >= MAX_MODEL_TYPE) {
            psError(PSPHOT_ERR_CONFIG, false, "found modelType %d max allowed is %d", modelType, MAX_MODEL_TYPE);
            return false;
        }
        psArray *sortedTable = sortedTables->data[modelType+1];
        if (!sortedTable) {
            sortedTable = psArrayAlloc(4*inTable->n);
            sortedTables->data[modelType+1] = sortedTable;
            // dont' free sortedTable the array of tables gets our reference
        }
        if (ID >= sortedTable->n) {
            sortedTable = psArrayRealloc(sortedTable, 2*ID);
            // Why doesn't psArrayRealloc do this?????
                sortedTable->n = sortedTable->nalloc;
        }
        if (sortedTable->data[ID]) {
            psError(PSPHOT_ERR_CONFIG, true, "Duplicate row with ID %d", ID);
            return false;
        }
        sortedTable->data[ID] = psMemIncrRefCounter(row);
    }
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, "CFF_TABLES", PS_META_REPLACE, "cff tables", sortedTables);
    psFree(inTable);
    psFree(sortedTables);

    return true;
}

# define ESCAPE(MESSAGE) {				\
	psError(PSPHOT_ERR_DATA, false, MESSAGE);	        \
	psFree (view);					\
	return false;					\
    }

bool psphotFullForceSummaryImageLoop (pmConfig *config) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.INPUT.CMF");
    if (!status) {
        psError(PSPHOT_ERR_PROG, false, "Can't find input data!");
        return false;
    }

    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSPHOT.FULLFORCE.OUTPUT");
    if (!output) {
        psError(PSPHOT_ERR_PROG, false, "Can't find output data!");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc (0);
    if (!pmFPAAddSourceFromView(output->fpa, view, output->format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to insert HDU into FPA for writing.\n");
        psFree(view);
        return NULL;
    }

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for fpa in psphot.");


    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
//        psLogMsg ("psphotFullForceSummary", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (! chip->process || ! chip->file_exists) { continue; }

        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip in psphotFullForceSummary.");

        // We read the WCS from the first input
        {
            pmHDU *hduLow = pmHDUGetLowest(input->fpa, chip, NULL);
            if (hduLow && !pmAstromReadWCS(input->fpa, chip, hduLow->header, 1.0)) {
                psWarning("Unable to read WCS astrometry from header.");
                psErrorClear();
                pmHDU *hduHigh = pmHDUGetHighest(input->fpa, chip, NULL);
                if (hduHigh && hduHigh != hduLow &&
                    !pmAstromReadWCS(input->fpa, chip, hduHigh->header, 1.0)) {
                    psWarning("Unable to read WCS astrometry from primary header.");
                    psErrorClear();
                }
            }
        }

        // Copy the transformations from the input
        output->fpa->fromTPA = psMemIncrRefCounter(input->fpa->fromTPA);
        output->fpa->toTPA = psMemIncrRefCounter(input->fpa->toTPA);
        output->fpa->toSky = psMemIncrRefCounter(input->fpa->toSky);
        pmChip *outputChip = pmFPAviewThisChip(view, output->fpa);
        outputChip->toFPA = psMemIncrRefCounter(chip->toFPA);
        outputChip->fromFPA = psMemIncrRefCounter(chip->fromFPA);
        if (output->fpa->hdu->header == NULL) {
            output->fpa->hdu->header = psMetadataAlloc();
        }
        // XXX: how come psphot and psphotStack don't have to do this?
        if (!pmAstromWriteWCS(output->fpa->hdu->header, output->fpa, outputChip, 0.001)) {
            ESCAPE("failure to copy WCS to header");
        }

        // there is now only a single chip (multiple readouts?). loop over it and process
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
	  // psLogMsg ("psphotFullForceSummary", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
	      // psLogMsg ("psphotFullForceSummary", 6, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (! readout->data_exists) { continue; }

                if (!psphotFullForceSummaryReadout(config, view)) {
                    ESCAPE ("failure in psphotFullForceSummaryReadout");
                }
            }
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed pmFPAfileIOChecks for Chip in psphotFullForceSummary.");
    }


    // If these keywords are not set we get a warning message on output. Since we are not copying the input header
    // and do not have an image pmFPA stuff doesn't have values for these
    // XXX: What other keywords and concepts should be set (or copied)?
    int numCols = psMetadataLookupS32(&status, input->fpa->hdu->header, "IMNAXIS1");
    int numRows = psMetadataLookupS32(&status, input->fpa->hdu->header, "IMNAXIS2");
    psMetadataAddS32(output->fpa->hdu->header, PS_LIST_TAIL, "IMNAXIS1", PS_META_REPLACE, "", numCols);
    psMetadataAddS32(output->fpa->hdu->header, PS_LIST_TAIL, "IMNAXIS2", PS_META_REPLACE, "", numRows);

    // XXX: Also add psphot version information

    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed pmFPAfileIOChecks for FPA in psphot.");

    // fail if we encountered an unhandled error
    if (psErrorCodeLast() != PS_ERR_NONE) psAbort ("failed to handle an error!");

    psFree (view);

    return true;
}
