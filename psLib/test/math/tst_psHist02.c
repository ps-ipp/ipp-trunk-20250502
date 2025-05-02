/*****************************************************************************
   This routine must ensure that the psHistogram structure is correctly
   populated by the procedure psGetArrayHistogram().
 
*****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include "psMemory.h"
#define MISC_FLOAT_NUMBER 345.0
#define MISC_INT_NUMBER 345
#define LOWER 20.0
#define UPPER 30.0
#define NUM_DATA 10000

psS32 main()
{
    psLogSetFormat("HLNM");
    psHistogram * myHist = NULL;
    psHistogram *myHist2 = NULL;
    psVector *myData = NULL;
    psVector *myMask = NULL;
    psS32 testStatus = true;
    psS32 memLeaks = 0;
    psS32 nb = 0;
    psS32 numBins = 0;
    psS32 i = 0;
    psS32 currentId = 0;

    currentId = psMemGetId();

    /*********************************************************************/
    /*  Allocate and initialize data structures                          */
    /*********************************************************************/
    myData = psVectorAlloc( NUM_DATA, PS_TYPE_F32 );
    myData->n = myData->nalloc;
    for ( i = 0;i < NUM_DATA;i++ ) {
        myData->data.F32[ i ] = LOWER + ( ( UPPER - LOWER ) / ( float ) NUM_DATA ) * ( float ) i;
    }

    myMask = psVectorAlloc( NUM_DATA, PS_TYPE_U8 );
    myMask->n = myMask->nalloc;
    for ( i = 0;i < NUM_DATA;i++ ) {
        if ( i >= ( NUM_DATA / 2 ) ) {
            myMask->data.U8[ i ] = 1;
        } else {
            myMask->data.U8[ i ] = 0;
        }
    }

    for ( nb = 0;nb < 4;nb++ ) {
        if ( nb == 0 )
            numBins = 1;
        if ( nb == 1 )
            numBins = 2;
        if ( nb == 2 )
            numBins = 10;
        if ( nb == 3 )
            numBins = 20;

        /*********************************************************************/
        /*  Allocate and Perform Histogram, no mask                          */
        /*********************************************************************/
        printPositiveTestHeader( stdout,
                                 "psStats functions",
                                 "Allocate and Perform Histogram, no mask" );

        myHist = psHistogramAlloc( LOWER, UPPER, numBins );
        myHist = psVectorHistogram( myHist, myData, NULL, NULL, 0 );

        for ( i = 0;i < numBins;i++ ) {
            printf( "Bin number %d bounds: (%.2f - %.2f) data (%f)\n", i,
                    myHist->bounds->data.F32[ i ],
                    myHist->bounds->data.F32[ i + 1 ],
                    myHist->nums->data.F32[ i ] );
        }
        psMemCheckCorruption( 1 );
        psFree( myHist );
        psMemCheckCorruption( 1 );

        printFooter( stdout,
                     "psStats functions",
                     "Allocate and Perform Histogram, no mask",
                     testStatus );

        /*********************************************************************/
        /*  Allocate and Perform Histogram with mask                         */
        /*********************************************************************/
        printPositiveTestHeader( stdout,
                                 "psStats functions",
                                 "Allocate and Perform Histogram with mask" );

        myHist = psHistogramAlloc( LOWER, UPPER, numBins );
        myHist = psVectorHistogram( myHist, myData, NULL, myMask, 1 );

        for ( i = 0;i < numBins;i++ ) {
            printf( "Bin number %d bounds: (%6.3f - %6.3f) data (%f)\n", i,
                    myHist->bounds->data.F32[ i ],
                    myHist->bounds->data.F32[ i + 1 ],
                    myHist->nums->data.F32[ i ] );
        }
        psMemCheckCorruption( 1 );
        psFree( myHist );
        psMemCheckCorruption( 1 );

        printFooter( stdout,
                     "psStats functions",
                     "Allocate and Perform Histogram with mask",
                     testStatus );
    }
    psFree( myMask );

    printPositiveTestHeader( stdout,
                             "psStats functions",
                             "Calling psVectorHistogram() with various NULL inputs." );



    // ********************************************************************
    // Verify the return value is null and program execution doesn't stop,
    // if input parameter myHist is null.

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message.");
    myHist2 = psVectorHistogram( NULL, myData, NULL, NULL, 0 );
    if ( myHist2 != NULL ) {
        printf( "ERROR: myHist2!=NULL\n" );
        testStatus = false;
    }
    psFree( myData );


    // ********************************************************************
    // Verify the return value is the same as the input parameter myHist and
    // program execution doesn't stop, if the input parameter myArray is
    // null.

    myHist = psHistogramAlloc( LOWER, UPPER, numBins );
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message.");
    myHist = psVectorHistogram( myHist, NULL, NULL, NULL, 0 );
    if ( myHist == NULL ) {
        printf( "ERROR: myHist==NULL\n" );
        testStatus = false;
    }
    psFree( myHist );


    //    exit(0);
    // ********************************************************************
    // Verify the return value is the same as the input parameter myHist and
    // program execution doesn't stop, if the input parameter myArray has no
    // elements.
    // NOTE: This code segment is commented out because psVectorAlloc returns
    // NULL if called with an N element data.
    /*
    myData = psVectorAlloc(0, PS_TYPE_F32);
    myData->n = myData->nalloc;
    myHist = psHistogramAlloc(LOWER, UPPER, numBins);
    myHist = psVectorHistogram(myHist, NULL, NULL, NULL, 0);
    if (myHist == NULL) {
        printf("ERROR: myHist==NULL\n");
        testStatus = false;
    }
    psFree(myHist);
    psFree(myData);
    */
    printFooter( stdout,
                 "psStats functions",
                 "Calling psVectorHistogram() with various NULL inputs.",
                 testStatus );

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/
    printPositiveTestHeader( stdout,
                             "psStats functions",
                             "Deallocate the psHistogram structure." );

    psMemCheckCorruption( 1 );
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if ( 0 != memLeaks ) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks );
    }
    psMemCheckCorruption( 1 );

    printFooter( stdout,
                 "psStats functions",
                 "Deallocate the psHistogram structure.",
                 testStatus );

    return ( !testStatus );
}
