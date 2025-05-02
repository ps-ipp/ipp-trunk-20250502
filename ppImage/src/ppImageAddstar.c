#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

bool setFilename (pmFPAfile *file, pmFPAview *view);
bool addstarFile (char *addstarCommand, char *filename);

bool ppImageAddstar (pmConfig *config) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    // select recipe options supplied on command line
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, RECIPE_NAME);

    // find a pmFPAfile PSASTRO.OUTPUT
    pmFPAfile *file = psMetadataLookupPtr (&status, config->files, "PSASTRO.OUTPUT");
    if (!status) {
        psError (PS_ERR_IO, true, "no PSASTRO.OUTPUT file defined");
        return false;
    }

    // find the addstar command: %s is replaced with name of output file
    char *addstarCommand = psMetadataLookupStr (&status, recipe, "ADDSTAR.COMMAND");

    pmFPAview *view = pmFPAviewAlloc (0);
    if (file->fileLevel == PM_FPA_LEVEL_CHIP) {
        // call addstar on this file
        setFilename (file, view);
        addstarFile (addstarCommand, file->filename);
        return true;
    }

    while ((chip = pmFPAviewNextChip (view, file->fpa, 1)) != NULL) {
        psLogMsg ("ppImageAddstar", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        if (file->fileLevel == PM_FPA_LEVEL_CHIP) {
            // call addstar on this file
            setFilename (file, view);
            addstarFile (addstarCommand, file->filename);
            continue;
        }

        while ((cell = pmFPAviewNextCell (view, file->fpa, 1)) != NULL) {
            psLogMsg ("ppImageAddstar", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (! cell->process || ! cell->file_exists) { continue; }

            if (file->fileLevel == PM_FPA_LEVEL_CELL) {
                // call addstar on this file
                setFilename (file, view);
                addstarFile (addstarCommand, file->filename);
                continue;
            }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, file->fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                if (file->fileLevel == PM_FPA_LEVEL_READOUT) {
                    // call addstar on this file
                    setFilename (file, view);
                    addstarFile (addstarCommand, file->filename);
                    continue;
                } else {
                    psError (PS_ERR_IO, true, "inconsistent fileLevel for %s: %d\n", file->name, file->fileLevel);
                    return false;
                }
            }
        }
    }
    return true;
}

bool setFilename (pmFPAfile *file, pmFPAview *view) {

    // determine the file name
    // free a name allocated earlier
    psFree (file->filename);
    file->filename = pmFPAfileNameFromRule (file->filerule, file, view);
    if (file->filename == NULL) {
        psError(PS_ERR_IO, true, "Filename is NULL");
        return false;
    }

    // indirect filenames are not allowed for output
    if (!strcasecmp (file->filename, "@FILES")) {
        psError(PS_ERR_IO, true, "indirect filenames are not allowed for output : %s\n", file->name);
        return false;
    }
    if (!strcasecmp (file->filename, "@DETDB")) {
        psError(PS_ERR_IO, true, "detrend db filenames are not allowed for output : %s\n", file->name);
        return false;
    }
    return true;
}

bool addstarFile (char *addstarCommand, char *filename) {

    bool status;
    char *addstarLine = psStringCopy(addstarCommand);
    psStringSubstitute(&addstarCommand, filename, "%s");

    // catch addstar stderr/stdout and do what?
    psIOBuffer *buffer = psIOBufferAlloc (512);
    psPipe *pipe = psPipeOpen (addstarLine);
    status = psIOBufferReadEmpty (buffer, 100, pipe->fd_stdout);
    if (!status) {
        psError (PS_ERR_IO, false, "detselect is not responding");
    }
    psFree (addstarLine);
    psFree (buffer);
    psPipeClose (pipe);
    psFree (pipe);
    return status;
}
