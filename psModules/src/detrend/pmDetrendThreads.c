#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "psPolynomialMD.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"

#include "pmOverscan.h"
#include "pmBias.h"
#include "pmDark.h"
#include "pmShutterCorrection.h"
#include "pmFlatField.h"
#include "pmDetrendThreads.h"

static int scanRows = 0;                // Number of rows to work on at once

int pmDetrendGetScanRows(void)
{
    return scanRows;
}

bool pmDetrendSetThreadTasks (int newScanRows)
{
    psAssert(scanRows == 0, "programming error: program called pmDetrendSetThreadTasks twice");

    PS_ASSERT_INT_POSITIVE(newScanRows, false);
    scanRows = newScanRows;

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_DETREND_BIAS", 7);
        task->function = &pmBiasSubtractScan_Threaded;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_DETREND_DARK", 9);
        task->function = &pmDarkApplyScan_Threaded;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_DETREND_SHUTTER", 8);
        task->function = &pmShutterCorrectionApplyScan_Threaded;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_DETREND_FLAT", 10);
        task->function = &pmFlatFieldScan_Threaded;
        psThreadTaskAdd(task);
        psFree(task);
    }

    // NOISEMAP : for now, not applied in the threaded loop 

    return true;
}
