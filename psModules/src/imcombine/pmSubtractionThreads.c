#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmSubtractionTypes.h"
#include "pmSubtractionMatch.h"
#include "pmSubtractionEquation.h"
#include "pmSubtraction.h"

bool threaded = false;                  // Run with threads?

bool pmSubtractionThreaded(void)
{
    return threaded;
}

void pmSubtractionThreadsInit(void)
{
    if (threaded) {
        psAbort("Already running threaded.");
    }

    threaded = true;

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_SUBTRACTION_ORDER", 8);
        task->function = &pmSubtractionOrderThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_SUBTRACTION_CALCULATE_EQUATION", 3);
        task->function = &pmSubtractionCalculateEquationThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_SUBTRACTION_CONVOLVE_STAMP", 3);
        task->function = &pmSubtractionConvolveStampThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_SUBTRACTION_CONVOLVE", 19);
        task->function = &pmSubtractionConvolveThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    return;
}


void pmSubtractionThreadsFinalize(void)
{
    if (!threaded) {
        return;
    }

    threaded = false;
    psThreadTaskRemove("PSMODULES_SUBTRACTION_ORDER");
    psThreadTaskRemove("PSMODULES_SUBTRACTION_CALCULATE_EQUATION");
    psThreadTaskRemove("PSMODULES_SUBTRACTION_CONVOLVE");

    return;
}
