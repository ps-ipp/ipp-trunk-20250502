#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(9);

    // ----------------------------------------------------------------------
    // pmFPAConstruct() tests
    // pmFPA *pmFPAConstruct(const psMetadata *camera)
    // test will NULL camera input param
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = pmFPAConstruct(NULL, NULL);
        ok(fpa == NULL, "pmFPAConstruct() NULL will NULL camera input param");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPA *pmFPAConstruct(const psMetadata *camera)
    // test will acceptable data
    // XXX: The memory leak code is put outside the call to pmConfigFileRead() because of
    // a memory leak in pmConfigFileRead().
    {
        psMetadata *camera = psMetadataAlloc();
        bool rc = pmConfigFileRead(&camera, "dataFiles/camera0/camera.config", "CAMERA 0 config file");
        if (!rc) {
             rc = pmConfigFileRead(&camera, "../dataFiles/camera0/camera.config", "CAMERA 0 config file");
	}

        // Generate the pmFPA heirarchy
        psMemId id = psMemGetId();
        ok(rc, "Succesfully read camera format file");
        pmFPA* fpa = pmFPAConstruct(camera, NULL);
        ok(fpa != NULL, "pmFPAConstruct() returned non-NULL");
        if (VERBOSE) {
            pmFPAPrint(stdout, fpa, true, true);
	}
        bool errorFlag = false;
        ok(fpa->chips->n == 2, "pmFPAConstruct() set fpa->chips->n (%d)", fpa->chips->n);
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            ok(chip->cells->n == 2, "pmFPAConstruct() set chip->cells->n (%d)", chip->cells->n);
            for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
                pmCell *cell = chip->cells->data[cellID];
                for (int readoutID = 0 ; readoutID < cell->readouts->n ; readoutID++) {
                    pmReadout *readout = cell->readouts->data[readoutID];
                    readout = readout;
		}
	    }
	}
        ok(!errorFlag, "pmFPAConstruct() read the pmFPA structure correctly");




        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

        psFree(camera);
    }
}
