/** @file ppBackgroundStackArguments.c
 *
 *  @brief
 *
 *  @ingroup ppBackgroundStack
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

#include "ppBackgroundStack.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppBackgroundStackData *data   // Run-time data
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


bool ppBackgroundStackArguments(ppBackgroundStackData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = data->config->arguments;
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-input", 0, "Filename of input metadata", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-image", 0, "Filename of image (required)", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask", 0,  "Filename of mask", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-fitOTAs", 0, "", false);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-OTApath", 0, "", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dumpconfig", 0, "", NULL);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, data);
    }

    unsigned int numBad = 0; // Number of bad lines
    data->contents = psMetadataConfigRead(NULL, &numBad,psMetadataLookupStr(NULL, arguments, "-input"),false);
    if (!(data->contents) || numBad > 0) {
      psError(PPBACKGROUND_ERR_CONFIG, false, "Unable to cleanly read MDC file with inputs.");
      return(false);
    }

    // Add any images to generate for to the list
    // If we're going to do this for a full projection cell, this probably needs to be a metadata object.
    psArrayAdd(data->stacks, data->stacks->n, psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-image")));

    // Determine what we're doing with the OTA solution
    if ((!psMetadataLookupBool(NULL,arguments,"-fitOTAs"))&&
	(!psMetadataLookupStr(NULL,arguments,"-OTApath"))) {
      psError(PPBACKGROUND_ERR_CONFIG, false, "No OTA solution path (-OTApath) provided, and no request to model this (-fitOTAs).");
      return(false);
    }
    data->fit_OTAS = psMetadataLookupBool(NULL, arguments, "-fitOTAs");
    data->OTApath  = psMetadataLookupStr(NULL, arguments, "-OTApath");


    // This is the output base
    data->outRoot = psStringCopy(argv[1]);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root name", data->outRoot);

    psTrace("ppBackgroundStack", 1, "Done reading command-line arguments\n");


    PS_ASSERT_STRING_NON_EMPTY(data->outRoot, false);

    return true;
}


