/** @file ppArithArguments.c
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 19:45:30 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppArith.h"

/**
 * Print usage information and die
 */
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config      // Configuration
    )
{
    fprintf(stderr, "\nPan-STARRS image arithmetic\n\n");
    fprintf(stderr, "Usage: %s -file1 INPUT1.fits -op OP -file2 INPUT2.fits OUTPUT_ROOT\n\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

/**
 * Get a string value from the command-line and add it to the target
 */
static bool valueArgStr(psMetadata *arguments, // Command-line arguments
                        const char *argName, // Argument name in the command-line arguments
                        const char *mdName, // Name for value in the metadata
                        psMetadata *target // Target metadata to which to add value
                        )
{
    psString value = psMetadataLookupStr(NULL, arguments, argName); // Value of interest
    if (value && strlen(value) > 0) {
        return psMetadataAddStr(target, PS_LIST_TAIL, mdName, 0, NULL, value);
    }
    return false;
}

/**
 * Add a single filename to the arguments as an array, so that it can be used with pmFPAfileBindFromArgs, etc
 */
static void fileList(const char *file, // The symbolic name for the file
                     const char *name, // The name of the file
                     const char *comment, // Description of the file
                     pmConfig *config // Configuration
    )
{
    psArray *files = psArrayAlloc(1); // Array with file names
    files->data[0] = psStringCopy(name);
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, file, 0, comment, files);
    psFree(files);
    return;
}

bool ppArithArguments(int argc, char *argv[], pmConfig *config)
{
    assert(config);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-file1", 0, "First image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-op", 0, "Operation to perform", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-file2", 0, "Second image", NULL);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-const2", 0, "Constant", NAN);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-mask", 0, "Treat images as masks", false);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stats", 0, "Statistics file", NULL);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, config);
    }

    bool isMask = psMetadataLookupBool(NULL, arguments, "-mask"); // Are we dealing with masks?
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "MASK", 0, "Produce a mask image?", isMask);
    const char *inFilerule = isMask ? "PPARITH.INPUT.MASK" : "PPARITH.INPUT.IMAGE"; ///< Input file rule
    const char *outFilerule = isMask ? "PPARITH.OUTPUT.MASK" : "PPARITH.OUTPUT.IMAGE"; // Output file rule
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "FILERULE.INPUT", 0,
                     "File rule for input", inFilerule);
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "FILERULE.OUTPUT", 0,
                     "File rule for output", outFilerule);

    bool status = false;                // Status for file definition

    // First file
    const char *name1 = psMetadataLookupStr(NULL, arguments, "-file1"); // Name of first image
    if (!name1 || strlen(name1) == 0) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No input image specified.");
        goto ERROR;
    }
    fileList("INPUT1", name1, "Name of the first input image", config);
    pmFPAfile *file1 = pmFPAfileDefineFromArgs(&status, config, inFilerule, "INPUT1");
    if (!status || !file1) {
        psError(PS_ERR_IO, false, "Failed to build FPA from %s", inFilerule);
        goto ERROR;
    }
    if ((isMask && file1->type != PM_FPA_FILE_MASK) || (!isMask && file1->type != PM_FPA_FILE_IMAGE)) {
        psError(PS_ERR_IO, true, "File rule %s is not of the expected type", inFilerule);
        goto ERROR;
    }

    // Second file is optional (won't be one for unary operations)
    const char *name2 = psMetadataLookupStr(NULL, arguments, "-file2"); // Name of second image
    if (name2 && strlen(name2) > 0) {
        fileList("INPUT2", name2, "Name of the second input image", config);
        pmFPAfile *file2 = pmFPAfileDefineFromArgs(&status, config, inFilerule, "INPUT2");
        if (!status || !file2) {
            psError(PS_ERR_IO, false, "Failed to build FPA from %s", inFilerule);
            goto ERROR;
        }
        if ((isMask && file2->type != PM_FPA_FILE_MASK) || (!isMask && file2->type != PM_FPA_FILE_IMAGE)) {
            psError(PS_ERR_IO, true, "File rule %s is not of the expected type", inFilerule);
            goto ERROR;
        }
        if (file2->fpa->camera != file1->fpa->camera) {
            psError(PS_ERR_IO, true, "Cameras for inputs differ.");
            goto ERROR;
        }
    }
    float const2 = psMetadataLookupF32(NULL, arguments, "-const2"); // Constant to apply
    if (!isnan(const2)) {
        if (name2) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot specify both -file2 and -const2");
            goto ERROR;
        }
        psMetadataAddF32(config->arguments, PS_LIST_TAIL, "PPARITH.CONST", 0, "Constant to apply", const2);
    }

    // Output image
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);
    pmFPAfile *output = pmFPAfileDefineOutput(config, file1->fpa, outFilerule);
    if (!output) {
        psError(PS_ERR_IO, false, "Unable to generate output file from %s", outFilerule);
        goto ERROR;
    }
    if ((isMask && output->type != PM_FPA_FILE_MASK) || (!isMask && output->type != PM_FPA_FILE_IMAGE)) {
        psError(PS_ERR_IO, true, "%s is not of the expected type", outFilerule);
        goto ERROR;
    }
    output->save = true;

    valueArgStr(arguments, "-op",    "OPERATION", config->arguments);
    valueArgStr(arguments, "-stats", "STATS",     config->arguments);

    psTrace("ppArith", 1, "Done reading command-line arguments\n");
    psFree(arguments);
    return true;

ERROR:
    psFree(arguments);
    return false;
}


