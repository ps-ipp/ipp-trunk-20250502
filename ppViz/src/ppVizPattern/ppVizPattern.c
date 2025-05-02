#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPattern.h"

int main(int argc, char *argv[])
{
    ppVizPatternVersionPrint();

    ppVizPatternData *data = ppVizPatternDataInit(&argc, argv);
    if (!data) {
        psErrorStackPrint(stderr, "Unable to initialise.");
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPatternArguments(data, argc, argv)) {
        psErrorStackPrint(stderr, "Unable to parse arguments.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPatternCamera(data)) {
        psErrorStackPrint(stderr, "Unable to parse camera configuration.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppVizPatternLoop(data)) {
        psErrorStackPrint(stderr, "Unable to process data.");
        psFree(data);
        return PS_EXIT_DATA_ERROR;
    }

    psFree(data);

    return PS_EXIT_SUCCESS;
}

