/** @file ppMergeFiles.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

const char *allFiles[] = { "PPMERGE.INPUT", "PPMERGE.INPUT.MASK", "PPMERGE.INPUT.VARIANCE",
                           "PPMERGE.OUTPUT", "PPMERGE.OUTPUT.COUNT", "PPMERGE.OUTPUT.SIGMA",
                           NULL };      // All files
const char *inputFiles[] = { "PPMERGE.INPUT", "PPMERGE.INPUT.MASK", "PPMERGE.INPUT.VARIANCE",
                             NULL };    // Input files
const char *outputFiles[] = { "PPMERGE.OUTPUT", "PPMERGE.OUTPUT.COUNT", "PPMERGE.OUTPUT.SIGMA",
                              NULL };   ///< Output files

/**
 * Select file list based on enum
 */
static const char **selectFiles(ppMergeFiles files)
{
    switch (files) {
      case PPMERGE_FILES_ALL:    return allFiles;
      case PPMERGE_FILES_INPUT:  return inputFiles;
      case PPMERGE_FILES_OUTPUT: return outputFiles;
      default:
        psAbort("Invalid file option");
    }
    return NULL;
}

bool ppMergeFileReadInput(pmConfig *config, pmReadout *readout, int num, int rows)
{
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", num);
    if (!pmReadoutReadChunk(readout, file->fits, 0, NULL, rows, 0, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read readout.");
        return false;
    }
    bool mdok;          // Status of MD lookup
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS")) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.MASK", num);
        if (!pmReadoutReadChunkMask(readout, file->fits, 0, NULL, rows, 0, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read readout mask.");
            return false;
        }
    }
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES")) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.VARIANCE", num);
        if (!pmReadoutReadChunkVariance(readout, file->fits, 0, NULL, rows, 0, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read readout variance.");
            return false;
        }
    }
    return true;
}

bool ppMergeFileOpenInput(pmConfig *config, const pmFPAview *view, int num)
{
    {
        pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", num);
        pmFPAview *fileView = pmFPAviewForLevel(input->fileLevel, view);
        if (!pmFPAfileOpen(input, fileView, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to open image file %d", num);
            psFree(fileView);
            return false;
        }
        psFree(fileView);

        // Read the headers, so we can have the concepts available
        pmCell *cell = pmFPAviewThisCell(view, input->fpa); // Cell of interest
        if (!pmCellReadHeaderSet(cell, input->fits, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read headers for image %d", num);
            return false;
        }
    }
    bool mdok;          // Status of MD lookup
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS")) {
        pmFPAfile *mask = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.MASK", num); // Mask file
        pmFPAview *fileView = pmFPAviewForLevel(mask->fileLevel, view);
        if (!pmFPAfileOpen(mask, fileView, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to open mask file %d", num);
            psFree(fileView);
            return false;
        }
        psFree(fileView);
    }
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES")) {
        pmFPAfile *variance = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.VARIANCE",
                                                    num); // Variance file
        pmFPAview *fileView = pmFPAviewForLevel(variance->fileLevel, view);
        if (!pmFPAfileOpen(variance, fileView, config)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to open variance file %d", num);
            psFree(fileView);
            return false;
        }
        psFree(fileView);
    }
    return true;
}

bool ppMergeFileFreeInput(pmConfig *config, const pmFPAview *view, int num)
{
    {
        pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", num);
        if (!pmFPAfileFreeData(input, view)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to free data for image file %d", num);
            return false;
        }
    }
    bool mdok;          // Status of MD lookup
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS")) {
        pmFPAfile *mask = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.MASK", num); // Mask file
        if (!pmFPAfileFreeData(mask, view)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to free data for mask file %d", num);
            return false;
        }
    }
    if (psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES")) {
        pmFPAfile *variance = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.VARIANCE", num); // Variance file
        if (!pmFPAfileFreeData(variance, view)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to free data for variance file %d", num);
            return false;
        }
    }
    return true;
}

// Select the specified input files and assign the dataLevel and freeLevel,
// saving a pointer on output array.
// This function takes the argument pmFPALevel and sets the dataLevel based on that value.
// However, this is only used with the value PM_FPA_LEVEL_READOUT, and it must be set
// to that level for the rest of ppMerge to work correctly.  Furthermore, the freeLevel
// must be set to Cell.  Remove the (false) option.
psArray *ppMergeFileDataLevel(const pmConfig *config, const char *name)
{
    assert(config);
    assert(name);

    int numFiles = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM"); // Number of input files
    assert(numFiles > 0);
    psArray *files = psArrayAlloc(numFiles); // Files of interest
    for (int i = 0; i < numFiles; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, name, i); // Image file
        file->dataLevel = PM_FPA_LEVEL_READOUT;
        file->freeLevel = PM_FPA_LEVEL_CELL;
        files->data[i] = psMemIncrRefCounter(file);
    }
    return files;
}

bool ppMergeFileActivate(const pmConfig *config, ppMergeFiles files, bool state)
{
    assert(config);

    bool mdok;                          // Status of MD lookup
    bool haveMasks = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS"); // Do we have masks?
    bool haveVariances = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES"); // Got variances?

    const char **fileList = selectFiles(files); // Files to activate
    for (int i = 0; fileList[i] != NULL; i++) {
        if (!haveMasks && strcmp(fileList[i], "PPMERGE.INPUT.MASK") == 0) {
            continue;
        }
        if (!haveVariances && strcmp(fileList[i], "PPMERGE.INPUT.VARIANCE") == 0) {
            continue;
        }
        psString name = NULL;           // Name of file
        if (strcmp(fileList[i], "PPMERGE.OUTPUT") == 0) {
            name = ppMergeOutputFile(config);
        } else {
            name = psStringCopy(fileList[i]);
        }

        if (!pmFPAfileActivate(config->files, state, name)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to activate file %s", name);
            psFree(name);
            return false;
        }
        psFree(name);
    }

    return true;
}



psArray *ppMergeFileActivateSingle(const pmConfig *config, ppMergeFiles files, bool state, int num)
{
    assert(config);

    bool mdok;                          // Status of MD lookup
    bool haveMasks = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS"); // Do we have masks?
    bool haveVariances = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES"); // Got variances?

    psList *list = psListAlloc(NULL);   // List of files
    const char **fileList = selectFiles(files); // Files to activate
    for (int i = 0; fileList[i] != NULL; i++) {
        if (!haveMasks && strcmp(fileList[i], "PPMERGE.INPUT.MASK") == 0) {
            continue;
        }
        if (!haveVariances && strcmp(fileList[i], "PPMERGE.INPUT.VARIANCE") == 0) {
            continue;
        }

        psString name = NULL;           // Name of file
        if (strcmp(fileList[i], "PPMERGE.OUTPUT") == 0) {
            name = ppMergeOutputFile(config);
        } else {
            name = psStringCopy(fileList[i]);
        }

        pmFPAfile *file = pmFPAfileActivateSingle(config->files, state, name, num); // Activated file
        psFree(name);
        psListAdd(list, PS_LIST_TAIL, file);
    }

    psArray *array = psListToArray(list);
    psFree(list);

    return array;
}


psString ppMergeOutputFile(const pmConfig *config)
{
    ppMergeType type = psMetadataLookupS32(NULL, config->arguments, "TYPE"); // Type of frame
    const char *outSuffix = NULL;         // Suffix for output file
    switch (type) {
      case PPMERGE_TYPE_BIAS:
        outSuffix = "BIAS";
        break;
      case PPMERGE_TYPE_DARK:
        outSuffix = "DARK";
        break;
      case PPMERGE_TYPE_MASK:
        outSuffix = "MASK";
        break;
      case PPMERGE_TYPE_CTEMASK:
        outSuffix = "MASK";
        break;
      case PPMERGE_TYPE_SHUTTER:
        outSuffix = "SHUTTER";
        break;
      case PPMERGE_TYPE_FLAT:
        outSuffix = "FLAT";
        break;
      case PPMERGE_TYPE_FRINGE:
        outSuffix = "FRINGE";
	break;
      case PPMERGE_TYPE_NOISEMAP:
	outSuffix = "NOISEMAP";
	break;
      default:
        psAbort("Unknown frame type: %x", type);
    }

    psString outName = NULL;            // Name of output file
    psStringAppend(&outName, "PPMERGE.OUTPUT.%s", outSuffix);

    return outName;
}
