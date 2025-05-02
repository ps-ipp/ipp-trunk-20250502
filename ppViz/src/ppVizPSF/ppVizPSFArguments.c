/** @file ppVizPSFArguments.c
 *
 *  @brief
 *
 *  @ingroup ppVizPSF
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

#include "ppVizPSF.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppVizPSFData *data   // Run-time data
    )
{
    fprintf(stderr, "\nPan-STARRS PSF visualisation\n\n");
    fprintf(stderr,
            "Usage: %s -psf PSF.psf -sources SOURCES.cmf OUTPUT_ROOT\n\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(data);

    pmConfigDone();
    psLibFinalize();

    exit(PS_EXIT_CONFIG_ERROR);
}


bool ppVizPSFArguments(ppVizPSFData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-psf", 0, "Filename of PSF", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-sources", 0, "Filename of CMF sources", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-input", 0, "Filename with x,y,mag", NULL);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-fake-num", 0, "Number of fake sources", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-fake-mag", 0, "Magnitude of fake sources", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-x", 0, "x position of fake source [0..1]", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-y", 0, "y position of fake source [0..1]", NAN);

    psMetadataAddBool(arguments, PS_LIST_TAIL, "-use-resid", 0, "Number of fake sources", false);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, data);
    }

    bool mdok;                          // Status of MD lookup
    data->psfName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-psf"));
    data->sourcesName = psMemIncrRefCounter(psMetadataLookupStr(&mdok, arguments, "-sources"));
    data->fakeNum = psMetadataLookupS32(NULL, arguments, "-fake-num");
    data->fakeMag = psMetadataLookupF32(NULL, arguments, "-fake-mag");
    data->outRoot = psStringCopy(argv[1]);
    data->x = psMetadataLookupF32(NULL, arguments, "-x");
    data->y = psMetadataLookupF32(NULL, arguments, "-y");

    data->useResiduals = psMetadataLookupBool(NULL, arguments, "-use-resid");

    psMetadataAddStr(data->config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root name", data->outRoot);

    const char *inputName = psMetadataLookupStr(NULL, arguments, "-input"); // Name of input file
    if (inputName) {
        data->input = psVectorsReadFromFile(inputName, "%f %f %f");
        if (!data->input) {
            psError(psErrorCodeLast(), false, "Unable to read input file %s", inputName);
            return false;
        }
    }

    psTrace("ppVizPSF", 1, "Done reading command-line arguments\n");
    psFree(arguments);

    PS_ASSERT_STRING_NON_EMPTY(data->psfName, false);
    PS_ASSERT_STRING_NON_EMPTY(data->outRoot, false);

    return true;
}


