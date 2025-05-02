/* @file tap_pmNonLinear.c
 *
 *  XXX: Add tests 
 *      Input psVectors and psImages have incorrect type
 *      Input psVectors are wrong size
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define TEST_NUM_ROWS 8
#define TEST_NUM_COLS 8
#define NUM_BIAS_DATA 2
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (int i=0;i<TEST_NUM_ROWS;i++) {
        for (int j=0;j<TEST_NUM_COLS;j++) {
            readout->image->data.F32[i][j] = (float) (i + j);
        }
    }
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(readout);
}

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(72);


    // ----------------------------------------------------------------------
    // pmNonLinearityPolynomial() tests
    // Call pmNonLinearityPolynomial() with NULL input readout.
    {
        psMemId id = psMemGetId();
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        myPoly->coeff[1] = 1.0;
        pmReadout *rc = pmNonLinearityPolynomial(NULL, myPoly);
        ok(!rc, "pmNonLinearityPolynomial() returned NULL with a NULL input pmReadout");
        psFree(myPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    
     
    // Call pmNonLinearityPolynomial() with NULL input readout->image.
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        myPoly->coeff[1] = 1.0;
        psImage *tmpImage = myReadout->image;
        myReadout->image = NULL;
        pmReadout *rc = pmNonLinearityPolynomial(myReadout, myPoly);
        ok(!rc, "pmNonLinearityPolynomial() returned NULL with a NULL input pmReadout");
        myReadout->image = tmpImage;
        psFree(myReadout);
        psFree(myPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    
     
    // Call pmNonLinearityPolynomial() with NULL polynomial.
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        pmReadout *rc = pmNonLinearityPolynomial(myReadout, NULL);
        ok(!rc, "pmNonLinearityPolynomial() returned NULL with a NULL input psPolynomial");
        psFree(myReadout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    
     
    // Call pmNonLinearityPolynomial() with acceptable input data
    { 
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        myPoly->coeff[1] = 1.0;
        myReadout = pmNonLinearityPolynomial(myReadout, myPoly);
        bool errorFlag = false;
        for (int i=0;i<TEST_NUM_ROWS;i++) {
            for (int j=0;j<TEST_NUM_COLS;j++) {
                float expect = psPolynomial1DEval(myPoly, (float) (i + j));
                float actual = myReadout->image->data.F32[i][j];
                if (FLT_EPSILON < fabs(expect - actual)) {
                    if (VERBOSE) diag("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmNonLinearityPolynomial() set the image data correctly");
    
        psFree(myReadout);
        psFree(myPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmNonLinearityLookup() tests
    // Call pmNonLinearityLookup() with NULL input pmReadout.
    {
        psMemId id = psMemGetId();
        psVector *inVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        psVector *outVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        for (psS32 i=0;i<PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3;i++) {
            inVec->data.F32[i] = (float) i;
            outVec->data.F32[i] = (float) (2 * i);
        }
        pmReadout *rc = pmNonLinearityLookup(NULL, inVec, outVec);
        ok(!rc, "pmNonLinearityLookup() returned NULL with NULL input pmReadout");
        psFree(inVec);
        psFree(outVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmNonLinearityLookup() with NULL input pmReadout->image
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psImage *tmpImage = myReadout->image;
        myReadout->image = NULL;
        psVector *inVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        psVector *outVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        for (psS32 i=0;i<PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3;i++) {
            inVec->data.F32[i] = (float) i;
            outVec->data.F32[i] = (float) (2 * i);
        }
        pmReadout *rc = pmNonLinearityLookup(myReadout, inVec, outVec);
        ok(!rc, "pmNonLinearityLookup() returned NULL with NULL input pmReadout->image");
        myReadout->image = tmpImage;
        psFree(myReadout);
        psFree(inVec);
        psFree(outVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmNonLinearityLookup() with NULL input inFlux
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psVector *inVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        psVector *outVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        for (psS32 i=0;i<PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3;i++) {
            inVec->data.F32[i] = (float) i;
            outVec->data.F32[i] = (float) (2 * i);
        }
        pmReadout *rc = pmNonLinearityLookup(myReadout, NULL, outVec);
        ok(!rc, "pmNonLinearityLookup() returned NULL with input inFlux");
        psFree(myReadout);
        psFree(inVec);
        psFree(outVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call pmNonLinearityLookup() with NULL input outFlux
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psVector *inVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        psVector *outVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        for (psS32 i=0;i<PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3;i++) {
            inVec->data.F32[i] = (float) i;
            outVec->data.F32[i] = (float) (2 * i);
        }
        pmReadout *rc = pmNonLinearityLookup(myReadout, inVec, NULL);
        ok(!rc, "pmNonLinearityLookup() returned NULL with input outFlux");
        psFree(myReadout);
        psFree(inVec);
        psFree(outVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmNonLinearityLookup() with acceptable input data
    {
        psMemId id = psMemGetId();
        pmReadout *myReadout = generateSimpleReadout(NULL);
        psVector *inVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        psVector *outVec = psVectorAlloc(PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3, PS_TYPE_F32);
        for (psS32 i=0;i<PS_MAX(TEST_NUM_COLS, TEST_NUM_ROWS)*3;i++) {
            inVec->data.F32[i] = (float) i;
            outVec->data.F32[i] = (float) (2 * i);
        }
        {
            myReadout = pmNonLinearityLookup(myReadout, inVec, outVec);
            ok(myReadout, "pmNonLinearityLookup() returned non-NULL with acceptable input data");
            bool errorFlag = false;
            if (myReadout) {
                for (int i=0;i<TEST_NUM_ROWS;i++) {
                    for (int j=0;j<TEST_NUM_COLS;j++) {
                        float expect = (float) (2 * (i + j));
                        float actual = myReadout->image->data.F32[i][j];
                        if (FLT_EPSILON < fabs(expect - actual)) {
                            if (VERBOSE) diag("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                            errorFlag = true;
                        }
                    }
                }
                ok(!errorFlag, "pmNonLinearityLookup() set the image data correctly");
	    }
	}
        psFree(myReadout);
        psFree(inVec);
        psFree(outVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
