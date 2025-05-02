/** @file tst_pmReadoutCombine.c
 *
 *  test00() This routine will test the basic functionality.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-03-04 01:01:34 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 *  XXX: Untested:
 * S16, S32 types
 * Multiple input readouts with varying sizes and offsets.
 * params->fracLow and params->fracHigh
 * params->nKeep
 * (gain > 0.0) && (readnoise >= 0.0) (applyZeroScale == true)
 * (gain > 0.0) && (readnoise >= 0.0) (applyZeroScale == false)
 *
 */

#include "psTest.h"
#include "pslib.h"
#include "pmReadoutCombine.h"
static int test00(void);
static int test01(void);
testDescription tests[] = {
                              {test00, 000, "pmSubtractBias(): Basic readout combines with no image overlap", true, false},
                              {test01, 000, "pmSubtractBias(): input parameter error conditions", true, false},
                              {NULL}
                          };

#define NUM_READOUTS  10
#define INPUT_NUM_ROWS 20
#define INPUT_NUM_COLS 20
#define VEC_ZERO 1.0
#define VEC_SCALE 2.0

psS32 VerifyTheOutput(psImage *output, psF32 expect)
{
    bool testStatus = true;

    for (psS32 i = 0 ; i < output->numRows ; i++) {
        for (psS32 j = 0 ; j < output->numCols ; j++) {
            if (output->data.F32[i][j] != expect) {
                printf("TEST ERROR: output[%d][%d] is %.2f, should be %f\n", i, j, output->data.F32[i][j], expect);
                testStatus = false;
            }
        }
    }
    return(testStatus);
}



int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

/******************************************************************************
simpleCombineNoOverlap(): this routine creates a list of NUM_READOUTS input
readouts and calls pmReadoutCombine().
 *****************************************************************************/
int simpleCombineNoOverlap(psS32 numInputCols, psS32 numInputRows)
{
    int i;
    int r;
    psList *list = NULL;
    int baseRowsReadout[NUM_READOUTS];
    int baseColsReadout[NUM_READOUTS];
    int baseRows[NUM_READOUTS];
    int baseCols[NUM_READOUTS];
    int numRows[NUM_READOUTS];
    int numCols[NUM_READOUTS];
    int minOutRow = 10000;
    int minOutCol = 10000;
    int maxOutRow = -1;
    int maxOutCol = -1;
    psImage *output = NULL;
    psCombineParams *params = (psCombineParams *) psAlloc(sizeof(psCombineParams));
    psVector *zero = psVectorAlloc(NUM_READOUTS, PS_TYPE_F32);
    psVector *scale = psVectorAlloc(NUM_READOUTS, PS_TYPE_F32);
    zero->n = zero->nalloc;
    scale->n = scale->nalloc;
    printPositiveTestHeader(stdout, "pmReadoutCombine", "simpleCombineNoOverlap");

    for (i=0;i<NUM_READOUTS;i++) {
        zero->data.F32[i] = VEC_ZERO;
    }
    for (i=0;i<NUM_READOUTS;i++) {
        scale->data.F32[i] = VEC_SCALE;
    }

    params->stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    params->maskVal = 1;
    params->fracLow = 0.0;
    params->fracHigh = 10000.0;
    params->nKeep = 0;

    //
    // Create a psList of psReadouts.  The pixels in readout r will all have the
    // value r.
    //
    for (r=0;r<NUM_READOUTS;r++) {
        baseRowsReadout[r] = r + 40;
        baseColsReadout[r] = r + 42;
        baseRows[r] = r;
        baseCols[r] = r+2;
        numRows[r] = 4 + (2 * r);
        numCols[r] = 8 + (2 * r);

        baseRowsReadout[r] = 0;
        baseColsReadout[r] = 0;
        baseRows[r] = 0;
        baseCols[r] = 0;
        numRows[r] = numInputRows;
        numCols[r] = numInputCols;

        psImage *tmpImage = psImageAlloc(numCols[r], numRows[r], PS_TYPE_F32);
        PS_IMAGE_SET_F32(tmpImage, ((float) r));
        *(int *) (& (tmpImage->row0)) = baseRows[r];
        *(int *) (& (tmpImage->col0)) = baseCols[r];
        pmReadout *tmpReadout = pmReadoutAlloc(NULL);
        tmpReadout->row0 = 0;
        tmpReadout->col0 = 0;
        tmpReadout->image = tmpImage;

        minOutRow = PS_MIN(minOutRow, (baseRowsReadout[r] + baseRows[r]));
        minOutCol = PS_MIN(minOutCol, (baseColsReadout[r] + baseCols[r]));
        maxOutRow = PS_MAX(maxOutRow, (baseRowsReadout[r] + baseRows[r] + numRows[r]));
        maxOutCol = PS_MAX(maxOutCol, (baseColsReadout[r] + baseCols[r] + numCols[r]));

        if (r == 0) {
            list = psListAlloc(tmpReadout);
        } else {
            psListAdd(list, PS_LIST_HEAD, tmpReadout);
        }
    }
    printf("tst_pmReadoutCombine(): (minOutRow, minOutCol) to (maxOutRow, maxOutCol) is (%d, %d) (%d, %d)\n",
           minOutRow, minOutCol, maxOutRow, maxOutCol);

    output = pmReadoutCombine(output, list, params, zero, scale, true, 0.0, 0.0);
    psF32 NR = (psF32) NUM_READOUTS;
    psF32 expectedPixel = ((NR/2.0) * (VEC_ZERO + (VEC_ZERO + VEC_SCALE * (NR - 1)))) / NR;

    int testStatus = VerifyTheOutput(output, expectedPixel);

    psFree(params->stats);
    psFree(params);
    psFree(output);
    psFree(zero);
    psFree(scale);

    psListElem *tmpInput = (psListElem *) list->head;
    while (NULL != tmpInput) {
        pmReadout *tmpReadout = (pmReadout *) tmpInput->data;
        psFree(tmpReadout);
        tmpInput = tmpInput->next;
    }
    psFree(list);

    printFooter(stdout, "pmReadoutCombine", "simpleCombineNoOverlap", true);
    return(testStatus);
}

int test00( void )
{
    int testStatus = 0;

    testStatus |= simpleCombineNoOverlap(1, 1);
    testStatus |= simpleCombineNoOverlap(INPUT_NUM_COLS, 1);
    testStatus |= simpleCombineNoOverlap(1, INPUT_NUM_ROWS);
    testStatus |= simpleCombineNoOverlap(INPUT_NUM_COLS, INPUT_NUM_ROWS);

    return(testStatus);
}

/******************************************************************************
test01(): we simply call pmReadoutCombine() with a variety of erroneous input
parameter combinations and verify that it behaves properly.
 *****************************************************************************/
int test01()
{
    int testStatus = true;
    int i;
    int r;
    psList *list = NULL;
    int baseRowsReadout[NUM_READOUTS];
    int baseColsReadout[NUM_READOUTS];
    int baseRows[NUM_READOUTS];
    int baseCols[NUM_READOUTS];
    int numRows[NUM_READOUTS];
    int numCols[NUM_READOUTS];
    int minOutRow = 10000;
    int minOutCol = 10000;
    int maxOutRow = -1;
    int maxOutCol = -1;
    psImage *output = NULL;
    psImage *rc = NULL;
    psCombineParams *params = (psCombineParams *) psAlloc(sizeof(psCombineParams));
    psVector *zero = psVectorAlloc(NUM_READOUTS, PS_TYPE_F32);
    psVector *zeroHalf = psVectorAlloc(NUM_READOUTS/2, PS_TYPE_F32);
    psVector *zeroBig = psVectorAlloc(NUM_READOUTS+1, PS_TYPE_F32);
    psVector *zeroF64 = psVectorAlloc(NUM_READOUTS, PS_TYPE_F64);
    psVector *scale = psVectorAlloc(NUM_READOUTS, PS_TYPE_F32);
    psVector *scaleHalf = psVectorAlloc(NUM_READOUTS/2, PS_TYPE_F32);
    psVector *scaleBig = psVectorAlloc(NUM_READOUTS*2, PS_TYPE_F32);
    psVector *scaleF64 = psVectorAlloc(NUM_READOUTS, PS_TYPE_F64);
    zero->n = zero->nalloc;
    zeroHalf->n = zeroHalf->nalloc;
    zeroBig->n = zeroBig->nalloc;
    zeroF64->n = zeroF64->nalloc;
    scale->n = scale->nalloc;
    scaleHalf->n = scaleHalf->nalloc;
    scaleBig->n = scaleBig->nalloc;
    scaleF64->n = scaleF64->nalloc;
    for (i=0;i<NUM_READOUTS;i++) {
        zero->data.F32[i] = 3.0;
        zeroBig->data.F32[i] = 3.0;
        zero->data.F32[i] = VEC_ZERO;
        zeroBig->data.F32[i] = VEC_ZERO;
    }
    for (i=0;i<NUM_READOUTS;i++) {
        scale->data.F32[i] = 6.0;
        scaleBig->data.F32[i] = 6.0;
        scale->data.F32[i] = VEC_SCALE;
        scaleBig->data.F32[i] = VEC_SCALE;
    }
    printPositiveTestHeader(stdout, "pmReadoutCombine", "Testing bad input parameter conditions");

    params->stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    params->maskVal = 1;
    params->fracLow = 0.0;
    params->fracHigh = 10000.0;
    params->nKeep = 0;

    for (r=0;r<NUM_READOUTS;r++) {
        baseRowsReadout[r] = 0;
        baseColsReadout[r] = 0;
        baseRows[r] = 0;
        baseCols[r] = 0;
        numRows[r] = INPUT_NUM_ROWS;
        numCols[r] = INPUT_NUM_COLS;

        psImage *tmpImage = psImageAlloc(numCols[r], numRows[r], PS_TYPE_F32);
        PS_IMAGE_SET_F32(tmpImage, ((float) r));
        *(int *) (& (tmpImage->row0)) = baseRows[r];
        *(int *) (& (tmpImage->col0)) = baseCols[r];
        pmReadout *tmpReadout = pmReadoutAlloc(NULL);
        tmpReadout->row0 = 0;
        tmpReadout->col0 = 0;
        tmpReadout->image = tmpImage;
        minOutRow = PS_MIN(minOutRow, (baseRowsReadout[r] + baseRows[r]));
        minOutCol = PS_MIN(minOutCol, (baseColsReadout[r] + baseCols[r]));
        maxOutRow = PS_MAX(maxOutRow, (baseRowsReadout[r] + baseRows[r] + numRows[r]));
        maxOutCol = PS_MAX(maxOutCol, (baseColsReadout[r] + baseCols[r] + numCols[r]));

        if (r == 0) {
            list = psListAlloc(tmpReadout);
        } else {
            psListAdd(list, PS_LIST_HEAD, tmpReadout);
        }
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with NULL zero vector.\n");
    rc = pmReadoutCombine(NULL, list, params, NULL, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        psF32 NR = (psF32) NUM_READOUTS;
        psF32 expectedPixel = ((NR/2.0) * (0.0 + (0.0 + VEC_SCALE * (NR - 1)))) / NR;
        if (false == VerifyTheOutput(rc, expectedPixel)) {
            testStatus = false;
        }

        if (rc->type.type != scale->type.type) {
            printf("TEST ERROR: output readout->image has incorrect type.\n");
            testStatus = false;
        }
        psFree(rc);
    } else {
        printf("TEST ERROR: pmReadoutCombine() returned NULL\n");
        testStatus = false;

    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect length zero vector (too small).  Should generate error.\n");
    rc = pmReadoutCombine(NULL, list, params, zeroHalf, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect type zero vector.  Should generate error.\n");
    rc = pmReadoutCombine(NULL, list, params, zeroF64, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect length zero vector (too big).  Should generate warning.\n");
    rc = pmReadoutCombine(output, list, params, zeroBig, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        psF32 NR = (psF32) NUM_READOUTS;
        psF32 expectedPixel = ((NR/2.0) * (VEC_ZERO + (VEC_ZERO + VEC_SCALE * (NR - 1)))) / NR;

        if (false == VerifyTheOutput(rc, expectedPixel)) {
            testStatus = false;
        }
        psFree(rc);
        rc = NULL;
    } else {
        printf("TEST ERROR: pmReadoutCombine() returned NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with NULL scale vector.\n");
    rc = pmReadoutCombine(output, list, params, zero, NULL, true, 0.0, 0.0);

    if (rc != NULL) {
        psF32 NR = (psF32) NUM_READOUTS;
        psF32 expectedPixel = ((NR/2.0) * (VEC_ZERO + (VEC_ZERO + 1.0 * (NR - 1)))) / NR;
        if (false == VerifyTheOutput(rc, expectedPixel)) {
            testStatus = false;
        }

        if (rc->type.type != scale->type.type) {
            printf("TEST ERROR: output readout->image has incorrect type.\n");
            testStatus = false;
        }
        psFree(rc);
    } else {
        printf("TEST ERROR: pmReadoutCombine() returned NULL\n");
        testStatus = false;

    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect length scale vector (too small).  Should generate error.\n");
    rc = pmReadoutCombine(output, list, params, zero, scaleHalf, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect type scale vector.  Should generate error.\n");
    rc = pmReadoutCombine(output, list, params, zero, scaleF64, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with incorrect length scale vector (too big).  Should generate warning.\n");
    rc = pmReadoutCombine(output, list, params, zero, scaleBig, true, 0.0, 0.0);
    if (rc != NULL) {
        psF32 NR = (psF32) NUM_READOUTS;
        psF32 expectedPixel = ((NR/2.0) * (VEC_ZERO + (VEC_ZERO + VEC_SCALE * (NR - 1)))) / NR;

        if (false == VerifyTheOutput(rc, expectedPixel)) {
            testStatus = false;
        }
        psFree(rc);
        rc = NULL;
    } else {
        printf("TEST ERROR: pmReadoutCombine() returned NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() insufficient size output image.  Should generate error, return NULL.\n");
    output = psImageAlloc(1, 1, PS_TYPE_F32);
    rc = pmReadoutCombine(output, list, params, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }
    psFree(output);
    output = NULL;

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() row0/col0 too large.  Should generate error, return NULL.\n");
    output = psImageAlloc(1, 1, PS_TYPE_F32);
    *(psS32*)&output->row0 = 10000;
    *(psS32*)&output->col0 = 10000;
    rc = pmReadoutCombine(output, list, params, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }
    psFree(output);
    output = NULL;

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with NULL input list.  Should generate error, return NULL.\n");
    rc = pmReadoutCombine(output, NULL, params, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    printf("Calling pmReadoutCombine() with NULL params.  Should generate error, return NULL.\n");
    rc = pmReadoutCombine(output, list, NULL, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------\n");
    psStatsOptions oldStatsOpts = params->stats->options |= PS_STAT_MIN;
    printf("Calling pmReadoutCombine() with multiple stats->options.  Should generate error, return NULL.\n");
    rc = pmReadoutCombine(output, list, params, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }
    params->stats->options = oldStatsOpts;

    printf("----------------------------------------------------------------------------\n");
    psStats *oldStats = params->stats;
    printf("Calling pmReadoutCombine() with NULL param->stats.  Should generate error, return NULL.\n");
    params->stats = NULL;
    rc = pmReadoutCombine(output, list, params, zero, scale, true, 0.0, 0.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmReadoutCombine() did not return NULL\n");
        testStatus = false;
    }
    params->stats = oldStats;

    printf("----------------------------------------------------------------------------\n");
    printf("============================================================================\n");

    psFree(params->stats);
    psFree(params);
    //    psFree(output);
    //    psFree(rc);
    psFree(zero);
    psFree(zeroHalf);
    psFree(zeroBig);
    psFree(zeroF64);
    psFree(scale);
    psFree(scaleHalf);
    psFree(scaleBig);
    psFree(scaleF64);
    psListElem *tmpInput = (psListElem *) list->head;
    while (NULL != tmpInput) {
        pmReadout *tmpReadout = (pmReadout *) tmpInput->data;
        psFree(tmpReadout);
        tmpInput = tmpInput->next;
    }
    psFree(list);

    printFooter(stdout, "pmReadoutCombine", "Testing bad input parameter conditions", true);
    return(testStatus);
}
