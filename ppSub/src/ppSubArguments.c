/** @file ppSubArguments.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.59 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config      // Configuration
    )
{
    fprintf(stderr, "\nPan-STARRS PSF-matched image subtraction\n\n");
    fprintf(stderr, "Usage: %s OUTPUT_ROOT \n"
            "\t[-psf REFERENCE.psf.fits]\n\n"
            "This subtracts the convolved REFERENCE from the INPUT, by default.\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
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

void ppSubSetThreads (void) {
    // ppSub does not have any of its own thread handlers
    return;
}

bool ppSubArguments(int argc, char *argv[], ppSubData *data)
{
    assert(data);
    pmConfig *config = data->config;
    assert(config);

    // generic arguments (version, dumpconfig)
    PS_ARGUMENTS_GENERIC( ppSub, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( ppSub, config, argc, argv )

    int argNum = psArgumentGet(argc, argv, "-debug"); // Debugging argument number
    if (argNum) {
        psArgumentRemove(argNum, &argc, argv);
        pmSubtractionRegions(true);
    }


    psMetadata *arguments = config->arguments; // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-inimage", 0, "Input image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-inmask", 0, "Input mask image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-invariance", 0, "Input variance image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-insources", 0, "Input source list", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-refimage", 0, "Reference image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-refmask", 0, "Reference mask image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-refvariance", 0, "Reference variance image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-refsources", 0, "Reference source list", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-kernel", 0, "Precalculated kernel to apply", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stats", 0, "Statistics file", NULL);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-stamps", 0, "Stamps filename; x,y on each line", NULL);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-convolve", 0, "Image to convolve [1 or 2]", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-photometry", 0, "Perform photometry?", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-forced-phot", 0, "Perform forced photometry?", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-forced-input1", 0, "Perform forced photometry?", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-forced-input2", 0, "Perform forced photometry?", NULL);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp", 0, "Zero point for photometry", NAN);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-inverse", 0, "Generate inverse subtractions?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-save-inconv", 0, "Save input convolved images?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-save-refconv", 0, "Save reference convolved images?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-visual", 0, "Show diagnostic plots", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-updatemode", 0, "update mode?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-require-subkernel", 0, "do not regenerate subkernel if missing?", false);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, config);
    }

    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);


    const char *inImage = psMetadataLookupStr(NULL, arguments, "-inimage"); // Name of input image
    if (inImage && strlen(inImage) > 0) {
        fileList("INPUT", inImage, "Name of the input image", config);
    }
    const char *inMask = psMetadataLookupStr(NULL, arguments, "-inmask"); // Name of input mask
    if (inMask && strlen(inMask) > 0) {
        fileList("INPUT.MASK", inMask, "Name of the input mask image", config);
    }
    const char *inVariance = psMetadataLookupStr(NULL, arguments, "-invariance"); // Name of input variance
    if (inVariance && strlen(inVariance) > 0) {
        fileList("INPUT.VARIANCE", inVariance, "Name of the input variance image", config);
    }
    const char *inSources = psMetadataLookupStr(NULL, arguments, "-insources"); // Name of input source list
    if (inSources && strlen(inSources) > 0) {
        fileList("INPUT.SOURCES", inSources, "Name of the input source list", config);
    }

    const char *refImage = psMetadataLookupStr(NULL, arguments, "-refimage"); // Name of reference image
    if (refImage && strlen(refImage) > 0) {
        fileList("REF", refImage, "Name of the reference image", config);
    }
    const char *refMask = psMetadataLookupStr(NULL, arguments, "-refmask"); // Name of reference mask
    if (refMask && strlen(refMask) > 0) {
        fileList("REF.MASK", refMask, "Name of the reference mask image", config);
    }
    const char *refVariance = psMetadataLookupStr(NULL, arguments, "-refvariance"); // Name of ref variance
    if (refVariance && strlen(refVariance) > 0) {
        fileList("REF.VARIANCE", refVariance, "Name of the reference variance image", config);
    }
    const char *refSources = psMetadataLookupStr(NULL, arguments, "-refsources"); // Name of ref source list
    if (refSources && strlen(refSources) > 0) {
        fileList("REF.SOURCES", refSources, "Name of the reference source list", config);
    }

    const char *kernel = psMetadataLookupStr(NULL, arguments, "-kernel"); // Name of kernel
    if (kernel && strlen(kernel) > 0) {
        fileList("KERNEL", kernel, "Name of the kernel to apply", config);
    }

    data->stamps = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-stamps"));

    data->statsName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-stats"));
    if (data->statsName && strlen(data->statsName) > 0) {
        psString resolved = pmConfigConvertFilename(data->statsName, config, true, true); // Resolved filename
        if (!resolved) {
            psError(psErrorCodeLast(), false, "Unable to resolve statistics file: %s", data->statsName);
            return false;
        }
        data->statsFile = fopen(resolved, "w");
        if (!data->statsFile) {
            psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
            psFree(resolved);
            return false;
        }
        psFree(resolved);
    }

    data->saveInConv = psMetadataLookupBool(NULL, arguments, "-save-inconv");
    data->saveRefConv = psMetadataLookupBool(NULL, arguments, "-save-refconv");

    if (psMetadataLookupBool(NULL, arguments, "-visual")) {
        pmVisualSetVisual(true);
    }

    psTrace("ppSub", 1, "Done reading command-line arguments\n");

    return true;
}
