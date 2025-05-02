/** @file ppBackgroundArguments.c
 *
 *  @brief
 *
 *  @ingroup ppBackground
 *
 *  @author Paul Price
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackground.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppBackgroundData *data   // Run-time data
    )
{
    fprintf(stderr, "\nPan-STARRS background replacement\n\n");
    fprintf(stderr, "Usage: %s OUTPUT_ROOT\n\n", program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(data);

    pmConfigDone();
    psLibFinalize();

    exit(PS_EXIT_CONFIG_ERROR);
}


bool ppBackgroundArguments(ppBackgroundData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = data->config->arguments;
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-pattern", 0, "Filename of pattern correction", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-background", 0, "Filename of background model", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-image", 0, "Filename of image (required)", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask", 0, "Filename of mask", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-variance", 0, "Filename of variance image", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-auxmask", 0, "Filename of auxiliary mask", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stats", 0, "Output statistics file", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dumpconfig", 0, "Output configuration file", NULL);
    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, data);
    }

    data->patternName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-pattern"));
    data->backgroundName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-background"));
    data->imageName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-image"));
    data->maskName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-mask"));
    data->varianceName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-variance"));
    data->auxMaskName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-auxmask"));
    data->outRoot = psStringCopy(argv[1]);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root name", data->outRoot);

    psTrace("ppBackground", 1, "Done reading command-line arguments\n");

    if (!data->patternName && !data->backgroundName && !data->auxMaskName) {
//        psError(PPBACKGROUND_ERR_CONFIG, true, "Must specify at least one of -pattern and -background -auxmask");
//        return false;
    }
    if (!data->imageName) {
        psError(PPBACKGROUND_ERR_CONFIG, true, "Must specify -image");
        return false;
    }

    PS_ASSERT_STRING_NON_EMPTY(data->outRoot, false);

    const char *statsName = psMetadataLookupStr(NULL, arguments, "-stats");
    if (statsName) {
        psString resolved = pmConfigConvertFilename(statsName, data->config, true, true); // Resolved filename
        if (!resolved) {
            psError(psErrorCodeLast(), false, "Unable to resolve statistics file %s", statsName);
            return false;
        }
        data->statsFile = fopen(resolved, "w");
        if (!data->statsFile) {
            psError(PPBACKGROUND_ERR_IO, true, "Unable to open statistics file %s = %s", statsName, resolved);
            psFree(resolved);
            return false;
        }
        psFree(resolved);
        data->stats = psMetadataAlloc();
        pmConfigRunFilenameAddWrite(data->config, "STATS", statsName);
    }

    return true;
}


