/** @file ppVizPatternArguments.c
 *
 *  @brief
 *
 *  @ingroup ppVizPattern
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

#include "ppVizPattern.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppVizPatternData *data   // Run-time data
    )
{
    fprintf(stderr, "\nPan-STARRS pattern visualisation\n\n");
    fprintf(stderr,
            "Usage: %s -pattern PATTERN.ptn OUTPUT_ROOT\n\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(data);

    pmConfigDone();
    psLibFinalize();

    exit(PS_EXIT_CONFIG_ERROR);
}


bool ppVizPatternArguments(ppVizPatternData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-pattern", 0, "Filename of pattern correction", NULL);
    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, data);
    }

    data->patternName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-pattern"));
    data->outRoot = psStringCopy(argv[1]);
    psMetadataAddStr(data->config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root name", data->outRoot);

    psTrace("ppVizPattern", 1, "Done reading command-line arguments\n");
    psFree(arguments);

    PS_ASSERT_STRING_NON_EMPTY(data->patternName, false);
    PS_ASSERT_STRING_NON_EMPTY(data->outRoot, false);

    return true;
}


