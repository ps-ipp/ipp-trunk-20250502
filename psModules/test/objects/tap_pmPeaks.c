#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
        pmPeaksInImage(): Must debug tests for small images (1-by-1, N-by-1, 1-by-N)
*/

#define TST01_VECTOR_LENGTH 10
#define NUM_ROWS 10
#define NUM_COLS 10
#define TST02_NUM_ROWS 5
#define TST02_NUM_COLS 5
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

/******************************************************************************
test01(): we first test pmPeaksInVector() with a variety of bad input
parameters.  Then we test it with a simple vector both 1- and multi-elements.
 *****************************************************************************/
bool test_pmPeaksInVector(int n)
{
    psMemId id = psMemGetId();
    bool testStatus = true;
    psVector *inData = psVectorAlloc(n, PS_TYPE_F32);
    inData->n = inData->nalloc;
    psVector *outData = NULL;

    // Test first pixel peak.
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (n-i);
    }
    inData->data.F32[0] = (float) n;
    outData= pmPeaksInVector(inData, 0.0);
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 1) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        if (outData->data.U32[0] != 0) {
            diag("TEST ERROR: Did not find peak at element 0.\n");
            testStatus = false;
        }
        psFree(outData);
    }


    //
    // Test first pixel peak, large threshold
    //
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (n-i);
    }
    inData->data.F32[0] = (float) n;
    outData= pmPeaksInVector(inData, (float) (n*n));
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 0) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);

        // Skip remaining tests if the input vector has length 1.
        if (n == 1) {
            psFree(inData);
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
            return(testStatus);
        }
    }

    // Test last pixel peak.
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (i);
    }
    inData->data.F32[n-1] = (float) n;

    outData= pmPeaksInVector(inData, 0.0);
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 1) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        if (outData->data.U32[0] != n-1) {
            diag("TEST ERROR: Did not find peak at element %d.\n", n-1);
            testStatus = false;
        }
        psFree(outData);
    }


    // Test last pixel peak, large threshold.
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (i);
    }
    inData->data.F32[n-1] = (float) n;
    outData= pmPeaksInVector(inData, (float) (n*n));
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 0) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);
    }


    // Test interior peaks.
    // Set all even number elements to be peaks.
    for (psS32 i = 0 ; i < n ; i++) {
        if (0 == i%2) {
            inData->data.F32[i] = (float) (2 * i);
        } else {
            inData->data.F32[i] = (float) (i);
        }
    }
    inData->data.F32[0] = (float) n;


    outData= pmPeaksInVector(inData, 0.0);
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != n/2) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        for (psS32 i = 0 ; i < outData->n ; i++) {
            if (outData->data.U32[i] != (2 * i)) {
                diag("TEST ERROR: the %d-th peak is element number %d\n", i, outData->data.U32[i]);
                testStatus = false;
            }
        }
        psFree(outData);
    }


    // Test interior peaks, with threshold = n*n.
    // Should generate an empty output psVector.
    outData= pmPeaksInVector(inData, (float) (n*n));
    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInVector returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 0) {
            diag("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);
    }
    psFree(inData);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(testStatus);
}


bool test_pmPeaksInImage(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    bool testStatus = true;
    psImage *inData = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psArray *outData = NULL;
    // Initialize test image.
    for (psS32 i = 0 ; i < numRows ; i++) {
        for (psS32 j = 0 ; j < numCols ; j++) {
            inData->data.F32[i][j] = PS_SQR(i - numRows/2) + PS_SQR(j-numCols/2);
        }
    }

    // Set corner and center pixels as peaks.
    inData->data.F32[0][0] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[0][numCols-1] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows-1][0] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows-1][numCols-1] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows/2][numCols/2] = PS_SQR(numRows) + PS_SQR(numCols);

    // Call pmPeaksInImage() with a threshold of 0.0.
    outData = pmPeaksInImage(inData, 0.0);

    if (outData == NULL) {
        diag("TEST ERROR: pmPeaksInImage returned a NULL psList.\n");
        testStatus = false;
    } else {
        psS32 expectedNumPeaks;
        if ((numRows == 1) && (numCols == 1)) {
            expectedNumPeaks = 1;
        } else if ((numRows == 1) || (numCols == 1)) {
            expectedNumPeaks = 3;
        } else {
            expectedNumPeaks = 5;
        }
        if (outData->n != expectedNumPeaks) {
            diag("TEST ERROR: pmPeaksInImage found %ld peaks (should be %d)\n", outData->n, expectedNumPeaks);
            testStatus = false;
        }

        // HEY: verify
        for (psS32 i = 0 ; i < outData->n ; i++) {
            pmPeak *tmpPeak = (pmPeak *) outData->data[i];
            if (((tmpPeak->x == 0) && (tmpPeak->y == 0)) ||
                    ((tmpPeak->x == 0) && (tmpPeak->y == numRows-1)) ||
                    ((tmpPeak->x == numCols-1) && (tmpPeak->y == 0)) ||
                    ((tmpPeak->x == numCols-1) && (tmpPeak->y == numRows-1))) {
                if (!((tmpPeak->type & PM_PEAK_LONE) || (tmpPeak->type & PM_PEAK_EDGE))) {
                    diag("TEST ERROR: (0) peak at (%d, %d) (%f) ->type set improperly (0x%x).",
                          tmpPeak->y, tmpPeak->x, tmpPeak->detValue, tmpPeak->type);
                    diag(" should be (0x%x or 0x%x).\n", PM_PEAK_LONE, PM_PEAK_EDGE);
                    testStatus = false;
                }
            } else if ((tmpPeak->x == numCols/2) && (tmpPeak->y == numRows/2)) {
                if (tmpPeak->type != PM_PEAK_LONE) {
                    diag("TEST ERROR: (1) peak at (%d, %d) (%f) ->type set improperly (0x%x).\n",
                           tmpPeak->y, tmpPeak->x, tmpPeak->detValue, tmpPeak->type);
                    diag(" should be (0x%x).\n", PM_PEAK_LONE);
                    testStatus = false;
                }
            } else {
                diag("TEST ERROR: Peak at (%d, %d) (%f)\n", tmpPeak->y, tmpPeak->x, tmpPeak->detValue);
                testStatus = false;
            }
        }
    }
    psFree(inData);
    psFree(outData);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(testStatus);
}


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(69);


    // ------------------------------------------------------------------------
    // Test pmPeakAlloc()
    {
        psMemId id = psMemGetId();
        pmPeak *tmpPeak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        ok(tmpPeak != NULL, "pmPeakAlloc() returned a non-NULL pmPeak");
        skip_start(tmpPeak == NULL, 9, "Skipping tests because pmPeakAlloc() returned NULL");
        ok(tmpPeak->id == 1, "pmPeakAlloc() set pmPeak->id");
        ok(tmpPeak->x == 1, "pmPeakAlloc() set pmPeak->x");
        ok(tmpPeak->y == 2, "pmPeakAlloc() set pmPeak->y");
        ok(tmpPeak->detValue == 3.0, "pmPeakAlloc() set pmPeak->detValue");
        ok(tmpPeak->flux == 0, "pmPeakAlloc() pmPeak->flux");
        ok(tmpPeak->SN == 0, "pmPeakAlloc() pmPeak->SN");
        ok(tmpPeak->xf == 1, "pmPeakAlloc() pmPeak->xf");
        ok(tmpPeak->yf == 2, "pmPeakAlloc() pmPeak->yF");
        ok(tmpPeak->type == PM_PEAK_LONE, "pmPeakAlloc() pmPeak->type");
        psFree(tmpPeak);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmPeakAlloc(): ensure pmPeak->id is properly incremented.
    {
        psMemId id = psMemGetId();
        pmPeak *tmpPeak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        skip_start(tmpPeak == NULL, 1, "Skipping tests because pmPeakAlloc() returned NULL");
        ok(tmpPeak->id == 2, "pmPeakAlloc() incremented and set pmPeak->id");
        psFree(tmpPeak);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Calling pmPeaksCompareAscend with NULL peak1
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareAscend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend(NULL, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareAscend() returned correct result (peak1 < peak2)");
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareAscend with NULL peak2
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareAscend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend((const void **)peak1, NULL);
        ok(rc == -1, "pmPeaksCompareAscend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareAscend with NULL *peak1
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareAscend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareAscend() returned correct result (peak1 < peak2)");
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareAscend with NULL *peak2
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareAscend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        int rc = pmPeaksCompareAscend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareAscend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Calling pmPeaksCompareAscend with peak1 < peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareAscend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareAscend with peak1 > peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend((const void **)peak1, (const void **) peak2);
        ok(rc == 1, "pmPeaksCompareAscend() returned correct result (peak1 > peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareAscend with peak1 == peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareAscend((const void **)peak1, (const void **) peak2);
        ok(rc == 0, "pmPeaksCompareAscend() returned correct result (peak1 == peak2)", rc);
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Calling pmPeaksCompareDescend with NULL peak1
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareDescend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend(NULL, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareDescend() returned correct result (peak1 < peak2)");
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareDescend with NULL peak2
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareDescend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend((const void **)peak1, NULL);
        ok(rc == -1, "pmPeaksCompareDescend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareDescend with NULL *peak1
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareDescend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareDescend() returned correct result (peak1 < peak2)");
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareDescend with NULL *peak2
    // XXX: This currently seg-faults because NULL args are not pretested in pmPeaksCompareDescend()
    if (0) {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        int rc = pmPeaksCompareDescend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareDescend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Calling pmPeaksCompareDescend with peak1 < peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend((const void **)peak1, (const void **) peak2);
        ok(rc == 1, "pmPeaksCompareDescend() returned correct result (peak1 < peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareDescend with peak1 > peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeaksCompareDescend() returned correct result (peak1 > peak2)");
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksCompareDescend with peak1 == peak2
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        int rc = pmPeaksCompareDescend((const void **)peak1, (const void **) peak2);
        ok(rc == 0, "pmPeaksCompareDescend() returned correct result (peak1 == peak2)", rc);
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmPeakSortBySN() tests
    // int pmPeakSortBySN (const void **a, const void **b)
    // Call pmPeakSortBySN() with acceptable input parameters.
    // XXX: We don't test with NULL input parameters since this functions has no PS_ASSERTS to protect
    // against that.
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        (*peak1)->SN = 10.0;
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        (*peak2)->SN = 20.0;
        int rc = pmPeakSortBySN((const void **)peak1, (const void **) peak2);
        ok(rc == 1, "pmPeakSortBySN() returned correct result (peak1 < peak2) (%d)", rc);
        rc = pmPeakSortBySN((const void **)peak2, (const void **) peak1);
        ok(rc == -1, "pmPeakSortBySN() returned correct result (peak2 < peak1) (%d)", rc);
        rc = pmPeakSortBySN((const void **)peak1, (const void **) peak1);
        ok(rc == 0, "pmPeakSortBySN() returned correct result (peak1 == peak2) (%d)", rc);
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmPeakSortByY() tests
    // int pmPeakSortByY (const void **a, const void **b)
    // Call pmPeakSortByY() with acceptable input parameters.
    // XXX: We don't test with NULL input parameters since this functions has no PS_ASSERTS to protect
    // against that.
    {
        psMemId id = psMemGetId();
        pmPeak **peak1 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak1 = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        (*peak1)->y = 10.0;
        pmPeak **peak2 = (pmPeak **) psAlloc(sizeof(pmPeak *));
        *peak2 = pmPeakAlloc(3, 4, 3.0, PM_PEAK_LONE);
        (*peak2)->y = 20.0;
        int rc = pmPeakSortByY((const void **)peak1, (const void **) peak2);
        ok(rc == -1, "pmPeakSortByY() returned correct result (peak1 < peak2) (%d)", rc);
        rc = pmPeakSortByY((const void **)peak2, (const void **) peak1);
        ok(rc == 1, "pmPeakSortByY() returned correct result (peak2 < peak1) (%d)", rc);
        rc = pmPeakSortByY((const void **)peak1, (const void **) peak1);
        ok(rc == 0, "pmPeakSortByY() returned correct result (peak1 == peak2) (%d)", rc);
        psFree(*peak1);
        psFree(peak1);
        psFree(*peak2);
        psFree(peak2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmPeaksInVector() tests
    // Test pmPeaksInVector() with bad input parameters.
    // Calling pmPeaksInVector with NULL psVector.  Should generate error.
    {
        psMemId id = psMemGetId();
        psVector *tmpVec = pmPeaksInVector(NULL, 0.0);
        ok(tmpVec == NULL, "pmPeaksInVector() returned a NULL with NULL psVector input");
        psFree(tmpVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksInVector with empty psVector.  Should generate error.
    {
        psMemId id = psMemGetId();
        psVector *tmpVecEmpty = psVectorAlloc(0, PS_TYPE_F32);
        psVector *tmpVec = pmPeaksInVector(tmpVecEmpty, 0.0);
        ok(tmpVec == NULL, "pmPeaksInVector() returned a NULL with NULL psVector input");
        psFree(tmpVec);
        psFree(tmpVecEmpty);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksInVector with PS_TYPE_F64 psVector.  Should generate error.
    {
        psMemId id = psMemGetId();
        psVector *tmpVecF64 = psVectorAlloc(TST01_VECTOR_LENGTH, PS_TYPE_F64);
        psVector *tmpVec = pmPeaksInVector(tmpVecF64, 0.0);
        ok(tmpVec == NULL, "pmPeaksInVector() returned a NULL with F64 psVector input");
        psFree(tmpVecF64);
        psFree(tmpVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    ok(test_pmPeaksInVector(1), "Tested pmPeaksInVector() on length 1 input vector");
    ok(test_pmPeaksInVector(10), "Tested pmPeaksInVector() on length 10 input vector");


    // ------------------------------------------------------------------------
    // pmPeaksInImage() tests
    // Calling pmPeaksInImage with NULL psImage.  Should generate error.
    {
        psMemId id = psMemGetId();
        psArray *tmpArray = pmPeaksInImage(NULL, 0.0);
        ok(tmpArray == NULL, "pmPeaksInImage() returned NULL with NULL input image");
        psFree(tmpArray);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling pmPeaksInImage with empty psImage.  Should generate error.
    {
        psMemId id = psMemGetId();
        psImage *tmpImageEmpty = psImageAlloc(0, 0, PS_TYPE_F32);
        psArray *tmpArray = pmPeaksInImage(tmpImageEmpty, 0.0);
        ok(tmpArray == NULL, "pmPeaksInImage() returned NULL with empty input image");
        psFree(tmpArray);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    

    // Calling pmPeaksInImage with PS_TYPE_F64 psImage.  Should generate error
    {
        psMemId id = psMemGetId();
        psImage *tmpImageF64 = psImageAlloc(TST02_NUM_ROWS, TST02_NUM_COLS, PS_TYPE_F64);
        psArray *tmpArray = pmPeaksInImage(tmpImageF64, 0.0);
        ok(tmpArray == NULL, "pmPeaksInImage() returned NULL with F64 input image");
        psFree(tmpImageF64);
        psFree(tmpArray);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // XXX: Uncomment these and debug
    //    testStatus&= test_pmPeaksInImage(1, 1);
    //    testStatus&= test_pmPeaksInImage(2, 5);
    //    testStatus&= test_pmPeaksInImage(5, 2);
    // HEY: add code for small images
    //    testStatus&= test_pmPeaksInImage(1, 1);
    //    testStatus&= test_pmPeaksInImage(1, 8);
    //    testStatus&= test_pmPeaksInImage(8, 1);
    ok(test_pmPeaksInImage(TST02_NUM_ROWS, TST02_NUM_COLS),
      "Tested pmPeaksInImage() on (%d, %d) image", TST02_NUM_ROWS, TST02_NUM_COLS);
    ok(test_pmPeaksInImage(2*TST02_NUM_ROWS, TST02_NUM_COLS),
      "Tested pmPeaksInImage() on (%d, %d) image", 2*TST02_NUM_ROWS, TST02_NUM_COLS);
    ok(test_pmPeaksInImage(TST02_NUM_ROWS, 2*TST02_NUM_COLS),
      "Tested pmPeaksInImage() on (%d, %d) image", TST02_NUM_ROWS, 2*TST02_NUM_COLS);
}
