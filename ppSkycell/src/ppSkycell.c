#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSkycell.h"

int main(int argc, char *argv[])
{
    ppSkycellData *data = ppSkycellDataInit(&argc, argv);
    if (!data) {
        psErrorStackPrint(stderr, "Unable to initialise.");
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppSkycellArguments(data, argc, argv)) {
        psErrorStackPrint(stderr, "Unable to parse arguments.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppSkycellCamera(data)) {
        psErrorStackPrint(stderr, "Unable to parse camera configuration.");
        psFree(data);
        return PS_EXIT_CONFIG_ERROR;
    }

    if (!ppSkycellLoop(data)) {
        psErrorStackPrint(stderr, "Unable to process data.");
        // psFree(data); -- XXX this can cause an error when freeing a file that failed
        return PS_EXIT_DATA_ERROR;
    }

    psFree(data);

    return PS_EXIT_SUCCESS;
}

