/*****************************************************************************
We test:
    psHistogramAlloc()
    psHistogramAllocGeneric()
    psVectorHistogram(): uniform histograms
    psVectorHistogram(): generic histograms
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define ERROR_TOLERANCE 0.001
#define NUM_DATA 10000

void genericTestHistogramAlloc(psS32 numBins, psS32 lower, psS32 higher)
{
    // Allocate the psHistogram structure
    {
        psMemId id = psMemGetId();
        psHistogram *myHist = psHistogramAlloc(lower, higher, numBins);
        ok(myHist != NULL, "psHistogram allocated successfully");
        skip_start(myHist == NULL, 7, "Skipping tests because psHistogramAlloc() failed");
        ok(myHist->nums->n = numBins, "psHistogramAlloc() set the correct number of bins");
        ok(myHist->bounds->n == numBins+1, "psHistogramAlloc() set the correct number of bounds");
        ok(myHist->minNum == 0, "psHistogramAlloc() initialized minNum to 0.0");
        ok(myHist->maxNum == 0, "psHistogramAlloc() initialized maxNum to 0.0");
        ok(myHist->uniform == true, "psHistogramAlloc() initialized ->uniform to true");

        bool errorFlag = false;
        for (int i=0;i<numBins;i++)
        {
            if (myHist->nums->data.F32[i] != 0.0) {
                diag("ERROR: myHist->nums->data.U32[%d] not initialized to 0.\n", i);
                errorFlag = true;
            }
            myHist->nums->data.F32[i] = 0.0;
        }
        ok(!errorFlag, "psHistogramAlloc() initialized the data to 0.0");

        errorFlag = false;
        for (int i=0;i<numBins;i++)
        {
            psF32 expectedBound = (psF32) (i + 1);
            if (fabs(expectedBound - myHist->bounds->data.F32[i]) > ERROR_TOLERANCE) {
                diag("Bin number %d bounds: (%6.3f - %6.3f)\n", i,
                     myHist->bounds->data.F32[i],
                     myHist->bounds->data.F32[i+1]);
                diag("Was %f, expected %f\n", myHist->bounds->data.F32[i], expectedBound);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psHistogramAlloc() set the histogram bounds correctly");

        skip_end();
        psFree(myHist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


void genericTestHistogramAllocGeneric(psS32 numBins, psS32 lower, psS32 higher)
{
    {
        psMemId id = psMemGetId();
        psVector *myBounds = psVectorAlloc(numBins+1, PS_TYPE_F32);
        for (int i=0;i<numBins+1;i++) {
            myBounds->data.F32[i] = lower + ((higher - lower) / (float) numBins) *
		(float) i;
        }
        psHistogram *myHist = psHistogramAllocGeneric(myBounds);
        ok(myHist != NULL, "psHistogram allocated successfully");
        skip_start(myHist == NULL, 7, "Skipping tests because psHistogramAlloc() failed");
        ok(myHist->nums->n = numBins, "psHistogramAllocGeneric() set the correct number of bins");
        ok(myHist->bounds->n == numBins+1, "psHistogramAllocGeneric() set the correct number of bounds");
        ok(myHist->minNum == 0, "psHistogramAllocGeneric() initialized minNum to 0.0");
        ok(myHist->maxNum == 0, "psHistogramAllocGeneric() initialized maxNum to 0.0");
        ok(myHist->uniform == false, "psHistogramAllocGeneric() initialized ->uniform to false");

        bool errorFlag = false;
        for (int i=0;i<numBins;i++) {
            if (myHist->nums->data.F32[i] != 0.0) {
                diag("ERROR: myHist->nums->data.F32[%d] not initialized to 0.\n", i);
                errorFlag = false;
            }
        }
        ok(!errorFlag, "psHistogramAllocGeneric() initialized the data to 0.0");

        errorFlag = false;
        for (int i=0;i<numBins;i++) {
            psF32 expectedBound = (psF32) (i + 1);
            if (fabs(expectedBound - myHist->bounds->data.F32[i]) > ERROR_TOLERANCE) {
                diag("Bin number %d bounds: (%6.3f - %6.3f)\n", i,
                     myHist->bounds->data.F32[i],
                     myHist->bounds->data.F32[i+1]);
                diag("Was %f, expected %f\n", myHist->bounds->data.F32[i], expectedBound);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psHistogramAlloc() set the histogram bounds correctly");

        skip_end();
        psFree(myHist);
        psFree(myBounds);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    }
}


void genericTestVectorHistogram1(psS32 numBins, psS32 lower, psS32 higher)
{
    if ((numBins%2) != 0) {
        diag("Configuration error: must run this test with even number of bins");
        return;
    }
    if ((NUM_DATA%numBins) != 0) {
        diag("Configuration error: NUM_DATA must be an integer multiple of numBins");
        return;
    }

    // Allocate the psHistogram structure, ensure that psVectorHistogram() sets
    // the data correctly when called with a NULL mask.
    {
        psMemId id = psMemGetId();
        psVector *myData = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        for (int i = 0;i < NUM_DATA;i++) {
            myData->data.F32[i] = lower + ((higher - lower) / (float) NUM_DATA) * (float) i;
        }

        psHistogram *myHist = psHistogramAlloc(lower, higher, numBins);
        bool rc = psVectorHistogram(myHist, myData, NULL, NULL, 0);
        ok(rc == true, "psVectorHistogram() returned TRUE");
        skip_start(myHist == NULL, 1, "Skipping tests because psVectorHistogram() returned NULL");

        bool errorFlag = false;
        for (int i = 0; i < numBins; i++) {

	    // we need to explicitly count the number in each bin.  we can fool ourselves with
	    // rounding otherwise
	    int nValues = 0;
	    for (int j = 0; j < myData->n; j++) {
		// valid bin: bound[bin] <= value < bound[bin+1]
		if (myData->data.F32[j] < myHist->bounds->data.F32[i]) continue;
		if (myData->data.F32[j] >= myHist->bounds->data.F32[i+1]) continue;
		nValues ++;
	    }
	    if (myHist->nums->data.F32[i] != nValues) {
		diag("Bin number %d bounds: (%.2f - %.2f) data (%.2f) should be (%2d)\n",
		     i,
		     myHist->bounds->data.F32[i],
		     myHist->bounds->data.F32[i+1],
		     myHist->nums->data.F32[i], nValues);
		errorFlag = true;
	    }
        }
        ok(!errorFlag, "psVectorHistogram() correctly set the histogram");
        skip_end();
        psFree(myHist);
        psFree(myData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate the psHistogram structure, ensure that psVectorHistogram() sets
    // the data correctly when called with a mask.
    {
        psMemId id = psMemGetId();
        psVector *myData = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        myData->n = myData->nalloc;
        for (int i = 0;i < NUM_DATA;i++) {
            myData->data.F32[i] = lower + ((higher - lower) / (float) NUM_DATA) * (float) i;
        }

        psVector *myMask = psVectorAlloc(NUM_DATA, PS_TYPE_U8);
        myMask->n = myMask->nalloc;
        for (int i = 0;i < NUM_DATA;i++) {
            if (i >= (NUM_DATA / 2)) {
                myMask->data.U8[i] = 1;
            } else {
                myMask->data.U8[i] = 0;
            }
        }
        psHistogram *myHist = psHistogramAlloc(lower, higher, numBins);
        bool rc = psVectorHistogram(myHist, myData, NULL, myMask, 1);
        ok(rc == true, "psVectorHistogram() returned TRUE");
        skip_start(myHist == NULL, 1, "Skipping tests because psVectorHistogram() returned NULL");

        bool errorFlag = false;
        for (int i = 0;i < numBins;i++) {

	    // we need to explicitly count the number in each bin.  we can fool ourselves with
	    // rounding otherwise
	    int nValues = 0;
	    for (int j = 0; j < myData->n; j++) {
		// valid bin: bound[bin] <= value < bound[bin+1]
		if (myMask->data.U8[j] == 1) continue;
		if (myData->data.F32[j] < myHist->bounds->data.F32[i]) continue;
		if (myData->data.F32[j] >= myHist->bounds->data.F32[i+1]) continue;
		nValues ++;
	    }
	    if (myHist->nums->data.F32[i] != nValues) {
		diag("Bin number %d bounds: (%6.3f - %6.3f) data (%f) should be (%.2f)\n", i,
		     myHist->bounds->data.F32[i],
		     myHist->bounds->data.F32[i + 1],
		     myHist->nums->data.F32[i], (float) (NUM_DATA/numBins));
		errorFlag = true;
	    }
        }
        ok(!errorFlag, "psVectorHistogram() correctly set the histogram");
        skip_end();
        psFree(myHist);
        psFree(myMask);
        psFree(myData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the return value is NULL and program execution doesn't stop,
    // if input psHistogram parameter is NULL.
    {
        psMemId id = psMemGetId();
        psVector *myData = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        myData->n = myData->nalloc;
        for (int i = 0;i < NUM_DATA;i++) {
            myData->data.F32[i] = lower + ((higher - lower) / (float) NUM_DATA) * (float) i;
        }
        ok(false == psVectorHistogram(NULL, myData, NULL, NULL, 0),
	   "psVectorHistogram() returned FALSE with a NULL psHistogram input");
        psFree(myData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the return value is the same as the input parameter myHist and
    // program execution doesn't stop, if the input parameter myArray is
    // NULL.
    {
        psMemId id = psMemGetId();
        psHistogram *myHist = psHistogramAlloc(lower, higher, numBins);
        ok(true == psVectorHistogram(myHist, NULL, NULL, NULL, 0),
	   "psVectorHistogram() returns TRUE with a NULL input array");
        psFree(myHist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the return value is the same as the input parameter myHist and
    // program execution doesn't stop, if the input parameter myArray has no
    // elements.
    {
        psMemId id = psMemGetId();
        psVector *myData = psVectorAlloc(0, PS_TYPE_F32);
        psHistogram *myHist = psHistogramAlloc(lower, higher, numBins);
        ok(true == psVectorHistogram(myHist, NULL, NULL, NULL, 0),
	   "psVectorHistogram() returns TRUE with an empty input array");
        psFree(myHist);
        psFree(myData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

//#define NUM_DATA 20
void genericTestVectorHistogram2(psS32 numBins, psS32 lower, psS32 higher)
{
    if (numBins > NUM_DATA) {
        diag("Configuration error: NUM_DATA must be larger than numBins");
        return;
    }


    {
        psMemId id = psMemGetId();
        psVector *myData = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        for (int i = 0;i < NUM_DATA;i++) {
            myData->data.F32[ i ] = lower + ((higher-lower) / (float) NUM_DATA) * (float) i;
        }

        psVector *myBounds = psVectorAlloc(numBins+1, PS_TYPE_F32);
        for (int i=0;i<numBins+1;i++) {
            myBounds->data.F32[i] = lower + ((higher - lower) / (float) numBins) * (float) i;
        }

        psHistogram *myHist = psHistogramAllocGeneric(myBounds);
        ok(myHist->nums->n == numBins, "psHistogramAllocGeneric() set correct number of bins");
        ok(myHist->bounds->n == numBins+1, "psHistogramAllocGeneric() set correct number of bounds");
        ok(myHist->minNum == 0, "psHistogramAlloc() initialized minNum to 0.0");
        ok(myHist->maxNum == 0, "psHistogramAlloc() initialized maxNum to 0.0");
        ok(myHist->uniform == false, "psHistogramAlloc() initialized ->uniform to false");

        bool errorFlag = false;
        for (int i=0;i<numBins;i++) {
            if (fabs(myHist->nums->data.F32[i]) > ERROR_TOLERANCE) {
                diag("ERROR: myHist->nums->data.F32[%d] not initialized to 0.\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psVectorHistogram() correctly initialized the bins");

        bool rc = psVectorHistogram(myHist, myData, NULL, NULL, 0);
        ok(rc == true, "psVectorHistogram() returned TRUE");
        skip_start(rc == false, 1, "Skipping tests because psVectorHistogram() returned FALSE");
        for (int i=0;i<numBins;i++) {

	    // we need to explicitly count the number in each bin.  we can fool ourselves with
	    // rounding otherwise
	    int nValues = 0;
	    for (int j = 0; j < myData->n; j++) {
		// valid bin: bound[bin] <= value < bound[bin+1]
		if (myData->data.F32[j] < myHist->bounds->data.F32[i]) continue;
		if (myData->data.F32[j] >= myHist->bounds->data.F32[i+1]) continue;
		nValues ++;
	    }
	    if (myHist->nums->data.F32[i] != nValues) {
                diag("Bin number %d bounds: (%.2f - %.2f): data: (%.2f) should be (%.2f)\n", i,
                     myHist->bounds->data.F32[i],
                     myHist->bounds->data.F32[i+1],
                     myHist->nums->data.F32[i], nValues);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psVectorHistogram() correctly set the bins");
        skip_end();
        psFree(myData);
        psFree(myHist);
        psFree(myBounds);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


psS32 main()
{
    psLogSetLevel(PS_LOG_INFO);
    psLogSetFormat("HLNM");
    plan_tests(168);


    // psHistogramAlloc() tests
    // Call with lower bound larger than upper bound.  Should return NULL.
    {
        psMemId id = psMemGetId();
        psHistogram *myHist = psHistogramAlloc(10, 5, 1);
        ok(myHist == NULL, "psHistogramAlloc() returned NULL with lower bound larger than upper bound");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with 0 numBins.  Should return NULL.
    {
        psMemId id = psMemGetId();
        psHistogram *myHist = psHistogramAlloc(5, 10, 0);
        ok(myHist == NULL, "psHistogramAlloc() returned NULL with 0 numBins");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psHistogramAlloc() tests
    {
        genericTestHistogramAlloc(1, 1, 2);
        genericTestHistogramAlloc(2, 1, 3);
        genericTestHistogramAlloc(10, 1, 11);
        genericTestHistogramAlloc(20, 1, 21);
    }


    // psHistogramAllocGeneric() tests
    // Use NULL psVector* bounds struct.
    {
        psMemId id = psMemGetId();
        psHistogram *myHist = psHistogramAllocGeneric(NULL);
        ok(myHist == NULL, "psHistogram() returned NULL with NULL psVector* bounds struct");
        psFree(myHist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use unallowed type for bounds
    {
        psMemId id = psMemGetId();
        psS32 numBins = 10;
        psS32 lower = 5;
        psS32 higher = 10;
        psVector *myBounds = psVectorAlloc(numBins+1, PS_TYPE_F64);
        for (int i=0;i<numBins+1;i++) {
            myBounds->data.F64[i] = lower + ((higher - lower) / (float) numBins) * (float) i;
        }
        psHistogram *myHist = psHistogramAllocGeneric(myBounds);
        ok(myHist == NULL, "psHistogram() returned NULL with unallowed type for psVector* bounds struct");
        ok(myBounds != NULL, "psHistogram() did not free bounds");
        psFree(myHist);
        psFree(myBounds);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use only a single bound
    {
        psMemId id = psMemGetId();
        psVector *myBounds = psVectorAlloc(1, PS_TYPE_F32);
        psHistogram *myHist = psHistogramAllocGeneric(myBounds);
        ok(myHist == NULL, "psHistogram() returned NULL with only a single bound");
        ok(myBounds != NULL, "psHistogram() did not free bounds");
        psFree(myHist);
        psFree(myBounds);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psHistogramAllocGeneric() tests
    {
        genericTestHistogramAllocGeneric(1, 1, 2);
        genericTestHistogramAllocGeneric(2, 1, 3);
        genericTestHistogramAllocGeneric(10, 1, 11);
        genericTestHistogramAllocGeneric(20, 1, 21);
    }


    // psVectorHistogram() tests: uniform histograms
    {
        genericTestVectorHistogram1(2, 1, 3);
        genericTestVectorHistogram1(10, 1, 11);
        genericTestVectorHistogram1(20, 1, 21);
        genericTestVectorHistogram1(500, 1, 501);
    }


    // psVectorHistogram() tests: generic histograms
    {
        genericTestVectorHistogram2(1, 1, 2);
        genericTestVectorHistogram2(2, 1, 3);
        genericTestVectorHistogram2(10, 1, 11);
        genericTestVectorHistogram2(20, 1, 21);
    }
}
