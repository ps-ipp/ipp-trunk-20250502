#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "ppImage.h"

//#define TESTING

void ppImageMemoryDump(const char *description)
{
#ifndef TESTING
    return;
#else
    psMemBlock **leaks = NULL;
    int numLeaks = psMemCheckLeaks(0, &leaks, NULL, true);

    static int num = 0;
    psString filename = NULL;
    psStringAppend(&filename, "%s_%d.txt", description, num++);
    FILE *fp = fopen(filename, "w");
    psFree(filename);

    for (int i = 0; i < numLeaks; i++) {
        psMemBlock *memBlock = leaks[i];
        fprintf(fp, "%ld : %s at (%s:%d)  ID: %lu  Ref: %lu\n",
                (unsigned long)memBlock->userMemorySize, memBlock->func, memBlock->file,
                (int)memBlock->lineno, (unsigned long)memBlock->id, memBlock->refCounter);
    }
    psFree(leaks);

    fclose(fp);

    return;
#endif
}
