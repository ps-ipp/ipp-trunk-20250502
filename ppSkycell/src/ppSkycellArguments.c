/** @file ppSkycellArguments.c
 *
 *  @brief
 *
 *  @ingroup ppSkycell
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

#include "ppSkycell.h"

/// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  ppSkycellData *data   // Run-time data
    )
{
    fprintf(stderr, "\nPan-STARRS skycell JPEGifier\n\n");
    fprintf(stderr, "Usage: %s -images INPUT.list OUTPUT_ROOT\n\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(data);

    pmConfigDone();
    psLibFinalize();

    exit(PS_EXIT_CONFIG_ERROR);
}


bool ppSkycellArguments(ppSkycellData *data, int argc, char *argv[])
{
    assert(data);
    assert(data->config);

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-images", 0, "Filename with input images", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-wcsref", 0, "Filename with WCS references", NULL);
    psMetadataAddS16(arguments, PS_LIST_TAIL, "-exptimeOrder", 0, "Order of exptime scaling", 0);
    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, data);
    }

    data->imagesName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-images"));
    data->wcsrefName = psMemIncrRefCounter(psMetadataLookupStr(NULL, arguments, "-wcsref"));
    data->exptimeOrder = psMetadataLookupS16(NULL,arguments,"-exptimeOrder");
    
    data->outRoot = psStringCopy(argv[1]);

    psTrace("ppSkycell", 1, "Done reading command-line arguments\n");
    psFree(arguments);

    PS_ASSERT_STRING_NON_EMPTY(data->imagesName, false);
    PS_ASSERT_STRING_NON_EMPTY(data->outRoot, false);

    return true;
}


