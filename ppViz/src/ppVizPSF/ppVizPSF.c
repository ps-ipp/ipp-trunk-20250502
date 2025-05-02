#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPSF.h"

int main(int argc, char *argv[])
{
    ppVizPSFVersionPrint();

    ppVizPSFData *data = ppVizPSFDataInit(&argc, argv);
    if (!data) {
        psErrorStackPrint(stderr, "Unable to initialise.");
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPSFArguments(data, argc, argv)) {
        psErrorStackPrint(stderr, "Unable to parse arguments.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPSFCamera(data)) {
        psErrorStackPrint(stderr, "Unable to parse camera configuration.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPSFLoop(data)) {
        psErrorStackPrint(stderr, "Unable to process data.");
        psFree(data);
        return PS_EXIT_DATA_ERROR;
    }

    psFree(data);

    return PS_EXIT_SUCCESS;
}

