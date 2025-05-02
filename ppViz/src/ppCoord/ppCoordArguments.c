/** @file ppCoordArguments.c
 *
 *  @brief
 *
 *  @ingroup ppCoord
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

#include "ppCoord.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppCoordData *data   // Run-time data
    )
{
    fprintf(stderr, "\nPan-STARRS coordinate transformation\n\n");
    fprintf(stderr, "Usage: %s \n\n", program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(data);

    pmConfigDone();
    psLibFinalize();

    exit(PS_EXIT_CONFIG_ERROR);
}


bool ppCoordArguments(ppCoordData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-astrom", 0, "Filename with astrometry", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-raw", 0, "Filename with raw data", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-pixels", 0, "Filename with pixel coordinates", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-chip", 0, "Chip for pixel coordinates", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-radec", 0, "Filename with RA,Dec (default decimal degrees)", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-streaks", 0, "Filename with streaks", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-clusters", 0, "Filename with clusters", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-radians", 0, "RA,Dec in radians?", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-all", 0, "Output all coordinates?", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-ds9", 0, "Output ds9 region file", NULL);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-ds9-radius", 0, "ds9 region radii", 5.0);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-ds9-color", 0, "ds9 region color", "red");
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-extra-orders", 0, "extra orders for transform inversion", -1);
    if (!psArgumentParse(arguments, &argc, argv) || argc != 1) {
        usage(argv[0], arguments, data);
    }

    data->astromName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-astrom"));
    data->rawName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-raw"));
    data->pixelsName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-pixels"));
    data->chipName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-chip"));
    data->radecName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-radec"));
    data->streaksName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-streaks"));
    data->clustersName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-clusters"));
    data->radians = psMetadataLookupBool(NULL, arguments, "-radians");
    data->all = psMetadataLookupBool(NULL, arguments, "-all");
//    psMetadataAddStr(data->config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root name", data->outRoot);

    data->ds9name = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-ds9"));
    if (data->ds9name) {
        data->ds9 = fopen(data->ds9name, "w");
        if (!data->ds9) {
            psError(PS_ERR_IO, true, "Unable to open region file %s", data->ds9name);
            return false;
        }
    }
    data->ds9radius = psMetadataLookupF32(NULL, arguments, "-ds9-radius");
    data->ds9color = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-ds9-color"));

    int ExtraOrders = psMetadataLookupS32(NULL, arguments, "-extra-orders");
    if (ExtraOrders >= 0) { pmAstrometrySetExtraOrders(ExtraOrders); }

    psTrace("ppCoord", 1, "Done reading command-line arguments\n");
    psFree(arguments);

    if (!data->astromName) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "No astrometry file specified via -astrom");
        return false;
    }

    if (!data->pixelsName && !data->radecName && !data->streaksName && !data->clustersName) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Neither -pixels nor -radec provided.");
        return false;
    }

    return true;
}


