#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    Under construction: 10% complete.
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8)
#define TEST_NUM_COLS           (16)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_SOURCES		100
int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(1);


    // ----------------------------------------------------------------------
    // pmPSFtoMetadata() tests
    // psMetadata *pmPSFtoMetadata (psMetadata *metadata, pmPSF *psf)
    // Call pmPSFtoMetadata() with NULL psPSF input parameter
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->psfTrendNx = 1;
        psfOptions->psfTrendNy = 2;
        psfOptions->psfFieldNx = 3;
        psfOptions->psfFieldNy = 4;
        psfOptions->psfFieldXo = 5;
        psfOptions->psfFieldYo = 6;
        pmModelClassInit();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        psMetadata *meta2 = pmPSFtoMetadata(metadata, NULL);
        ok(meta2 == NULL, "pmPSFtoMetadata() returned NULL with NULL psPSF input parameter");
        psFree(metadata);
        psFree(psfOptions);
        psFree(psf);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
